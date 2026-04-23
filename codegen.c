// codegen.c  —  LLVM IR generation with int / float type support
//
// Type strategy
// -------------
// Variables are stored as doubles (LLVMDoubleType) in all cases: this
// avoids alloca-type mismatches when the same variable is assigned both
// an integer and a float expression across separate statements.
//
// gen_expr() always returns a double value.  Integer literals and
// integer sub-expressions are promoted with SIToFP exactly once, at
// the leaf / coercion point, so no implicit truncation ever happens
// silently.
//
// NODE_CAST(->float)  : SIToFP (int -> double)
// NODE_CAST(->int)    : FPToSI (double -> i32, truncation, explicit)
//
// printf uses "%g\n" so integers print without a decimal point when
// the fractional part is zero, and floats print naturally otherwise.
//
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"

/* ------------------------------------------------------------------ */
/* Symbol table: name -> alloca (always stored as double)             */
/* ------------------------------------------------------------------ */
#define MAX_VARS 64
static struct { char name[64]; LLVMValueRef alloca; } vars[MAX_VARS];
static int  nv = 0;
static int  codegen_errors = 0;

static LLVMValueRef lookup(const char *name) {
    for (int i = 0; i < nv; i++)
        if (!strcmp(vars[i].name, name)) return vars[i].alloca;
    return NULL;
}

static LLVMValueRef declare(LLVMBuilderRef B, const char *name) {
    /* All variables are stored as double regardless of the RHS type. */
    LLVMValueRef a = LLVMBuildAlloca(B, LLVMDoubleType(), name);
    strncpy(vars[nv].name, name, 63);
    vars[nv++].alloca = a;
    return a;
}

/* ------------------------------------------------------------------ */
/* gen_expr: always returns an LLVMValueRef of type double            */
/* ------------------------------------------------------------------ */
static LLVMValueRef gen_expr(LLVMBuilderRef B, ASTNode *n) {
    if (!n) return LLVMConstReal(LLVMDoubleType(), 0.0);

    switch (n->kind) {

    /* ---- literals ------------------------------------------------- */
    case NODE_NUM:
        /* Promote integer literal directly to double. */
        return LLVMConstReal(LLVMDoubleType(), (double)n->ival);

    case NODE_FLOAT:
        return LLVMConstReal(LLVMDoubleType(), n->fval);

    /* ---- variable load -------------------------------------------- */
    case NODE_VAR: {
        LLVMValueRef a = lookup(n->name);
        if (!a) {
            fprintf(stderr, "Error: undeclared variable '%s' (defaulting to 0.0)\n", n->name);
            codegen_errors++;
            return LLVMConstReal(LLVMDoubleType(), 0.0);
        }
        return LLVMBuildLoad2(B, LLVMDoubleType(), a, n->name);
    }

    /* ---- unary negation ------------------------------------------- */
    case NODE_NEG: {
        LLVMValueRef v = gen_expr(B, n->left);
        return LLVMBuildFNeg(B, v, "fneg");
    }

    /* ---- explicit type cast --------------------------------------- */
    case NODE_CAST: {
        LLVMValueRef v = gen_expr(B, n->left);
        if (n->etype == TYPE_FLOAT) {
            /*
             * The child may already be a double (e.g. a variable load).
             * Check the LLVM type to avoid a no-op sitofp on a double.
             */
            if (LLVMTypeOf(v) == LLVMDoubleType())
                return v;                           /* already float */
            return LLVMBuildSIToFP(B, v, LLVMDoubleType(), "itof");
        } else {
            /* TYPE_INT: truncate double to i32, then widen back to double
             * so the rest of codegen stays uniform (double everywhere).  */
            LLVMValueRef i32v = LLVMBuildFPToSI(B, v, LLVMInt32Type(), "ftoi");
            return LLVMBuildSIToFP(B, i32v, LLVMDoubleType(), "itof2");
        }
    }

    /* ---- binary operations --------------------------------------- */
    case NODE_BINOP: {
        LLVMValueRef l = gen_expr(B, n->left);
        LLVMValueRef r = gen_expr(B, n->right);
        /*
         * Both operands are already double (gen_expr guarantees this).
         * Use floating-point LLVM instructions throughout.
         */
        switch (n->op) {
        case '+': return LLVMBuildFAdd(B, l, r, "fadd");
        case '-': return LLVMBuildFSub(B, l, r, "fsub");
        case '*': return LLVMBuildFMul(B, l, r, "fmul");
        case '/': return LLVMBuildFDiv(B, l, r, "fdiv");
        default:
            fprintf(stderr, "Error: unknown operator '%c'\n", n->op);
            codegen_errors++;
            return LLVMConstReal(LLVMDoubleType(), 0.0);
        }
    }

    default:
        fprintf(stderr, "Error: unexpected node kind %d in expression\n", n->kind);
        codegen_errors++;
        return LLVMConstReal(LLVMDoubleType(), 0.0);
    }
}

/* ------------------------------------------------------------------ */
/* gen_stmt                                                            */
/* ------------------------------------------------------------------ */
static void gen_stmt(LLVMBuilderRef B,
                     LLVMValueRef   printf_fn,
                     LLVMValueRef   fmt,
                     ASTNode       *n)
{
    if (!n) return;

    switch (n->kind) {
    case NODE_SEQ:
        gen_stmt(B, printf_fn, fmt, n->left);
        gen_stmt(B, printf_fn, fmt, n->right);
        break;

    case NODE_ASSIGN: {
        LLVMValueRef a = lookup(n->name);
        if (!a) a = declare(B, n->name);
        /*
         * gen_expr always returns double, which matches the alloca type,
         * so no extra cast is needed here.
         */
        LLVMValueRef val = gen_expr(B, n->right);
        LLVMBuildStore(B, val, a);
        break;
    }

    case NODE_PRINT: {
        /* Validate: if printing a bare variable it must be declared */
        if (n->left && n->left->kind == NODE_VAR && !lookup(n->left->name)) {
            fprintf(stderr, "Error: 'print' of undeclared variable '%s'\n",
                    n->left->name);
            codegen_errors++;
            return;
        }
        LLVMValueRef val = gen_expr(B, n->left);  /* always double */
        /*
         * printf("%%g\n", val)
         * %%g prints integers without a trailing ".0" and floats naturally.
         */
        LLVMTypeRef printf_type = LLVMFunctionType(
            LLVMInt32Type(),
            (LLVMTypeRef[]){ LLVMPointerType(LLVMInt8Type(), 0) },
            1, /*isVarArg=*/1);
        LLVMValueRef args[] = { fmt, val };
        LLVMBuildCall2(B, printf_type, printf_fn, args, 2, "");
        break;
    }

    default:
        fprintf(stderr, "Error: unexpected statement node kind %d\n", n->kind);
        codegen_errors++;
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                  */
/* ------------------------------------------------------------------ */
void codegen(ASTNode *root) {
    if (!root) {
        fprintf(stderr, "Error: codegen called with NULL root\n");
        exit(1);
    }

    LLVMModuleRef  M = LLVMModuleCreateWithName("myprog");
    LLVMBuilderRef B = LLVMCreateBuilder();

    /* Declare printf (varargs, returns i32) */
    LLVMTypeRef printf_t = LLVMFunctionType(
        LLVMInt32Type(),
        (LLVMTypeRef[]){ LLVMPointerType(LLVMInt8Type(), 0) },
        1, /*isVarArg=*/1);
    LLVMValueRef printf_fn = LLVMAddFunction(M, "printf", printf_t);

    /* main() -> i32 */
    LLVMTypeRef  main_t  = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_fn = LLVMAddFunction(M, "main", main_t);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_fn, "entry");
    LLVMPositionBuilderAtEnd(B, entry);

    /*
     * Use "%g\n": prints doubles without trailing zeros for whole numbers
     * (e.g. 7 -> "7", 3.14 -> "3.14").
     */
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(B, "%g\n", "fmt");

    gen_stmt(B, printf_fn, fmt, root);
    LLVMBuildRet(B, LLVMConstInt(LLVMInt32Type(), 0, 0));

    /* ---- error gate ---------------------------------------------- */
    if (codegen_errors > 0) {
        fprintf(stderr, "Error: %d codegen error(s); aborting IR emit.\n",
                codegen_errors);
        LLVMDisposeBuilder(B);
        LLVMDisposeModule(M);
        exit(1);
    }

    /* ---- LLVM module verification --------------------------------- */
    char *err = NULL;
    if (LLVMVerifyModule(M, LLVMReturnStatusAction, &err)) {
        fprintf(stderr, "Error: LLVM module verification failed: %s\n", err);
        LLVMDisposeMessage(err);
        LLVMDisposeBuilder(B);
        LLVMDisposeModule(M);
        exit(1);
    }
    LLVMDisposeMessage(err);

    /* ---- emit IR -------------------------------------------------- */
    char *err2 = NULL;
    if (LLVMPrintModuleToFile(M, "out.ll", &err2)) {
        fprintf(stderr, "Error: could not write IR: %s\n", err2);
        LLVMDisposeMessage(err2);
        LLVMDisposeBuilder(B);
        LLVMDisposeModule(M);
        exit(1);
    }
    LLVMDisposeMessage(err2);

    fprintf(stderr, "LLVM IR written to out.ll\n");
    LLVMDisposeBuilder(B);
    LLVMDisposeModule(M);
}
