// codegen.c
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

/* simple symbol table: name → alloca'd variable */
#define MAX_VARS 64
static struct { char name[64]; LLVMValueRef alloca; } vars[MAX_VARS];
static int nv = 0;

static LLVMValueRef lookup(const char *name) {
    for (int i = 0; i < nv; i++)
        if (!strcmp(vars[i].name, name)) return vars[i].alloca;
    return NULL;
}
static LLVMValueRef declare(LLVMBuilderRef B, const char *name) {
    LLVMValueRef a = LLVMBuildAlloca(B, LLVMInt32Type(), name);
    strncpy(vars[nv].name, name, 63);
    vars[nv++].alloca = a;
    return a;
}

static LLVMValueRef gen_expr(LLVMBuilderRef B, ASTNode *n) {
    if (!n) return NULL;
    switch (n->kind) {
    case NODE_NUM:
        return LLVMConstInt(LLVMInt32Type(), n->ival, 0);
    case NODE_VAR: {
        LLVMValueRef a = lookup(n->name);
        return a ? LLVMBuildLoad2(B, LLVMInt32Type(), a, n->name) 
                 : LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
    case NODE_BINOP: {
        LLVMValueRef l = gen_expr(B, n->left);
        LLVMValueRef r = gen_expr(B, n->right);
        switch (n->op) {
        case '+': return LLVMBuildAdd(B, l, r, "add");
        case '-': return LLVMBuildSub(B, l, r, "sub");
        case '*': return LLVMBuildMul(B, l, r, "mul");
        case '/': return LLVMBuildSDiv(B, l, r, "div");
        }
    }
    default: return LLVMConstInt(LLVMInt32Type(), 0, 0);
    }
}

static void gen_stmt(LLVMBuilderRef B, LLVMValueRef printf_fn, LLVMValueRef fmt, ASTNode *n) {
    if (!n) return;
    switch (n->kind) {
    case NODE_SEQ:
        gen_stmt(B, printf_fn, fmt, n->left);
        gen_stmt(B, printf_fn, fmt, n->right);
        break;
    case NODE_ASSIGN: {
        LLVMValueRef a = lookup(n->name);
        if (!a) a = declare(B, n->name);
        LLVMBuildStore(B, gen_expr(B, n->right), a);
        break;
    }
    case NODE_PRINT: {
        LLVMValueRef val = gen_expr(B, n->left);
        LLVMValueRef args[] = { fmt, val };
        LLVMBuildCall2(B,
            LLVMFunctionType(LLVMInt32Type(),
                (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(),0), LLVMInt32Type()}, 2, 1),
            printf_fn, args, 2, "");
        break;
    }
    default: break;
    }
}

void codegen(ASTNode *root) {
    LLVMModuleRef  M = LLVMModuleCreateWithName("myprog");
    LLVMBuilderRef B = LLVMCreateBuilder();

    /* declare printf */
    LLVMTypeRef  printf_t = LLVMFunctionType(LLVMInt32Type(),
        (LLVMTypeRef[]){LLVMPointerType(LLVMInt8Type(), 0)}, 1, 1);
    LLVMValueRef printf_fn = LLVMAddFunction(M, "printf", printf_t);

    /* main function */
    LLVMTypeRef  main_t  = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_fn = LLVMAddFunction(M, "main", main_t);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_fn, "entry");
    LLVMPositionBuilderAtEnd(B, entry);

    /* format string for printf */
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(B, "%d\n", "fmt");

    gen_stmt(B, printf_fn, fmt, root);
    LLVMBuildRet(B, LLVMConstInt(LLVMInt32Type(), 0, 0));

    /* verify + emit IR */
	char *err = NULL;
        if (LLVMVerifyModule(M, LLVMReturnStatusAction, &err)) {
            fprintf(stderr, "Module error: %s\n", err);
            LLVMDisposeMessage(err);
        }

        char *err2 = NULL;
        if (LLVMPrintModuleToFile(M, "out.ll", &err2)) {
            fprintf(stderr, "Could not write IR: %s\n", err2);
            LLVMDisposeMessage(err2);
        }

	printf("LLVM IR written to out.ll\n");
	LLVMDisposeBuilder(B);
	LLVMDisposeModule(M);
}
