%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
int yylex();
void yyerror(char *s);
void codegen(ASTNode *root);
ASTNode *root;   /* the final tree lives here */
%}

%union {
    int      ival;
    char     sval[64];
    ASTNode *node;
}

%token <ival> NUM
%token <sval> VARIABLE
%token PRINT

%type <node> stmt stmtlist expr term factor

%left '+' '-'
%left '*' '/'

%%
program : stmtlist          { root = $1; }
        ;

stmtlist: stmtlist stmt     { $$ = make_seq($1, $2); }
        | stmt              { $$ = $1; }
        ;

stmt : PRINT VARIABLE ';'   { $$ = make_print(make_var($2)); }
     | VARIABLE '=' expr ';'{ $$ = make_assign($1, $3); }
     ;
expr : expr '+' term        { $$ = make_binop('+', $1, $3); }
     | expr '-' term        { $$ = make_binop('-', $1, $3); }
     | term                 { $$ = $1; }
     ;

term : term '*' factor      { $$ = make_binop('*', $1, $3); }
     | term '/' factor      { $$ = make_binop('/', $1, $3); }
     | factor               { $$ = $1; }
     ;

factor : '(' expr ')'      { $$ = $2; }
       | NUM                { $$ = make_num($1); }
       | VARIABLE           { $$ = make_var($1); }
       ;
%%
int main() {
    yyparse();
    /* pass root to codegen next */
    codegen(root);
    return 0;
}
void yyerror(char *s) { printf("Parse error: %s\n", s); }
