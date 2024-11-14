%code requires{
#ifdef LOCAL
  #include "ParseTree/ParseTree.h"
#else
  #include "ParseTree.h"
#endif
}
%{
  #ifdef LOCAL
    #include "lex.yy.h"
    #include "ParseTree/ParseTree.h"
  #else
    #include "lex.yy.c"
  #endif

  #ifdef PARSER_DEBUG
    #undef  YYDEBUG
    #define YYDEBUG 1
  #endif

  void yyerror(char* msg);
  extern ParseTNode *root; // Root of the AST
%}

%define api.value.type {ParseTNode*}
%locations

/* declared tokens */
/* %token <node> INT FLOAT TYPE ID */
%token INT FLOAT TYPE ID

%token SEMI COMMA ASSIGNOP
%token RELOP DOT
%token PLUS MINUS STAR DIV
%token AND OR NOT
%token LP RP LB RB LC RC

%token STRUCT RETURN IF ELSE WHILE

/* declared operators and precedence */
%right      ASSIGNOP
%left       OR
%left       AND
%left       RELOP
%left       PLUS MINUS
%left       STAR DIV
%right      NOT UMINUS        /* unary minus (negation) */
%left       LP RP LB RB DOT
%nonassoc   INFERIOR_ELSE
%nonassoc   ELSE

%%
/* High-level Definitions */
Program : ExtDefList {
            root = createParseTNode("Program", @$.first_line, 0);
            addChild(root, $1);
        }
        ;

ExtDefList : /* empty */ {
                $$ = createParseTNode("ExtDefList", @$.first_line, 0);
            }
           | ExtDef ExtDefList {
                $$ = createParseTNode("ExtDefList", @$.first_line, 0);
                addChild($$, $1);
                addChild($$, $2);
            }
           ;

ExtDef : Specifier ExtDecList SEMI {
            $$ = createParseTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createParseTNode("SEMI", @3.first_line, 1));
        }
       | Specifier SEMI {
            $$ = createParseTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("SEMI", @2.first_line, 1));
        }
       | Specifier FunDec CompSt {
            $$ = createParseTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, $3);
        }
       ;

ExtDecList : VarDec {
                $$ = createParseTNode("ExtDecList", @$.first_line, 0);
                addChild($$, $1);
            }
           | VarDec COMMA ExtDecList {
                $$ = createParseTNode("ExtDecList", @$.first_line, 0);
                addChild($$, $1);
                addChild($$, createParseTNode("COMMA", @2.first_line, 1));
                addChild($$, $3);
            }
           ;

/* Specifiers */
Specifier : TYPE {
                $$ = createParseTNode("Specifier", @$.first_line, 0);
                addChild($$, $1);
            }
          | StructSpecifier {
                $$ = createParseTNode("Specifier", @$.first_line, 0);
                addChild($$, $1);
            }
          ;

StructSpecifier : STRUCT OptTag LC DefList RC {
                    $$ = createParseTNode("StructSpecifier", @$.first_line, 0);
                    addChild($$, createParseTNode("STRUCT", @1.first_line, 1));
                    addChild($$, $2);
                    addChild($$, createParseTNode("LC", @3.first_line, 1));
                    addChild($$, $4);
                    addChild($$, createParseTNode("RC", @5.first_line, 1));
                }
                | STRUCT Tag {
                    $$ = createParseTNode("StructSpecifier", @$.first_line, 0);
                    addChild($$, createParseTNode("STRUCT", @1.first_line, 1));
                    addChild($$, $2);
                }
                ;

OptTag : /* empty */ {
            $$ = createParseTNode("OptTag", @$.first_line, 0);
        }
       | ID {
            $$ = createParseTNode("OptTag", @$.first_line, 0);
            addChild($$, $1);
        }
       ;

Tag : ID {
        $$ = createParseTNode("Tag", @$.first_line, 0);
        addChild($$, $1);
    }
    ;

/* Declarators */
VarDec : ID {
            $$ = createParseTNode("VarDec", @$.first_line, 0);
            addChild($$, $1);
        }
       | VarDec LB INT RB {
            $$ = createParseTNode("VarDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LB", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RB", @4.first_line, 1));
        }
       ;

FunDec : ID LP VarList RP {
            $$ = createParseTNode("FunDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RP", @4.first_line, 1));
        }
       | ID LP RP {
            $$ = createParseTNode("FunDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, createParseTNode("RP", @3.first_line, 1));
        }
       ;

VarList : ParamDec COMMA VarList {
            $$ = createParseTNode("VarList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
     }
        | ParamDec {
            $$ = createParseTNode("VarList", @$.first_line, 0);
            addChild($$, $1);
        }
        ;

ParamDec : Specifier VarDec {
            $$ = createParseTNode("ParamDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
        ;

/* Statements */
/* variables can only be declared at the beginning of CompSt */
CompSt : LC DefList StmtList RC {
            $$ = createParseTNode("CompSt", @$.first_line, 0);
            addChild($$, createParseTNode("LC", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, $3);
            addChild($$, createParseTNode("RC", @4.first_line, 1));
        }
       ;

StmtList : /* empty */ {
            $$ = createParseTNode("StmtList", @$.first_line, 0);
        }
         | Stmt StmtList {
            $$ = createParseTNode("StmtList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
         ;

Stmt : Exp SEMI {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("SEMI", @2.first_line, 1));
        }
     | CompSt {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, $1);
        }
     | RETURN Exp SEMI {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, createParseTNode("RETURN", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, createParseTNode("SEMI", @3.first_line, 1));
        }
     | IF LP Exp RP Stmt %prec INFERIOR_ELSE {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, createParseTNode("IF", @1.first_line, 1));
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RP", @4.first_line, 1));
            addChild($$, $5);
        }
     | IF LP Exp RP Stmt ELSE Stmt {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, createParseTNode("IF", @1.first_line, 1));
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RP", @4.first_line, 1));
            addChild($$, $5);
            addChild($$, createParseTNode("ELSE", @6.first_line, 1));
            addChild($$, $7);
        }
     | WHILE LP Exp RP Stmt {
            $$ = createParseTNode("Stmt", @$.first_line, 0);
            addChild($$, createParseTNode("WHILE", @1.first_line, 1));
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RP", @4.first_line, 1));
            addChild($$, $5);
        }
     ;

/* Local Definitions */
DefList : /* empty */ {
            $$ = createParseTNode("DefList", @$.first_line, 0);
        }
        | Def DefList {
            $$ = createParseTNode("DefList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
        ;

Def : Specifier DecList SEMI {
            $$ = createParseTNode("Def", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createParseTNode("SEMI", @3.first_line, 1));
        }
    ;

DecList : Dec {
            $$ = createParseTNode("DecList", @$.first_line, 0);
            addChild($$, $1);
        }
        | Dec COMMA DecList {
            $$ = createParseTNode("DecList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
        }
        ;

Dec : VarDec {
            $$ = createParseTNode("Dec", @$.first_line, 0);
            addChild($$, $1);
        }
    | VarDec ASSIGNOP Exp {
            $$ = createParseTNode("Dec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("ASSIGNOP", @2.first_line, 1));
            addChild($$, $3);
        }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("ASSIGNOP", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp AND Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("AND", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp OR Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("OR", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp RELOP Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("RELOP", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp PLUS Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("PLUS", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp MINUS Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("MINUS", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp STAR Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("STAR", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp DIV Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("DIV", @2.first_line, 1));
            addChild($$, $3);
        }
    | LP Exp RP {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, createParseTNode("LP", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, createParseTNode("RP", @3.first_line, 1));
        }
    | MINUS Exp %prec UMINUS {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, createParseTNode("MINUS", @1.first_line, 1));
            addChild($$, $2);
        }
    | NOT Exp {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, createParseTNode("NOT", @1.first_line, 1));
            addChild($$, $2);
        }
    | ID LP Args RP {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RP", @4.first_line, 1));
        }
    | ID LP RP {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LP", @2.first_line, 1));
            addChild($$, createParseTNode("RP", @3.first_line, 1));
        }
    | Exp LB Exp RB {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("LB", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createParseTNode("RB", @4.first_line, 1));
        }
    | Exp DOT ID {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("DOT", @2.first_line, 1));
            addChild($$, $3);
        }
    | ID {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    | INT {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    | FLOAT {
            $$ = createParseTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    ;

Args : Exp COMMA Args {
            $$ = createParseTNode("Args", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createParseTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
        }
     | Exp {
            $$ = createParseTNode("Args", @$.first_line, 0);
            addChild($$, $1);
        }
     ;

%%
void yyerror(char* msg) {
    fprintf(stdout, "Error type B at Line %d: %s around \"%s\".\n", yylineno, msg, yytext);
}
