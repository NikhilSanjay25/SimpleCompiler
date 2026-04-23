%{
#include "ast.h"
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
%}

%%
print      { return PRINT; }
[0-9]+\.[0-9]* { yylval.fval = atof(yytext); return FNUM; }
[0-9]+     { yylval.ival = atoi(yytext); return NUM; }
"+"        { return '+'; }
"*"        { return '*'; }
"/"        { return '/'; }
"-"        { return '-'; }
"="        { return '='; }
"("        { return '('; }
";"        { return ';'; }
")"        { return ')'; }
[a-zA-Z][a-zA-Z0-9]* { strncpy(yylval.sval, yytext, 63); return VARIABLE; }
[ \t\n]    ;
.          { fprintf(stderr, "Error: unknown character '%s'\n", yytext); exit(1); }
%%
int yywrap() { return 1; }
