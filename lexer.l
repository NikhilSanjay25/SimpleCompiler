%{
#include "y.tab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
%}

%%
print {return PRINT;}
[0-9]+ {
    yylval=atoi(yytext);
    return NUM;
    }
"+" {return '+';}
"*" {return '*';}
"/" {return '/';}
"-" {return '-';}
"=" {return '=';}
"(" {return '(';}
";" {return ';';}
[a-zA-Z]+ { return VARIABLE;}
[ \t\n] ;
. {printf("Unknow character: %s\n",yytext);}
%%
int yywrap(){
return 1;
}

