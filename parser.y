%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
int yylex();
void yyerror(const char *s);
void codegen(ASTNode *root);
ASTNode *root;
static int parse_ok = 1;   /* cleared on any yyerror */

/*
 * coerce_to_float: wrap a node in a CAST(->float) node only when it is
 * a known integer literal or an integer-typed node.  This is called
 * when we detect a mixed int/float binary operation so that codegen
 * always sees a uniform type on both sides.
 */
static ASTNode *coerce_to_float(ASTNode *n) {
    if (!n) return n;
    if (n->kind == NODE_NUM)    return make_float((double)n->ival);
    if (n->etype == TYPE_FLOAT) return n;          /* already float */
    return make_cast(n, TYPE_FLOAT);
}

/*
 * make_binop_typed: build a binop, promoting operands to float when
 * either side is float so that the tree is always type-consistent.
 */
static ASTNode *make_binop_typed(char op, ASTNode *l, ASTNode *r) {
    int lf = (l && (l->kind == NODE_FLOAT || l->etype == TYPE_FLOAT));
    int rf = (r && (r->kind == NODE_FLOAT || r->etype == TYPE_FLOAT));
    if (lf && !rf) r = coerce_to_float(r);
    if (rf && !lf) l = coerce_to_float(l);
    ASTNode *n = make_binop(op, l, r);
    n->etype = (lf || rf) ? TYPE_FLOAT : TYPE_INT;
    return n;
}
%}

%union {
    int      ival;
    double   fval;
    char     sval[64];
    ASTNode *node;
}

%token <ival> NUM
%token <fval> FNUM
%token <sval> VARIABLE
%token PRINT

%type <node> stmt stmtlist expr term factor

%left '+' '-'
%left '*' '/'
%right UMINUS

%%
program : stmtlist          { root = $1; }
        ;

stmtlist: stmtlist stmt     { $$ = make_seq($1, $2); }
        | stmt              { $$ = $1; }
        ;

stmt : PRINT expr ';'          { $$ = make_print($2); }
     | VARIABLE '=' expr ';'   { $$ = make_assign($1, $3); }
     | error ';'               { $$ = NULL; yyerrok; }
     ;

expr : expr '+' term        { $$ = make_binop_typed('+', $1, $3); }
     | expr '-' term        { $$ = make_binop_typed('-', $1, $3); }
     | term                 { $$ = $1; }
     ;

term : term '*' factor      { $$ = make_binop_typed('*', $1, $3); }
     | term '/' factor      {
         /* compile-time division-by-zero check for constant divisors */
         if ($3 && $3->kind == NODE_NUM   && $3->ival == 0) {
             fprintf(stderr, "Error: integer division by zero\n");
             parse_ok = 0;
         }
         if ($3 && $3->kind == NODE_FLOAT && $3->fval == 0.0) {
             fprintf(stderr, "Warning: floating-point division by zero (will produce Inf/NaN)\n");
         }
         $$ = make_binop_typed('/', $1, $3);
       }
     | factor               { $$ = $1; }
     ;

factor : '(' expr ')'            { $$ = $2; }
       | '-' factor %prec UMINUS { $$ = make_neg($2); }
       | NUM                     { $$ = make_num($1); }
       | FNUM                    { $$ = make_float($1); }
       | VARIABLE                { $$ = make_var($1); }
       ;
%%

int main(void) {
    yyparse();
    if (!parse_ok || !root) {
        fprintf(stderr, "Error: compilation aborted due to parse errors.\n");
        return 1;
    }
    fprintf(stderr, "=== AST ===\n");
    print_ast(root, 0);
    fprintf(stderr, "===========\n");
    codegen(root);
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
    parse_ok = 0;
}
