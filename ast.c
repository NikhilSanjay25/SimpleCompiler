#include <stdlib.h>
#include <string.h>
#include "ast.h"

static ASTNode *node(NodeKind k) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = k;
    return n;
}
ASTNode *make_num(int v)              { ASTNode *n=node(NODE_NUM);    n->ival=v; return n; }
ASTNode *make_var(char *s)            { ASTNode *n=node(NODE_VAR);    strncpy(n->name,s,63); return n; }
ASTNode *make_assign(char *s, ASTNode *v){ ASTNode *n=node(NODE_ASSIGN); strncpy(n->name,s,63); n->right=v; return n; }
ASTNode *make_binop(char op,ASTNode *l,ASTNode *r){ ASTNode *n=node(NODE_BINOP); n->op=op; n->left=l; n->right=r; return n; }
ASTNode *make_print(ASTNode *e)       { ASTNode *n=node(NODE_PRINT);  n->left=e; return n; }
ASTNode *make_seq(ASTNode *a,ASTNode *b){ ASTNode *n=node(NODE_SEQ); n->left=a; n->right=b; return n; }
