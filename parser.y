%{
#include<stdio.h>
#include<stdlib.h>
int yylex();
%}


%token NUM VARIABLE PRINT
%left '+' '-'
%left '*' '/'
%%
S :
    S stmt
  | /* empty */
  ;
stmt:P A ';' 
| B ';';
P:PRINT;
A:VARIABLE;
B: A '=' C ;
C :
    C '+' C
  | C '-' C
  | C '*' C
  | C '/' C
  | '(' C ')'
  | NUM
  | VARIABLE;
%%
int main(){
return yyparse();
}
void yyerror(char *s){ 
printf("Invalid expression\n"); 
} 