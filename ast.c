#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

static ASTNode *node(NodeKind k) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = k;
    return n;
}
ASTNode *make_num(int v)               { ASTNode *n=node(NODE_NUM);    n->ival=v; n->etype=TYPE_INT;   return n; }
ASTNode *make_float(double v)          { ASTNode *n=node(NODE_FLOAT);  n->fval=v; n->etype=TYPE_FLOAT; return n; }
ASTNode *make_var(char *s)             { ASTNode *n=node(NODE_VAR);    strncpy(n->name,s,63); return n; }
ASTNode *make_assign(char *s, ASTNode *v){ ASTNode *n=node(NODE_ASSIGN); strncpy(n->name,s,63); n->right=v; return n; }
ASTNode *make_binop(char op,ASTNode *l,ASTNode *r){ ASTNode *n=node(NODE_BINOP); n->op=op; n->left=l; n->right=r; return n; }
ASTNode *make_neg(ASTNode *operand)    { ASTNode *n=node(NODE_NEG);    n->left=operand; return n; }
ASTNode *make_cast(ASTNode *operand, ExprType to) {
    ASTNode *n = node(NODE_CAST);
    n->left  = operand;
    n->etype = to;
    return n;
}
ASTNode *make_print(ASTNode *e)        { ASTNode *n=node(NODE_PRINT);  n->left=e; return n; }
ASTNode *make_seq(ASTNode *a,ASTNode *b){ ASTNode *n=node(NODE_SEQ);   n->left=a; n->right=b; return n; }

/* ------------------------------------------------------------------ */
/* print_ast_r: recursive tree printer with box-drawing lines.         */
/* ------------------------------------------------------------------ */
static void print_ast_r(ASTNode *n, const char *prefix, int is_last) {
    if (!n) return;

    const char *branch = is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "
                                 : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";
    char child_prefix[256];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix,
             is_last ? "    " : "\xe2\x94\x82   ");

    fprintf(stderr, "%s%s", prefix, branch);

    switch (n->kind) {
    case NODE_NUM:
        fprintf(stderr, "NUM(%d)\n", n->ival);
        break;
    case NODE_FLOAT:
        fprintf(stderr, "FLOAT(%g)\n", n->fval);
        break;
    case NODE_VAR:
        fprintf(stderr, "VAR(%s)\n", n->name);
        break;
    case NODE_NEG:
        fprintf(stderr, "NEG\n");
        print_ast_r(n->left, child_prefix, 1);
        break;
    case NODE_CAST:
        fprintf(stderr, "CAST(->%s)\n", n->etype == TYPE_FLOAT ? "float" : "int");
        print_ast_r(n->left, child_prefix, 1);
        break;
    case NODE_ASSIGN:
        fprintf(stderr, "ASSIGN(%s)\n", n->name);
        print_ast_r(n->right, child_prefix, 1);
        break;
    case NODE_BINOP:
        fprintf(stderr, "BINOP(%c)\n", n->op);
        print_ast_r(n->left,  child_prefix, 0);
        print_ast_r(n->right, child_prefix, 1);
        break;
    case NODE_PRINT:
        fprintf(stderr, "PRINT\n");
        print_ast_r(n->left, child_prefix, 1);
        break;
    case NODE_SEQ:
        fprintf(stderr, "SEQ\n");
        print_ast_r(n->left,  child_prefix, 0);
        print_ast_r(n->right, child_prefix, 1);
        break;
    default:
        fprintf(stderr, "???\n");
        break;
    }
}

void print_ast(ASTNode *n, int indent) {
    if (!n) return;
    char prefix[256] = "";
    for (int i = 0; i < indent; i++)
        strncat(prefix, "    ", sizeof(prefix) - strlen(prefix) - 1);
    switch (n->kind) {
    case NODE_NUM:    fprintf(stderr, "%sNUM(%d)\n",    prefix, n->ival); break;
    case NODE_FLOAT:  fprintf(stderr, "%sFLOAT(%g)\n",  prefix, n->fval); break;
    case NODE_VAR:    fprintf(stderr, "%sVAR(%s)\n",    prefix, n->name); break;
    case NODE_CAST:
        fprintf(stderr, "%sCAST(->%s)\n", prefix, n->etype == TYPE_FLOAT ? "float" : "int");
        print_ast_r(n->left, prefix, 1);
        break;
    case NODE_NEG:
        fprintf(stderr, "%sNEG\n", prefix);
        print_ast_r(n->left, prefix, 1);
        break;
    case NODE_ASSIGN:
        fprintf(stderr, "%sASSIGN(%s)\n", prefix, n->name);
        print_ast_r(n->right, prefix, 1);
        break;
    case NODE_BINOP:
        fprintf(stderr, "%sBINOP(%c)\n", prefix, n->op);
        print_ast_r(n->left,  prefix, 0);
        print_ast_r(n->right, prefix, 1);
        break;
    case NODE_PRINT:
        fprintf(stderr, "%sPRINT\n", prefix);
        print_ast_r(n->left, prefix, 1);
        break;
    case NODE_SEQ:
        fprintf(stderr, "%sSEQ\n", prefix);
        print_ast_r(n->left,  prefix, 0);
        print_ast_r(n->right, prefix, 1);
        break;
    default:
        fprintf(stderr, "%s???\n", prefix);
        break;
    }
}



