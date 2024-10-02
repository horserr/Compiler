%{
#include <stdio.h>
#include "syntax.tab.h"
%}
%option yylineno
letter      [a-zA-Z]
digit       [0-9]
alnum       [0-9a-zA-Z]
whitespace  [ \t\r\n]+
%%
";"     { return SEMI; }
","     { return COMMA; }
"="     { return ASSIGNOP; }
">"|"<"|">="|"<="|"=="|"!=" { return RELOP; }
"+"     { return PLUS; }
"-"     { return MINUS; }
"*"     { return STAR; }
"/"     { return DIV; }
"&&"    { return AND; }
"||"    { return OR; }
"."     { return DOT; }
"!"     { return NOT; }
"("     { return LP; }
")"     { return RP; }
"["     { return LB; }
"]"     { return RB; }
"{"     { return LC; }
"}"     { return RC; }
"int"|"float" { return TYPE; }
"struct" { return STRUCT; }
"return" { return RETURN; }
"if"    { return IF; }
"else"  { return ELSE; }
"while" { return WHILE; }

(_|{letter})(_|{alnum})*   { return ID; }
{digit}+\.{digit}+         { return FLOAT; }
0|([1-9]{digit}*)          {
                        yylval.type_int = atoi(yytext);
                        return INT;
                    }
0[0-7]+                    { return INT; }
0[xX][0-9a-fA-F]+          { return INT; }

{whitespace} { /* Ignore whitespace */ }

.       { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }
%%