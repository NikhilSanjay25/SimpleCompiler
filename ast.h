// ast.h
#ifndef AST_H
#define AST_H

typedef enum {
    NODE_NUM,
    NODE_VAR,
    NODE_ASSIGN,
    NODE_BINOP,
    NODE_PRINT,
    NODE_SEQ,
} NodeKind;

typedef struct ASTNode {
    NodeKind kind;
    int       ival;          // NODE_NUM: the integer value
    char      name[64];      // NODE_VAR / NODE_ASSIGN: variable name
    char      op;            // NODE_BINOP: '+' '-' '*' '/'
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

ASTNode *make_num(int v);
ASTNode *make_var(char *name);
ASTNode *make_assign(char *name, ASTNode *val);
ASTNode *make_binop(char op, ASTNode *l, ASTNode *r);
ASTNode *make_print(ASTNode *expr);
ASTNode *make_seq(ASTNode *first, ASTNode *second);

#endif
