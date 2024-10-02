%{
#include <stdio.h>
%}
%option yylineno
letter      [a-zA-Z]
digit       [0-9]
alnum       [0-9a-zA-Z]
whitespace  [ \t\r\n]+
%%
";"     { printf("SEMI\n"); }
","     { printf("COMMA\n"); }
"="     { printf("ASSIGNOP\n"); }
">"|"<"|">="|"<="|"=="|"!=" { printf("RELOP\n"); }
"+"     { printf("PLUS\n"); }
"-"     { printf("MINUS\n"); }
"*"     { printf("STAR\n"); }
"/"     { printf("DIV\n"); }
"&&"    { printf("AND\n"); }
"||"    { printf("OR\n"); }
"."     { printf("DOT\n"); }
"!"     { printf("NOT\n"); }
"("     { printf("LP\n"); }
")"     { printf("RP\n"); }
"["     { printf("LB\n"); }
"]"     { printf("RB\n"); }
"{"     { printf("LC\n"); }
"}"     { printf("RC\n"); }
"int"|"float" { printf("TYPE\n"); }
"struct" { printf("STRUCT\n"); }
"return" { printf("RETURN\n"); }
"if"    { printf("IF\n"); }
"else"  { printf("ELSE\n"); }
"while" { printf("WHILE\n"); }


(_|{letter})(_|{alnum})*   { printf("ID\n"); }
{digit}+\.{digit}+         { printf("FLOAT\n"); }
0|([1-9]{digit}*)          { printf("decimal INT\n"); }
0[0-7]+                    { printf("octal INT\n"); }
0[xX][0-9a-fA-F]+          { printf("hexadecimal INT\n"); }

{whitespace} { /* Ignore whitespace */ }

.       { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }
%%