%{
#include <stdio.h>
#include <stdlib.h>
#include "syntax.tab.h"

int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno
letter      [a-zA-Z]
digit       [0-9]
alnum       [0-9a-zA-Z]
whitespace  [ \t\r]+

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
"struct" { return STRUCT; }
"return" { return RETURN; }
"if"    { return IF; }
"else"  { return ELSE; }
"while" { return WHILE; }

"int"|"float" {
  ValueUnion value = {.identifier = yytext};
  yylval = createASTNodeWithValue("TYPE", yylineno, value);
  return TYPE;
}
(_|{letter})(_|{alnum})*  {
  ValueUnion value = {.identifier = yytext};
  yylval = createASTNodeWithValue("ID", yylineno, value);
  return ID;
}
{digit}+\.{digit}+  {
  ValueUnion value = {.float_value = strtof(yytext, NULL)};
  yylval = createASTNodeWithValue("FLOAT", yylineno, value);
  return FLOAT;
}
0|([1-9]{digit}*) {
  ValueUnion value = {.int_value = atoi(yytext)};
  yylval = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}
0[0-7]+ {
  ValueUnion value = {.int_value = strtol(yytext, NULL, 8)};
  yylval = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}
0[xX][0-9a-fA-F]+ {
  ValueUnion value = {.int_value = strtol(yytext, NULL, 16)};
  yylval = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}

\n    { yycolumn = 1; }
{whitespace}  { /* Ignore whitespace */ }

.       { printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext); }
%%