%{
  #include "lex.yy.c"
  #include "../AST.h"

  void yyerror(char* msg);

  #ifdef DEBUG
    #undef  YYDEBUG
    #define YYDEBUG 1
  #endif

  // todo use this
  extern const char * const yytname[];
  extern ASTNode *root; // Root of the AST
%}

/* declared types */
%union {
  ASTNode* node;
}

/* declared tokens */
%token <node> INT FLOAT TYPE ID

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

/* declared non-terminals */
%type <node> Program ExtDefList ExtDef Specifier ExtDecList VarDec FunDec CompSt StmtList Stmt DefList Def DecList Dec Exp Args StructSpecifier OptTag Tag VarList ParamDec

%%
/* High-level Definitions */
Program : ExtDefList {
            root = createASTNode("Program", yylineno, 0);
            addChild(root, $1);
        }
        | error {
            $$ = createASTNode("Program", yylineno, 0);
        }
        ;

ExtDefList : /* empty */ {
                $$ = createASTNode("ExtDefList", yylineno, 0);
            }
           | ExtDef ExtDefList {
                $$ = createASTNode("ExtDefList", yylineno, 0);
                addChild($$, $1);
                addChild($$, $2);
            }
           | error ExtDefList {
                $$ = createASTNode("ExtDefList", yylineno, 0);
            }
           ;

ExtDef : Specifier ExtDecList SEMI {
            $$ = createASTNode("ExtDef", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", yylineno, 1));
        }
       | Specifier SEMI {
            $$ = createASTNode("ExtDef", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("SEMI", yylineno, 1));
        }
       | Specifier FunDec CompSt {
            $$ = createASTNode("ExtDef", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, $3);
        }
       ;

ExtDecList : VarDec {
                $$ = createASTNode("ExtDecList", yylineno, 0);
                addChild($$, $1);
            }
           | VarDec COMMA ExtDecList {
                $$ = createASTNode("ExtDecList", yylineno, 0);
                addChild($$, $1);
                addChild($$, createASTNode("COMMA", yylineno, 1));
                addChild($$, $3);
            }
           ;

/* Specifiers */
Specifier : TYPE {
                $$ = createASTNode("Specifier", yylineno, 0);
                addChild($$, $1);
            }
          | StructSpecifier {
                $$ = createASTNode("Specifier", yylineno, 0);
                addChild($$, $1);
            }
          ;

StructSpecifier : STRUCT OptTag LC DefList RC {
                    $$ = createASTNode("StructSpecifier", yylineno, 0);
                    addChild($$, createASTNode("STRUCT", yylineno, 1));
                    addChild($$, $2);
                    addChild($$, createASTNode("LC", yylineno, 1));
                    addChild($$, $4);
                    addChild($$, createASTNode("RC", yylineno, 1));
                }
                | STRUCT Tag {
                    $$ = createASTNode("StructSpecifier", yylineno, 0);
                    addChild($$, createASTNode("STRUCT", yylineno, 1));
                    addChild($$, $2);
                }
                ;

OptTag : /* empty */ {
            $$ = createASTNode("OptTag", yylineno, 0);
        }
       | ID {
            $$ = createASTNode("OptTag", yylineno, 0);
            addChild($$, $1);
        }
       ;

Tag : ID {
        $$ = createASTNode("Tag", yylineno, 0);
        addChild($$, $1);
    }
    ;

/* Declarators */
VarDec : ID {
            $$ = createASTNode("VarDec", yylineno, 0);
            addChild($$, $1);
        }
       | VarDec LB INT RB {
            $$ = createASTNode("VarDec", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LB", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RB", yylineno, 1));
        }
       ;

FunDec : ID LP VarList RP {
            $$ = createASTNode("FunDec", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", yylineno, 1));
        }
       | ID LP RP {
            $$ = createASTNode("FunDec", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, createASTNode("RP", yylineno, 1));
        }
       ;

VarList : ParamDec COMMA VarList {
            $$ = createASTNode("VarList", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", yylineno, 1));
            addChild($$, $3);
     }
        | ParamDec {
            $$ = createASTNode("VarList", yylineno, 0);
            addChild($$, $1);
        }
        ;

ParamDec : Specifier VarDec {
            $$ = createASTNode("ParamDec", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
        ;

/* Statements */
/* variables can only be declared at the beginning of CompSt */
CompSt : LC DefList StmtList RC {
            $$ = createASTNode("CompSt", yylineno, 0);
            addChild($$, createASTNode("LC", yylineno, 1));
            addChild($$, $2);
            addChild($$, $3);
            addChild($$, createASTNode("RC", yylineno, 1));
        }
       ;

StmtList : /* empty */ {
            $$ = createASTNode("StmtList", yylineno, 0);
        }
         | Stmt StmtList {
            $$ = createASTNode("StmtList", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
         | error StmtList {
            $$ = createASTNode("StmtList", yylineno, 0);
        }
         ;

Stmt : Exp SEMI {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("SEMI", yylineno, 1));
        }
     | CompSt {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, $1);
        }
     | RETURN Exp SEMI {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, createASTNode("RETURN", yylineno, 1));
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", yylineno, 1));
        }
     | IF LP Exp RP Stmt %prec INFERIOR_ELSE {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, createASTNode("IF", yylineno, 1));
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", yylineno, 1));
            addChild($$, $5);
        }
     | IF LP Exp RP Stmt ELSE Stmt {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, createASTNode("IF", yylineno, 1));
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", yylineno, 1));
            addChild($$, $5);
            addChild($$, createASTNode("ELSE", yylineno, 1));
            addChild($$, $7);
        }
     | WHILE LP Exp RP Stmt {
            $$ = createASTNode("Stmt", yylineno, 0);
            addChild($$, createASTNode("WHILE", yylineno, 1));
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", yylineno, 1));
            addChild($$, $5);
        }
     ;

/* Local Definitions */
DefList : /* empty */ {
            $$ = createASTNode("DefList", yylineno, 0);
        }
        | Def DefList {
            $$ = createASTNode("DefList", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
        | error DefList {
            $$ = createASTNode("DefList", yylineno, 0);
        }
        ;

Def : Specifier DecList SEMI {
            $$ = createASTNode("Def", yylineno, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", yylineno, 1));
        }
    ;

DecList : Dec {
            $$ = createASTNode("DecList", yylineno, 0);
            addChild($$, $1);
        }
        | Dec COMMA DecList {
            $$ = createASTNode("DecList", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", yylineno, 1));
            addChild($$, $3);
        }
        ;

Dec : VarDec {
            $$ = createASTNode("Dec", yylineno, 0);
            addChild($$, $1);
        }
    | VarDec ASSIGNOP Exp {
            $$ = createASTNode("Dec", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("ASSIGNOP", yylineno, 1));
            addChild($$, $3);
        }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("ASSIGNOP", yylineno, 1));
            addChild($$, $3);
        }
    | Exp AND Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("AND", yylineno, 1));
            addChild($$, $3);
        }
    | Exp OR Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("OR", yylineno, 1));
            addChild($$, $3);
        }
    | Exp RELOP Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("RELOP", yylineno, 1));
            addChild($$, $3);
        }
    | Exp PLUS Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("PLUS", yylineno, 1));
            addChild($$, $3);
        }
    | Exp MINUS Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("MINUS", yylineno, 1));
            addChild($$, $3);
        }
    | Exp STAR Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("STAR", yylineno, 1));
            addChild($$, $3);
        }
    | Exp DIV Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("DIV", yylineno, 1));
            addChild($$, $3);
        }
    | LP Exp RP {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $2);
            addChild($$, createASTNode("RP", yylineno, 1));
        }
    | MINUS Exp %prec UMINUS {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, createASTNode("MINUS", yylineno, 1));
            addChild($$, $2);
        }
    | NOT Exp {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, createASTNode("NOT", yylineno, 1));
            addChild($$, $2);
        }
    | ID LP Args RP {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", yylineno, 1));
        }
    | ID LP RP {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", yylineno, 1));
            addChild($$, createASTNode("RP", yylineno, 1));
        }
    | Exp LB Exp RB {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LB", yylineno, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RB", yylineno, 1));
        }
    | Exp DOT ID {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("DOT", yylineno, 1));
            addChild($$, $3);
        }
    | ID {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
        }
    | INT {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
        }
    | FLOAT {
            $$ = createASTNode("Exp", yylineno, 0);
            addChild($$, $1);
        }
    | error {
            $$ = createASTNode("Exp", yylineno, 0);
        }
    ;

Args : Exp COMMA Args {
            $$ = createASTNode("Args", yylineno, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", yylineno, 1));
            addChild($$, $3);
        }
     | Exp {
            $$ = createASTNode("Args", yylineno, 0);
            addChild($$, $1);
        }
     ;

%%
void yyerror(char* msg) {
    fprintf(stdout, "Error type B at Line %d: %s around \"%s\".\n", yylineno, msg, yytext);
}