%{
  #include "lex.yy.c"

  void yyerror(char* msg);
%}

/* declared types */
%union {
  int type_int;
  float type_float;
  double type_double;
}

/* declared tokens */
%token <type_int> INT
%token <type_float> FLOAT

%token SEMI COMMA ASSIGNOP
%token RELOP DOT
%token PLUS MINUS STAR DIV
%token AND OR NOT
%token LP RP LB RB LC RC

%token TYPE STRUCT RETURN IF ELSE WHILE ID

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
/* %type <type_int> Exp Factor Term */
%%
/* High-level Definitions */
Program : ExtDefList
    ;

ExtDefList : /* empty */
    | ExtDef ExtDefList
    ;

ExtDef : Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;

ExtDecList : VarDec
    | VarDec COMMA ExtDecList
    ;

/* Specifiers */
Specifier : TYPE
    | StructSpecifier
    ;

StructSpecifier : STRUCT OptTag LC DefList RC
    | STRUCT Tag
    ;

OptTag : /* empty */
    | ID
    ;

Tag : ID
    ;

/* Declarators */
VarDec : ID
    | VarDec LB INT RB
    ;

FunDec : ID LP VarList RP
    | ID LP RP
    ;

VarList : ParamDec COMMA VarList
    | ParamDec
    ;

ParamDec : Specifier VarDec
    ;

/* Statements */
/* variables can only be declared at the beginning of CompSt */
CompSt : LC DefList StmtList RC
    ;

StmtList : /* empty */
    | Stmt StmtList
    ;

Stmt : Exp SEMI
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt %prec INFERIOR_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
    ;

/* Local Definitions */
DefList : /* empty */
    | Def DefList
    ;

Def : Specifier DecList SEMI
    ;

DecList : Dec
    | Dec COMMA DecList
    ;

Dec : VarDec
    | VarDec ASSIGNOP Exp
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp %prec UMINUS  // unary minus
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
    ;

Args : Exp COMMA Args
    | Exp
    ;


%%
void yyerror(char* msg) {
    fprintf(stdout, "Error type B at Line %d: %s\n", yylineno, msg);
}