%{
#include <stdio.h>
#include <stdlib.h>
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
"struct" { return STRUCT; }
"return" { return RETURN; }
"if"    { return IF; }
"else"  { return ELSE; }
"while" { return WHILE; }

"int"|"float" {
  ValueUnion value = {.identifier = yytext};
  yylval.node = createASTNodeWithValue("TYPE", yylineno, value);
  return TYPE;
}
(_|{letter})(_|{alnum})*  {
  ValueUnion value = {.identifier = yytext};
  yylval.node = createASTNodeWithValue("ID", yylineno, value);
  return ID;
}
{digit}+\.{digit}+  {
  ValueUnion value = {.float_value = strtof(yytext, NULL)};
  yylval.node = createASTNodeWithValue("FLOAT", yylineno, value);
  return FLOAT;
}
0|([1-9]{digit}*) {
  ValueUnion value = {.int_value = atoi(yytext)};
  yylval.node = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}
0[0-7]+ {
  ValueUnion value = {.int_value = strtol(yytext, NULL, 8)};
  yylval.node = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}
0[xX][0-9a-fA-F]+ {
  ValueUnion value = {.int_value = strtol(yytext, NULL, 16)};
  yylval.node = createASTNodeWithValue("INT", yylineno, value);
  return INT;
}

{whitespace}  { /* Ignore whitespace */ }

.       { printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext); }
%%