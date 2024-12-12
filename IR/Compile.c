#ifdef LOCAL
#include <Compile.h>
#else
#include "Compile.h"
#endif

#define STACK_MAX_NUM 20

#define COMPILE(root, part) \
  compile##part(getChildByName(root, #part))

#define CODE_ASSIGN(left_, right_) (Code){\
    .kind = C_ASSIGN,\
    .as.assign = {\
      .left = left_,\
      .right = right_\
    }\
  }
#define CODE_UNARY(T, u_op) (Code){\
  .kind = C_##T, .as.unary = u_op\
}
#define CODE_BINARY(T, result_, x, y) (Code){\
  .kind = C_##T, .as.binary = {\
    .result = result_, \
    .op1 = x,\
    .op2 = y\
  }\
}

#define OP_TEMP() (Operand){\
  .kind = O_TEM_VAR, .var_no = tmp_cnt++\
}
#define OP_LABEL() (Operand){\
  .kind = O_LABEL, .var_no = label_cnt++\
}
#define OP_CONSTANT(value_) (Operand){\
  .kind = O_CONSTANT, .value = value_\
}

int label_cnt = 0;
int tmp_cnt = 0;

const SymbolTable *symbolTable;
Chunk *sentinelChunk = NULL;

// a handy way to get parameters information while inside `CompSt`
struct {
  int argc;
  // pointing to the function type
  Type **argv;
  // a list of parameter name
  char *param_s[MAX_DIMENSION];
} funcParamInfo;

static unsigned arrayTypeSize(const char *name, int depth);

static void compileArgs(const ParseTNode *node, Operand *stack, int *top);

// helper function of `compileExp`
static Operand evalFuncInvocation(const ParseTNode *node) {
  const char *expressions[] = {
    "ID LP Args RP",
    "ID LP RP",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  const char *func_name = getStrFrom(node, ID);
  if (strcmp(func_name, "read") == 0) {
    assert(i == 1);
    const Operand tmp = OP_TEMP();
    addCode(sentinelChunk, CODE_UNARY(READ, tmp));
    return tmp;
  }
  if (i == 0) {
    Operand stack[STACK_MAX_NUM]; // maximum 20 arguments
    int top = 0;
    compileArgs(getChildByName(node, "Args"), stack, &top);
    if (strcmp(func_name, "write") == 0) {
      assert(top == 1);
      addCode(sentinelChunk, CODE_UNARY(WRITE, stack[0]));
      return OP_CONSTANT(0);
    }
    for (int j = top - 1; j >= 0; --j)
      addCode(sentinelChunk, CODE_UNARY(ARG, stack[j]));
  }
  // i == 0 or i == 1
  const Operand tmp = OP_TEMP();
  const Operand func_op = (Operand){
    .kind = O_INVOKE,
    .value_s = my_strdup(func_name)
  };
  addCode(sentinelChunk, CODE_ASSIGN(tmp, func_op));
  return tmp;
}

static Operand compileExp(const ParseTNode *node);

// helper function of `evalRelation` and `compileBranch`
static void compileCondition(const ParseTNode *node,
                             const Operand *T_label, const Operand *F_label) {
#define names_equal(i) nodeChildrenNamesEqual(node, expressions[i])
#define CODE_IFGOTO(x_, y_, label_, relation_) (Code){\
  .kind = C_IFGOTO,\
  .as.ternary = {\
    .x = x_, .y = y_,\
    .label = label_,\
    .relation = relation_\
    }\
  }
  const char *expressions[] = {
    "NOT Exp",
    "Exp AND Exp",
    "Exp OR Exp",
    "Exp RELOP Exp",
  };
  if (names_equal(0))
    return compileCondition(getChild(node, 1), F_label, T_label);
  if (names_equal(1)) {
    const Operand label = OP_LABEL();
    compileCondition(getChild(node, 0), &label, F_label);
    addCode(sentinelChunk, CODE_UNARY(LABEL, label));
    compileCondition(getChild(node, 2), T_label, F_label);
    return;
  }
  if (names_equal(2)) {
    const Operand label = OP_LABEL();
    compileCondition(getChild(node, 0), T_label, &label);
    addCode(sentinelChunk, CODE_UNARY(LABEL, label));
    compileCondition(getChild(node, 2), T_label, F_label);
    return;
  }
  if (names_equal(3)) {
    // notice here: use `compileExp` rather than `compileCondition`
    const Operand o1 = compileExp(getChild(node, 0));
    const Operand o2 = compileExp(getChild(node, 2));
    addCode(sentinelChunk,
            CODE_IFGOTO(o1, o2, *T_label,
                        my_strdup(getStrFrom(node, RELOP))));
    addCode(sentinelChunk, CODE_UNARY(GOTO, *F_label));
    return;
  }
  // single expression condition
  // e.g. if(a), if(a[0])...
  // note: pass the same node to `compileExp`
  addCode(sentinelChunk,
          CODE_IFGOTO(compileExp(node), OP_CONSTANT(0),
                      *T_label, my_strdup("!=")));
  addCode(sentinelChunk, CODE_UNARY(GOTO, *F_label));
#undef names_equal
#undef CODE_IFGOTO
}

// helper function of `compileExp`
static Operand evalRelation(const ParseTNode *node) {
  const Operand result = OP_TEMP();
  addCode(sentinelChunk, CODE_ASSIGN(result, OP_CONSTANT(0)));
  const Operand T_label = OP_LABEL(), F_label = OP_LABEL();
  compileCondition(node, &T_label, &F_label);
  addCode(sentinelChunk, CODE_UNARY(LABEL, T_label));
  addCode(sentinelChunk, CODE_ASSIGN(result, OP_CONSTANT(1)));
  addCode(sentinelChunk, CODE_UNARY(LABEL, F_label));
  return result;
}

// helper function of `compileExp`
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
    const Operand op1 = compileExp(getChild(node, 0)); \
    const Operand op2 = compileExp(getChild(node, 2)); \
    const Operand result = OP_TEMP(); \
    addCode(sentinelChunk, CODE_BINARY(T, result, op1, op2)); \
    return result; \
  }
  CODE_BINARY_CASE(ADD);
  CODE_BINARY_CASE(SUB);
  CODE_BINARY_CASE(MUL);
  CODE_BINARY_CASE(DIV);
  exit(EXIT_FAILURE);
#undef CODE_BINARY_CASE
}

/**
 *  helper function of `derefArray`
 *  memorize those index value from right to left.
 *  @return a pointer directly pointing to string value, no copy.
 */
static const char* gatherArrayInfo(const ParseTNode *node, Operand stack[], int *top) {
  if (*top >= STACK_MAX_NUM) {
    DEBUG_INFO("Too much dimension. Only support 20.\n");
    exit(EXIT_FAILURE);
  }

  stack[(*top)++] = compileExp(getChild(node, 2));
  const ParseTNode *exp = getChild(node, 0);
  if (nodeChildrenNamesEqual(exp, "ID")) {
    return getStrFrom(exp, ID);
  }
  return gatherArrayInfo(exp, stack, top);
}

// helper function of `derefArray` and `compileExp`
static Operand evalVariable(const char *name) {
  const char *name_copy = my_strdup(name);
  // search in funcParamInfo
  for (int i = 0; i < funcParamInfo.argc; ++i) {
    if (strcmp(name, funcParamInfo.param_s[i]) != 0) continue;
    // always return name, whether it is array or basic type.
    return (Operand){.kind = O_VARIABLE, .value_s = name_copy};
  }
  const Type *type = searchWithName(symbolTable->vars, name)->variable.type;
  assert(type->kind != ERROR);
  // search in symbolTable
  if (type->kind == STRUCT) {
    DEBUG_INFO("Not implement yet.\n");
    exit(EXIT_FAILURE);
  }
  if (type->kind == ARRAY) {
    Operand *var_op = malloc(sizeof(Operand));
    var_op->kind = O_VARIABLE;
    var_op->value_s = name_copy;
    return (Operand){.kind = O_REFER, .address = var_op};
  }
  // basic type
  return (Operand){.kind = O_VARIABLE, .value_s = name_copy};
}

// helper function of `dereference`
static Operand derefArray(const ParseTNode *node) {
  const char *expressions[] = {"Exp LB Exp RB"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);

  Operand stack[STACK_MAX_NUM];
  int top = 0;
  const char *name = gatherArrayInfo(node, stack, &top);
  const Operand base_addr_op = evalVariable(name);

  const Operand offset_op = OP_TEMP();
  addCode(sentinelChunk, CODE_ASSIGN(offset_op, OP_CONSTANT(0)));
  const Operand tmp = OP_TEMP();  // no need to create a new temp variable each loop
  for (int i = 0; i < top; ++i) { // sum all offsets
    addCode(sentinelChunk,
            CODE_BINARY(MUL, tmp, stack[i],
                        OP_CONSTANT(arrayTypeSize(name, top - i))));
    addCode(sentinelChunk, CODE_BINARY(ADD, offset_op, offset_op, tmp));
  }

  const Operand result_addr_op = OP_TEMP();
  addCode(sentinelChunk, CODE_BINARY(ADD, result_addr_op, base_addr_op, offset_op));

  Operand *deref_op = malloc(sizeof(Operand));
  *deref_op = result_addr_op; // get a copy of result_addr in heap.
  return (Operand){.kind = O_DEREF, .address = deref_op};
}

// helper function of `dereference`
static Operand derefStruct(const ParseTNode *node) {
  const char *expressions[] = {"Exp DOT ID"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);

  DEBUG_INFO("Not implement yet.\n");
  exit(EXIT_FAILURE);
}

// helper function of `evalExp`
static Operand dereference(const ParseTNode *node) {
  const char *expressions[] = {
    "Exp LB Exp RB",
    "Exp DOT ID",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) return derefArray(node);
  if (i == 1) return derefStruct(node);
  DEBUG_INFO("Shouldn't reach this place.\n");
  exit(EXIT_FAILURE);
}

// todo check every receiver of this function, and make it free.
// remember to free returned operand
static Operand compileExp(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Exp") == 0);
  // notice: remove AND, OR, RELOP, NOT. They are now inside compileCondition.
  const char *expressions[] = {
    [0] = "Exp ASSIGNOP Exp",
    [1] = "Exp PLUS Exp",
    [2] = "Exp MINUS Exp",
    [3] = "Exp STAR Exp",
    [4] = "Exp DIV Exp",
    [5] = "LP Exp RP",
    [6] = "MINUS Exp",
    [7] = "ID LP Args RP",
    [8] = "ID LP RP",
    [9] = "Exp LB Exp RB",
    [10] = "Exp DOT ID",
    [11] = "ID",
    [12] = "INT",
    [13] = "FLOAT",
    [14] = "NOT Exp",
    [15] = "Exp AND Exp",
    [16] = "Exp OR Exp",
    [17] = "Exp RELOP Exp",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) {
    const Operand right = compileExp(getChild(node, 2));
    const Operand left = compileExp(getChild(node, 0));
    addCode(sentinelChunk, CODE_ASSIGN(left, right));
    // if simply return right_op, this may cause double free error.
    return copyOperand(&right);
  }
  if (1 <= i && i <= 4)
    return evalArithmatic(node);
  if (i == 5)
    return COMPILE(node, Exp);
  if (i == 6) {
    const Operand right = COMPILE(node, Exp);
    const Operand result = OP_TEMP();
    addCode(sentinelChunk,
            CODE_BINARY(SUB, result, OP_CONSTANT(0), right));
    return result;
  }
  if (i == 7 || i == 8)
    return evalFuncInvocation(node);
  if (i == 9 || i == 10)
    return dereference(node);
  if (i == 11)
    return evalVariable(getStrFrom(node, ID));
  if (i == 12)
    return OP_CONSTANT(getValFromINT(node));
  if (i == 13) {
    DEBUG_INFO("Doesn't support float type.\n");
    exit(EXIT_FAILURE);
  }
  if (14 <= i && i <= 17)
    return evalRelation(node);
  DEBUG_INFO("Shouldn't reach this place.\n");
  exit(EXIT_FAILURE);
}

static void compileArgs(const ParseTNode *node, Operand *stack, int *top) {
  assert(node != NULL);
  assert(strcmp(node->name,"Args") == 0);
  const char *expressions[] = {
    "Exp",
    "Exp COMMA Args"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (*top >= STACK_MAX_NUM) {
    DEBUG_INFO("Too many arguments. Only support 20.\n");
    exit(EXIT_FAILURE);
  }
  stack[(*top)++] = COMPILE(node, Exp);
  if (i == 1)
    compileArgs(getChildByName(node, "Args"), stack, top);
}

/**
 *  helper function of `compileVarDec`
 *  @note not the copy, directly pointing to the original value.
 */
const char* getVariableName(const ParseTNode *node) {
  const char *expressions[] = {
    "ID",
    "VarDec LB INT RB"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) return getStrFrom(node, ID);
  return getVariableName(getChildByName(node, "VarDec"));
}

static Operand compileVarDec(const ParseTNode *node, const bool is_param) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarDec") == 0);
  const char *expressions[] = {
    "ID",
    "VarDec LB INT RB"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  const char *name_copy = my_strdup(getVariableName(node));
  const Operand var_op = (Operand){.kind = O_VARIABLE, .value_s = name_copy};

  if (i == 0 || is_param) return var_op;
  addCode(sentinelChunk, (Code){
            .kind = C_DEC,
            .as.dec = {
              .target = var_op,
              .size = arrayTypeSize(name_copy, 0)
            }
          });
  return copyOperand(&var_op);
}

static void compileDec(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Dec") == 0);
  const char *expressions[] = {
    "VarDec",
    "VarDec ASSIGNOP Exp"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) {
    const Operand tmp =
        compileVarDec(getChildByName(node, "VarDec"),false);
    cleanOp(&tmp);
    return;
  }
  // right first
  const Operand right = COMPILE(node, Exp);
  const Operand left =
      compileVarDec(getChildByName(node, "VarDec"),false);
  addCode(sentinelChunk, CODE_ASSIGN(left, right));
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

static void compileStmt(const ParseTNode *node);

// helper function of `compileBranches`
static void compileIfStmt(const ParseTNode *node) {
  const char *expressions[] = {"IF LP Exp RP Stmt"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  const Operand T_label = OP_LABEL();
  const Operand F_label = OP_LABEL();
  compileCondition(getChildByName(node, "Exp"), &T_label, &F_label);
  addCode(sentinelChunk, CODE_UNARY(LABEL, T_label));
  COMPILE(node, Stmt);
  addCode(sentinelChunk, CODE_UNARY(LABEL, F_label));
}

// helper function of `compileBranches`
static void compileIfElseStmt(const ParseTNode *node) {
  const char *expressions[] = {"IF LP Exp RP Stmt ELSE Stmt"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  const Operand T_label = OP_LABEL();
  const Operand F_label = OP_LABEL();
  compileCondition(getChildByName(node, "Exp"), &T_label, &F_label);
  addCode(sentinelChunk, CODE_UNARY(LABEL, T_label));
  compileStmt(getChild(node, 4));
  const Operand jump = OP_LABEL();
  addCode(sentinelChunk, CODE_UNARY(GOTO, jump));
  addCode(sentinelChunk, CODE_UNARY(LABEL, F_label));
  compileStmt(getChild(node, 6));
  addCode(sentinelChunk, CODE_UNARY(LABEL, jump));
}

// helper function of `compileBranches`
static void compileWhileStmt(const ParseTNode *node) {
  const char *expressions[] = {"WHILE LP Exp RP Stmt"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  const Operand start = OP_LABEL();
  addCode(sentinelChunk, CODE_UNARY(LABEL, start));
  const Operand T_label = OP_LABEL();
  const Operand F_label = OP_LABEL();
  compileCondition(getChildByName(node, "Exp"), &T_label, &F_label);
  addCode(sentinelChunk, CODE_UNARY(LABEL, T_label));
  COMPILE(node, Stmt);
  addCode(sentinelChunk, CODE_UNARY(GOTO, start));
  addCode(sentinelChunk, CODE_UNARY(LABEL, F_label));
}

// helper function of `compileStmt`
static void compileBranches(const ParseTNode *node) {
  const char *expressions[] = {
    "IF LP Exp RP Stmt",
    "WHILE LP Exp RP Stmt",
    "IF LP Exp RP Stmt ELSE Stmt",
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0) return compileIfStmt(node);
  if (i == 1) return compileWhileStmt(node);
  return compileIfElseStmt(node);
}

static void compileCompSt(const ParseTNode *node);

static void compileStmt(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Stmt") == 0);
  const char *expressions[] = {
    "CompSt",                      // 0
    "IF LP Exp RP Stmt",           // 1
    "IF LP Exp RP Stmt ELSE Stmt", // 2
    "WHILE LP Exp RP Stmt",        // 3
    "Exp SEMI",                    // 4
    "RETURN Exp SEMI",             // 5
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (i == 0)
    return COMPILE(node, CompSt);
  if (1 <= i && i <= 3)
    return compileBranches(node);
  if (i == 4) {
    const Operand tmp = COMPILE(node, Exp);
    cleanOp(&tmp); // may contain val_s and address in it
    return;
  }
  const Operand tmp = COMPILE(node, Exp);
  addCode(sentinelChunk, CODE_UNARY(RETURN, tmp));
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
  const char *expressions[] = {"Specifier VarDec"};
  assert(EXPRESSION_INDEX(node, expressions) == 0);
  return compileVarDec(getChildByName(node, "VarDec"), true);
}

// helper function of `compileFunDec`
static void compileVarList(const ParseTNode *node, Operand *stack, int *top) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarList") == 0);
  const char *expressions[] = {
    "ParamDec",
    "ParamDec COMMA VarList"
  };
  const int i = EXPRESSION_INDEX(node, expressions);
  if (*top >= STACK_MAX_NUM) {
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
  const char *name = getStrFrom(node, ID);
  const Operand func_op = (Operand){
    .kind = O_VARIABLE, .value_s = my_strdup(name)
  };
  addCode(sentinelChunk, CODE_UNARY(FUNCTION, func_op));
  if (i == 1) return;

  Operand stack[STACK_MAX_NUM];
  int top = 0;
  compileVarList(getChildByName(node, "VarList"), stack, &top);
  assert(top != 0);
  // update funcParamInfo
  const Data *data = searchWithName(symbolTable->funcs, name);
  funcParamInfo.argc = data->function.argc;
  funcParamInfo.argv = data->function.argvTypes;
  for (int j = 0; j < top; ++j) {
    const Operand var_op = stack[j];
    addCode(sentinelChunk, CODE_UNARY(PARAM, var_op));
    funcParamInfo.param_s[j] = my_strdup(var_op.value_s);
  }
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
  // clear funcParamInfo
  for (int i = 0; i < funcParamInfo.argc; ++i)
    free(funcParamInfo.param_s[i]);
  memset(&funcParamInfo, 0, sizeof(funcParamInfo));
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

#undef STACK_MAX_NUM
#undef COMPILE
#undef CODE_ASSIGN
#undef CODE_UNARY
#undef CODE_BINARY
#undef OP_TEMP
#undef OP_LABEL
#undef OP_CONSTANT

/**
 *  helper function of `arrayTypeSize`
 *  @note no need to worry about alignment.
 */
static unsigned typeSizeHelper(const Type *type) {
  assert(type != NULL && type->kind != ERROR);
  if (type->kind == INT || type->kind == FLOAT) {
    return 4;
  }
  if (type->kind == ARRAY)
    return type->array.size * typeSizeHelper(type->array.elemType);

  // struct
  const structFieldElement *f = type->structure.fields;
  unsigned size = 0;
  while (f != NULL) {
    size += typeSizeHelper(f->elemType);
    f = f->next;
  }
  return size;
}

// return the size to be allocated
static unsigned arrayTypeSize(const char *name, const int depth) {
  const Type *type = searchWithName(symbolTable->vars, name)->variable.type;
  for (int i = 0; i < depth; ++i) {
    assert(type->kind == ARRAY);
    type = type->array.elemType;
  }
  return typeSizeHelper(type);
}
