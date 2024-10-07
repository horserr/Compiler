%{
  #include "lex.yy.c"

  #ifdef DEBUG
    #undef  YYDEBUG
    #define YYDEBUG 1
  #endif

  void yyerror(char* msg);
  extern ASTNode *root; // Root of the AST
  extern int hasError;
%}

%define api.value.type {ASTNode*}
%locations
%code requires{
  #include "AST.h"
}

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
            root = createASTNode("Program", @$.first_line, 0);
            addChild(root, $1);
        }
        ;

ExtDefList : /* empty */ {
                $$ = createASTNode("ExtDefList", @$.first_line, 0);
            }
           | ExtDef ExtDefList {
                $$ = createASTNode("ExtDefList", @$.first_line, 0);
                addChild($$, $1);
                addChild($$, $2);
            }
           | error ExtDefList
           ;

ExtDef : Specifier ExtDecList SEMI {
            $$ = createASTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", @3.first_line, 1));
        }
       | Specifier SEMI {
            $$ = createASTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("SEMI", @2.first_line, 1));
        }
       | Specifier FunDec CompSt {
            $$ = createASTNode("ExtDef", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, $3);
        }
       | error ExtDecList SEMI
       | Specifier ExtDecList error SEMI
       | error SEMI
       | error FunDec CompSt
       | Specifier error CompSt
       | Specifier FunDec error
       ;

ExtDecList : VarDec {
                $$ = createASTNode("ExtDecList", @$.first_line, 0);
                addChild($$, $1);
            }
           | VarDec COMMA ExtDecList {
                $$ = createASTNode("ExtDecList", @$.first_line, 0);
                addChild($$, $1);
                addChild($$, createASTNode("COMMA", @2.first_line, 1));
                addChild($$, $3);
            }
           | error COMMA ExtDecList
           ;

/* Specifiers */
Specifier : TYPE {
                $$ = createASTNode("Specifier", @$.first_line, 0);
                addChild($$, $1);
            }
          | StructSpecifier {
                $$ = createASTNode("Specifier", @$.first_line, 0);
                addChild($$, $1);
            }
          ;

StructSpecifier : STRUCT OptTag LC DefList RC {
                    $$ = createASTNode("StructSpecifier", @$.first_line, 0);
                    addChild($$, createASTNode("STRUCT", @1.first_line, 1));
                    addChild($$, $2);
                    addChild($$, createASTNode("LC", @3.first_line, 1));
                    addChild($$, $4);
                    addChild($$, createASTNode("RC", @5.first_line, 1));
                }
                | STRUCT Tag {
                    $$ = createASTNode("StructSpecifier", @$.first_line, 0);
                    addChild($$, createASTNode("STRUCT", @1.first_line, 1));
                    addChild($$, $2);
                }
                | STRUCT error LC DefList RC
                | STRUCT OptTag LC error RC
                | STRUCT error
                ;

OptTag : /* empty */ {
            $$ = createASTNode("OptTag", @$.first_line, 0);
       }
       | ID {
            $$ = createASTNode("OptTag", @$.first_line, 0);
            addChild($$, $1);
       }
       ;

Tag : ID {
        $$ = createASTNode("Tag", @$.first_line, 0);
        addChild($$, $1);
    }
    ;

/* Declarators */
VarDec : ID {
            $$ = createASTNode("VarDec", @$.first_line, 0);
            addChild($$, $1);
       }
       | VarDec LB INT RB {
            $$ = createASTNode("VarDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LB", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RB", @4.first_line, 1));
       }
       | VarDec LB error RB
       | error LB INT RB
       ;

FunDec : ID LP VarList RP {
            $$ = createASTNode("FunDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", @4.first_line, 1));
        }
       | ID LP RP {
            $$ = createASTNode("FunDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, createASTNode("RP", @3.first_line, 1));
       }
       | error LP VarList RP
       | ID LP error RP
       | error LP RP
       ;

VarList : ParamDec COMMA VarList {
            $$ = createASTNode("VarList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
        }
        | ParamDec {
            $$ = createASTNode("VarList", @$.first_line, 0);
            addChild($$, $1);
        }
        | error COMMA VarList
        | ParamDec COMMA error
        ;

ParamDec : Specifier VarDec {
            $$ = createASTNode("ParamDec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
         }
         | error VarDec
         | Specifier error
         ;

/* Statements */
/* variables can only be declared at the beginning of CompSt */
CompSt : LC DefList StmtList RC {
            $$ = createASTNode("CompSt", @$.first_line, 0);
            addChild($$, createASTNode("LC", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, $3);
            addChild($$, createASTNode("RC", @4.first_line, 1));
       }
       | LC DefList error RC
       ;

StmtList : /* empty */ {
            $$ = createASTNode("StmtList", @$.first_line, 0);
         }
         | Stmt StmtList {
            $$ = createASTNode("StmtList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
         }
         ;

Stmt : Exp SEMI {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("SEMI", @2.first_line, 1));
        }
     | CompSt {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, $1);
        }
     | RETURN Exp SEMI {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, createASTNode("RETURN", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", @3.first_line, 1));
        }
     | IF LP Exp RP Stmt %prec INFERIOR_ELSE {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, createASTNode("IF", @1.first_line, 1));
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", @4.first_line, 1));
            addChild($$, $5);
        }
     | IF LP Exp RP Stmt ELSE Stmt {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, createASTNode("IF", @1.first_line, 1));
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", @4.first_line, 1));
            addChild($$, $5);
            addChild($$, createASTNode("ELSE", @6.first_line, 1));
            addChild($$, $7);
        }
     | WHILE LP Exp RP Stmt {
            $$ = createASTNode("Stmt", @$.first_line, 0);
            addChild($$, createASTNode("WHILE", @1.first_line, 1));
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", @4.first_line, 1));
            addChild($$, $5);
        }
       | error SEMI
       | RETURN error SEMI
       | IF LP error RP Stmt %prec INFERIOR_ELSE
       | IF LP Exp RP error %prec INFERIOR_ELSE
       | IF LP error RP Stmt ELSE Stmt
       | IF LP Exp RP error ELSE Stmt
       | IF LP Exp RP Stmt ELSE error
       | WHILE LP error RP Stmt
       | WHILE LP Exp RP error
     ;

/* Local Definitions */
DefList : /* empty */ {
            $$ = createASTNode("DefList", @$.first_line, 0);
        }
        | Def DefList {
            $$ = createASTNode("DefList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
        }
        ;

Def : Specifier DecList SEMI {
            $$ = createASTNode("Def", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, $2);
            addChild($$, createASTNode("SEMI", @3.first_line, 1));
    }
    ;

DecList : Dec {
            $$ = createASTNode("DecList", @$.first_line, 0);
            addChild($$, $1);
        }
        | Dec COMMA DecList {
            $$ = createASTNode("DecList", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
        }
        | error COMMA DecList
        ;

Dec : VarDec {
            $$ = createASTNode("Dec", @$.first_line, 0);
            addChild($$, $1);
        }
    | VarDec ASSIGNOP Exp {
            $$ = createASTNode("Dec", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("ASSIGNOP", @2.first_line, 1));
            addChild($$, $3);
        }
    | VarDec ASSIGNOP error
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("ASSIGNOP", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp AND Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("AND", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp OR Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("OR", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp RELOP Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("RELOP", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp PLUS Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("PLUS", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp MINUS Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("MINUS", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp STAR Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("STAR", @2.first_line, 1));
            addChild($$, $3);
        }
    | Exp DIV Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("DIV", @2.first_line, 1));
            addChild($$, $3);
        }
    | LP Exp RP {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, createASTNode("LP", @1.first_line, 1));
            addChild($$, $2);
            addChild($$, createASTNode("RP", @3.first_line, 1));
        }
    | MINUS Exp %prec UMINUS {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, createASTNode("MINUS", @1.first_line, 1));
            addChild($$, $2);
        }
    | NOT Exp {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, createASTNode("NOT", @1.first_line, 1));
            addChild($$, $2);
        }
    | ID LP Args RP {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RP", @4.first_line, 1));
        }
    | ID LP RP {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LP", @2.first_line, 1));
            addChild($$, createASTNode("RP", @3.first_line, 1));
        }
    | Exp LB Exp RB {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("LB", @2.first_line, 1));
            addChild($$, $3);
            addChild($$, createASTNode("RB", @4.first_line, 1));
        }
    | Exp DOT ID {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("DOT", @2.first_line, 1));
            addChild($$, $3);
        }
    | ID {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    | INT {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    | FLOAT {
            $$ = createASTNode("Exp", @$.first_line, 0);
            addChild($$, $1);
        }
    ;

Args : Exp COMMA Args {
            $$ = createASTNode("Args", @$.first_line, 0);
            addChild($$, $1);
            addChild($$, createASTNode("COMMA", @2.first_line, 1));
            addChild($$, $3);
        }
     | Exp {
            $$ = createASTNode("Args", @$.first_line, 0);
            addChild($$, $1);
        }
     ;

%%
void yyerror(char* msg) {
    hasError = 1;
    fprintf(stdout, "Error type B at Line %d: %s around \"%s\".\n", yylineno, msg, yytext);
}