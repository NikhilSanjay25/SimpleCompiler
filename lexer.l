%%
print printf("Detected Print Statement %s\n",yytext);
[+|*|/|-] printf("Detected operator Statement %s\n",yytext);
= printf("Detected assignment Statement %s\n",yytext);
[a-zA-Z]+ printf("Detected a variable %s\n",yytext);
%%
int yywrap(){
return 1;
}

int main(){
yylex();
return 0;
}
