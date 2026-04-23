// ast.h
#ifndef AST_H
#define AST_H

/* Static type of an expression node (resolved during codegen) */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
} ExprType;

typedef enum {
    NODE_NUM,
    NODE_FLOAT,    /* floating-point literal */
    NODE_VAR,
    NODE_ASSIGN,
    NODE_BINOP,
    NODE_NEG,
    NODE_CAST,     /* explicit int->float or float->int widening/narrowing */
    NODE_PRINT,
    NODE_SEQ,
} NodeKind;

typedef struct ASTNode {
    NodeKind  kind;
    ExprType  etype;         /* inferred type (filled in by codegen) */
    int       ival;          /* NODE_NUM:   integer value              */
    double    fval;          /* NODE_FLOAT: floating-point value       */
    char      name[64];      /* NODE_VAR / NODE_ASSIGN: variable name  */
    char      op;            /* NODE_BINOP: '+' '-' '*' '/'            */
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

ASTNode *make_num(int v);
ASTNode *make_float(double v);
ASTNode *make_var(char *name);
ASTNode *make_assign(char *name, ASTNode *val);
ASTNode *make_binop(char op, ASTNode *l, ASTNode *r);
ASTNode *make_neg(ASTNode *operand);
ASTNode *make_cast(ASTNode *operand, ExprType to);
ASTNode *make_print(ASTNode *expr);
ASTNode *make_seq(ASTNode *first, ASTNode *second);

void print_ast(ASTNode *n, int indent);

#endif
