%{
  #include "lex.yy.c"

  typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
  } YYLTYPE;

  #define YYLTYPE_IS_DECLARED 1

  void yyerror(char* msg);
%}

/* declared types */
%union {
  int type_int;
  float type_float;
  double type_double;
}

/* declared tokens */
/* %token <type_int> INT
%token <type_float> FLOAT */
%token <type_int> INT
%token <type_float> FLOAT

/* declared operators and precedence */
/* %token ADD SUB MUL DIV
%right ASSIGN
%left ADD SUB
%left MUL DIV
%left LP RP */

/* declared non-terminals */
%type <type_int> Exp Factor Term
%token SEMI COMMA ASSIGNOP
%token LP RP LB RB LC RC
%token RELOP PLUS MINUS STAR DIV AND OR DOT NOT TYPE STRUCT RETURN IF ELSE WHILE ID

%%
Calc : /* empty */
    | Exp { printf("= %d\n", $1); }
    ;

Exp : Factor
    | Exp PLUS Factor { $$ = $1 + $3; }
    | Exp MINUS Factor { $$ = $1 - $3; }
    ;

Factor : Term
    | Factor STAR Term { $$ = $1 * $3; }
    | Factor DIV Term { $$ = $1 / $3; }
    ;

Term : INT
    ;
%%
void yyerror(char* msg) {
    fprintf(stdout, "error: %s\n", msg);
}