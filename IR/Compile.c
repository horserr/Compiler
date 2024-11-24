#include "Compile.h"

#include <stdlib.h>
#include <linux/limits.h>
#ifdef LOCAL
#include <ParseTree.h>
#include <utils.h>
#else
#include "ParseTree.h"
#include "utils.h"
#endif

#define MAX_NUM_ARGC 20

#define COMPILE(root, part) \
  compile##part(getChildByName(root, #part))

#define CODE_ASSIGN(left, right) (Code){\
    .kind = C_ASSIGN,\
    .as.assign = {\
      .left = left,\
      .right = right\
    }\
  }

#define OP_TEMP() (Operand){\
    .kind = O_TEM_VAR,\
    .var_no = tmp_cnt++\
  }

int label_cnt = 0;
int tmp_cnt = 0;

const SymbolTable *symbolTable;
// a list of code
Chunk *sentinelChunk = NULL;

static void compileArgs(const ParseTNode *node, Operand *stack, int *top);

static Operand evalFuncInvocation(const ParseTNode *node) {
  const char *expressions[] = {
    "ID LP Args RP",
    "ID LP RP",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (strcmp(getStrFromID(node), "read") == 0) {
    assert(i == 1);
    const Operand tmp = OP_TEMP();
    addCode(sentinelChunk, (Code){.kind = C_READ, .as.unary = tmp});
    return tmp;
  }
  if (i == 0) {
    Operand stack[MAX_NUM_ARGC]; // maximum 20 arguments
    int top = 0;
    compileArgs(getChildByName(node, "Args"), stack, &top);
    if (strcmp(getStrFromID(node), "write") == 0) {
      assert(top == 1);
      addCode(sentinelChunk, (Code){.kind = C_WRITE, .as.unary = stack[0]});
      return (Operand){.kind = O_CONSTANT, .value_s = my_strdup("0")};
    }
    for (int j = top - 1; j >= 0; --j) {
      // todo
    }
  }
}

static Operand compileExp(const ParseTNode *node);

static Operand evalArithmatic(const ParseTNode *node) {
  const char *expressions[] = {
    "Exp PLUS Exp",
    "Exp MINUS Exp",
    "Exp STAR Exp",
    "Exp DIV Exp",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  enum binary_op { ADD, SUB, MUL, DIV };
#define CODE_BINARY_CASE(T) \
  if (i == T) { \
    const Operand op2 = compileExp(node->children.container[2]); \
    const Operand op1 = compileExp(node->children.container[0]); \
    const Operand result = OP_TEMP(); \
    addCode(sentinelChunk, (Code){ \
              .kind = C_##T, .as.binary = { \
                .result = result, \
                .op1 = op1, \
                .op2 = op2 \
              } \
            }); \
    return result; \
  }
  CODE_BINARY_CASE(ADD);
  CODE_BINARY_CASE(SUB);
  CODE_BINARY_CASE(MUL);
  CODE_BINARY_CASE(DIV);
  exit(EXIT_FAILURE);
#undef CODE_BINARY_CASE
}

// todo check every receiver of this function, and make it free.
// remember to free returned operand
static Operand compileExp(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Exp") == 0);
  const char *expressions[] = {
    [0] = "Exp ASSIGNOP Exp",
    [1] = "Exp AND Exp",
    [2] = "Exp OR Exp",
    [3] = "Exp RELOP Exp",
    [4] = "Exp PLUS Exp",
    [5] = "Exp MINUS Exp",
    [6] = "Exp STAR Exp",
    [7] = "Exp DIV Exp",
    [8] = "LP Exp RP",
    [9] = "MINUS Exp",
    [10] = "NOT Exp",
    [11] = "ID LP Args RP",
    [12] = "ID LP RP",
    [13] = "Exp LB Exp RB",
    [14] = "Exp DOT ID",
    [15] = "ID",
    [16] = "INT",
    [17] = "FLOAT"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (4 <= i && i <= 7)
    return evalArithmatic(node);
  if (i == 11 || i == 12)
    return evalFuncInvocation(node);
  if (i == 15)
    return (Operand){.kind = O_VARIABLE, .value_s = my_strdup(getStrFromID(node))};
  if (i == 16)
    return (Operand){.kind = O_CONSTANT, .value_s = int2String(getValFromINT(node))};
  if (i == 17)
    return (Operand){.kind = O_CONSTANT, .value_s = float2String(getValFromFLOAT(node))};
}

static void compileArgs(const ParseTNode *node, Operand *stack, int *top) {
  assert(node != NULL);
  assert(strcmp(node->name,"Args") == 0);
  const char *expressions[] = {
    "Exp",
    "Exp COMMA Args"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (*top >= MAX_NUM_ARGC) {
    DEBUG_INFO("Too many arguments. Only support 20.\n");
    exit(EXIT_FAILURE);
  }
  stack[(*top)++] = COMPILE(node, Exp);
  if (i == 1)
    compileArgs(getChildByName(node, "Args"), stack, top);
}

static Operand compileVarDec(const ParseTNode *node);

static void compileDec(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Dec") == 0);
  const char *expressions[] = {
    "VarDec",
    "VarDec ASSIGNOP Exp"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 1) { // right first
    const Operand right = COMPILE(node, Exp);
    const Operand left = COMPILE(node, VarDec);
    addCode(sentinelChunk, CODE_ASSIGN(left, right));
  }
}

static void compileDecList(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"DecList") == 0);
  const char *expressions[] = {
    "Dec",
    "Dec COMMA DecList"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  COMPILE(node, Dec);
  if (i == 1)
    COMPILE(node, DecList);
}

static void compileDef(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Def") == 0);
  const char *expressions[] = {
    "Specifier DecList SEMI"
  };
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  COMPILE(node, DecList);
}

static void compileDefList(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"DefList") == 0);
  const char *expressions[] = {
    "",
    "Def DefList"
  };
  if (EXPRESSION_INDEX(node, expressions) == 0) return;
  COMPILE(node, Def);
  COMPILE(node, DefList);
}

static void compileStmt(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Stmt") == 0);
  const char *expressions[] = {
    "Exp SEMI",                    // 0
    "CompSt",                      // 1
    "RETURN Exp SEMI",             // 2
    "IF LP Exp RP Stmt",           // 3
    "IF LP Exp RP Stmt ELSE Stmt", // 4
    "WHILE LP Exp RP Stmt"         // 5
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) {
    const Operand tmp = COMPILE(node, Exp);
    // may contain val_s and address in it
    freeOp(&tmp);
  }
}

static void compileStmtList(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"StmtList") == 0);
  const char *expressions[] = {
    "",
    "Stmt StmtList"
  };
  if (EXPRESSION_INDEX(node, expressions) == 0) return;
  COMPILE(node, Stmt);
  COMPILE(node, StmtList);
}

static void compileCompSt(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"CompSt") == 0);
  const char *expressions[] = {
    "LC DefList StmtList RC"
  };
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  COMPILE(node, DefList);
  COMPILE(node, StmtList);
}

static Operand compileParamDec(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"ParamDec") == 0);
  const char *expressions[] = {
    "Specifier VarDec"
  };
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  return COMPILE(node, VarDec);
}

static void compileVarList(const ParseTNode *node, Operand *stack, int *top) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarList") == 0);
  const char *expressions[] = {
    "ParamDec",
    "ParamDec COMMA VarList"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (*top >= MAX_NUM_ARGC) {
    DEBUG_INFO("Too many arguments! Only support 20.\n");
    exit(EXIT_FAILURE);
  }
  stack[(*top)++] = COMPILE(node, ParamDec);
  if (i == 1)
    compileVarList(getChildByName(node, "VarList"), stack, top);
}

static void compileFunDec(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"FunDec") == 0);
  const char *expressions[] = {
    "ID LP VarList RP",
    "ID LP RP"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  // add function name
  addCode(sentinelChunk, (Code){
            .kind = C_FUNCTION, .as.unary = (Operand){
              .kind = O_FUNCTION, .value_s = my_strdup(getStrFromID(node))
            }
          });
  if (i == 1) return;
  Operand stack[MAX_NUM_ARGC];
  int top = 0;
  compileVarList(getChildByName(node, "VarList"), stack, &top);
  assert(top != 0);
  for (int j = 0; j < top; ++j) {
    addCode(sentinelChunk, (Code){.kind = C_PARAM, .as.unary = stack[j]});
  }
}

// return the name
static Operand compileVarDec(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarDec") == 0);
  const char *expressions[] = {
    "ID",
    "VarDec LB INT RB"
  };
  const int i = EXPRESSION_INDEX(node, expressions);

  if (i == 1) return COMPILE(node, VarDec);
  return (Operand){.kind = O_VARIABLE, .value_s = my_strdup(getStrFromID(node))};
}

static void compileExtDef(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"ExtDef") == 0);
  const char *expressions[] = {
    "Specifier SEMI",
    "Specifier ExtDecList SEMI",
    "Specifier FunDec CompSt"
  };
  if (EXPRESSION_INDEX(node, expressions) != 2) return;
  COMPILE(node, FunDec);
  COMPILE(node, CompSt);
}

static void compileExtDefList(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"ExtDefList") == 0);
  const char *expressions[] = {
    "",
    "ExtDef ExtDefList"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) return;
  COMPILE(node, ExtDef);
  COMPILE(node, ExtDefList);
}

const Chunk* compile(const ParseTNode *root, const SymbolTable *table) {
  assert(root != NULL);
  assert(strcmp(root->name, "Program") == 0);
  char *expressions[] = {
    "ExtDefList"
  };
  assert(EXPRESSION_INDEX(root, expressions) == 0);
  symbolTable = table;
  initChunk(&sentinelChunk);
  COMPILE(root, ExtDefList);
  return sentinelChunk;
}

#undef COMPILE
#undef CODE_ASSIGN
#undef MAX_NUM_ARGC
