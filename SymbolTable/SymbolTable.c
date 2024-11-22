#include "SymbolTable.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

Environment *currentEnv = NULL;
// register all defined functions
RedBlackTree *funcMap = NULL;
// a linked list registers those defined struct type
StructRegister *definedStructList = NULL;

/**
 * @brief helper function for 'resolveExp'
 * @warning may return ERROR type
 * @return a copy of type of expression
*/
const Type* evalBinaryOperator(const ParseTNode *node) {
  const char *expressions[] = {
    "Exp ASSIGNOP Exp", // 0
    "Exp AND Exp",      // 1
    "Exp OR Exp",       // 2
    "Exp RELOP Exp",    // 3
    "Exp PLUS Exp",     // 4
    "Exp MINUS Exp",    // 5
    "Exp STAR Exp",     // 6
    "Exp DIV Exp"       // 7
  };
  const int i = matchExprPattern(node, expressions,ARRAY_LEN(expressions));
  const Type *type1 = resolveExp(node->children.container[0]);
  if (type1->kind == ERROR) return type1;
  const Type *type2 = resolveExp(node->children.container[2]);
  if (type2->kind == ERROR) {
    freeType((Type *) type1);
    return type2;
  }
  if (i == 0) { // assignment
    if (!typeEqual(type1, type2)) {
      const int lineNum = getChildByName(node, "ASSIGNOP")->lineNum;
      error(5, lineNum, "Type mismatched for assignment.\n");
      freeType((Type *) type1);
      freeType((Type *) type2);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    if (!(nodeChildrenNameEqualHelper(node->children.container[0], "ID") ||
          nodeChildrenNameEqualHelper(node->children.container[0], "Exp LB Exp RB") ||
          nodeChildrenNameEqualHelper(node->children.container[0], "Exp DOT ID"))) {
      const int lineNum = getChildByName(node, "ASSIGNOP")->lineNum;
      error(6, lineNum, "The left-hand side of an assignment must be a variable.\n");
      freeType((Type *) type1);
      freeType((Type *) type2);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    freeType((Type *) type1);
    return type2;
  }
  if (1 <= i && i <= 3) { // logical operations
    if (type1->kind != INT || type2->kind != INT) {
      const int lineNum = node->children.container[1]->lineNum;
      error(7, lineNum, "Type mismatched for operands.\n");
      freeType((Type *) type1);
      freeType((Type *) type2);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    freeType((Type *) type1);
    return type2;
  } // arithmatic operations: 4 <= i && i <= 7
  if (!((type1->kind == INT || type1->kind == FLOAT) && typeEqual(type1, type2))) {
    const int lineNum = node->children.container[1]->lineNum;
    error(7, lineNum, "Type mismatched for operands.\n");
    freeType((Type *) type1);
    freeType((Type *) type2);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  freeType((Type *) type1);
  return type2;
}

/**
 * @brief helper function for 'resolveExp'
 * @warning may return ERROR type
 * @return a copy of type of function's return type.
*/
const Type* evalFuncInvocation(const ParseTNode *node) {
  const char *expressions[] = {
    "ID LP Args RP",
    "ID LP RP"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  const ParseTNode *child = getChildByName(node, "ID");
  const char *name = child->value.str_value;
  const int lineNum = child->lineNum;
  if (searchEntireScopeWithName(currentEnv, name) != NULL) {
    error(11, lineNum, "\"%s\" is not a function.\n", name);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  const Data *data = searchWithName(funcMap, name);
  if (data == NULL) { // not find function definition
    error(2, lineNum, "Undefined function \"%s\".\n", name);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  assert(data->kind == FUNC);
  if (i == 0) { // deal with args
    ParamGather *gather = NULL;
    resolveArgs(getChildByName(node, "Args"), &gather);
    assert(gather != NULL);
    // reverse gather because it adds from head
    reverseParamGather(&gather);
    if (!checkFuncCallArgs(data, gather)) {
      const char *strType = dataToString(data);
      error(9, lineNum, "Function %s is not applicable for arguments.\n", strType);
      free((char *) strType);
      // error occurs, but still returns the correct type
    }
    freeParamGather(gather);
  }
  return deepCopyType(data->function.returnType);
}

/**
 * @brief helper function for 'resolveExp'
 * @warning may return ERROR type
 * @return a copy of element type in struct.
*/
const Type* evalStruct(const ParseTNode *node) {
  const char *expressions[] = {
    "Exp DOT ID"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  const Type *expType = resolveExp(getChildByName(node, "Exp"));
  if (expType->kind == ERROR) return expType;
  if (expType->kind != STRUCT) {
    const int lineNum = getChildByName(node, "DOT")->lineNum;
    error(13, lineNum, "Illegal use of \".\".\n");
    freeType((Type *) expType);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  // get the name of identifier
  const char *name = getChildByName(node, "ID")->value.str_value;
  // loop through the field of struct
  const structFieldElement *f = expType->structure.fields;
  while (f != NULL) {
    if (strcmp(f->name, name) == 0) break;
    f = f->next;
  }
  if (f == NULL) {
    const int lineNum = getChildByName(node, "DOT")->lineNum;
    error(14, lineNum, "Non-existent field \"%s\".\n", name);
    freeType((Type *) expType);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  assert(strcmp(f->name, name) == 0);
  const Type *copy = deepCopyType(f->elemType);
  freeType((Type *) expType);
  return copy;
}

/**
 * @brief helper function for 'resolveExp'
 * @warning may return ERROR type
 * @return a copy of element type in struct.
*/
const Type* evalArray(const ParseTNode *node) {
  const char *expressions[] = {
    "Exp LB Exp RB"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  const ParseTNode *firstExp = node->children.container[0];
  const Type *first = resolveExp(firstExp);
  if (first->kind == ERROR) return first;
  if (first->kind != ARRAY) {
    error(10, firstExp->lineNum, "is not an array.\n");
    freeType((Type *) first);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  const ParseTNode *secondExp = node->children.container[2];
  const Type *second = resolveExp(secondExp);
  if (second->kind != INT) {
    error(12, secondExp->lineNum, "is not an integer.\n");
    freeType((Type *) first);
    freeType((Type *) second);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }
  const Type *rst = deepCopyType(first->array.elemType);
  freeType((Type *) first);
  freeType((Type *) second);
  return rst;
}

/**
 * @warning may return ERROR type
 * @return a copy of type for this expression
*/
const Type* resolveExp(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Exp") == 0);
  const char *expressions[] = {
    "Exp ASSIGNOP Exp", // 0
    "Exp AND Exp",      // 1
    "Exp OR Exp",       // 2
    "Exp RELOP Exp",    // 3
    "Exp PLUS Exp",     // 4
    "Exp MINUS Exp",    // 5
    "Exp STAR Exp",     // 6
    "Exp DIV Exp",      // 7
    "LP Exp RP",        // 8
    "MINUS Exp",        // 9
    "NOT Exp",          // 10
    "ID LP Args RP",    // 11
    "ID LP RP",         // 12
    "Exp LB Exp RB",    // 13
    "Exp DOT ID",       // 14
    "ID",               // 15
    "INT",              // 16
    "FLOAT"             // 17
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i <= 7) return evalBinaryOperator(node);
  if (i == 8) return resolveExp(getChildByName(node, "Exp"));
  if (i == 9) {
    const Type *expType = resolveExp(getChildByName(node, "Exp"));
    if (expType->kind == ERROR) return expType;
    if (!(expType->kind == INT || expType->kind == FLOAT)) {
      const int lineNum = getChildByName(node, "MINUS")->lineNum;
      error(7, lineNum, "Type mismatched for operands.\n");
      freeType((Type *) expType);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    return expType;
  }
  if (i == 10) {
    const Type *expType = resolveExp(getChildByName(node, "Exp"));
    if (expType->kind == ERROR) return expType;
    if (expType->kind != INT) {
      const int lineNum = getChildByName(node, "NOT")->lineNum;
      error(7, lineNum, "Type mismatched for operands.\n");
      freeType((Type *) expType);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    return expType;
  }
  if (i == 11 || i == 12) return evalFuncInvocation(node);
  if (i == 13) return evalArray(node);
  if (i == 14) return evalStruct(node);
  if (i == 15) {
    const char *name = getChildByName(node, "ID")->value.str_value;
    assert(currentEnv->kind == COMPOUND || currentEnv->kind == GLOBAL); // not in struct env
    const Data *d = searchEntireScopeWithName(currentEnv, name);
    if (d == NULL) {
      const int lineNum = getChildByName(node, "ID")->lineNum;
      error(1, lineNum, "Undefined variable \"%s\".\n", name);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    assert(d->kind == VAR);
    return deepCopyType(d->variable.type);
  }
  if (i == 16 || i == 17) return createBasicTypeOfNode(node->children.container[0]);
}

// simply gather all arguments, regardless is error or not.
void resolveArgs(const ParseTNode *node, ParamGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"Args") == 0);
  const char *expressions[] = {
    "Exp",
    "Exp COMMA Args"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  const Type *expType = resolveExp(getChildByName(node, "Exp"));
  // temporary data
  Data *data = malloc(sizeof(Data));
  data->kind = VAR;
  data->name = my_strdup("");
  data->variable.type = (Type *) expType; // directly pointing to expType, so no need to free
  const int lineNum = getChildByName(node, "Exp")->lineNum;
  gatherParamInfo(gather, data, lineNum);
  freeData(data);
  if (i == 1) {
    resolveArgs(getChildByName(node, "Args"), gather);
  }
}

/**
 * @brief relay function
*/
void resolveDec(const ParseTNode *node, DecGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"Dec") == 0);
  const char *expressions[] = {
    "VarDec",
    "VarDec ASSIGNOP Exp"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  resolveVarDec(getChildByName(node, "VarDec"), gather);
  if (i == 1 && currentEnv->kind == STRUCTURE) {
    const int lineNum = getChildByName(node, "ASSIGNOP")->lineNum;
    error(15, lineNum, "initializing a field in structure.\n");
  }
}

void resolveDecList(const ParseTNode *node, DecGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"DecList") == 0);
  const char *expressions[] = {
    "Dec",
    "Dec COMMA DecList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  resolveDec(getChildByName(node, "Dec"), gather);
  if (i == 1) {
    resolveDecList(getChildByName(node, "DecList"), gather);
  }
}

/**
 * @brief check initialization type.
 * manually traverse down parse tree to check assignment.
 * Helper function for 'resolveCompSt'
*/
void checkInit(const ParseTNode *CompSt_node) {
  assert(CompSt_node != NULL && strcmp(CompSt_node->name,"CompSt") == 0);
  const char *CompSt_expressions[] = {"LC DefList StmtList RC"};
  assert(matchExprPattern(CompSt_node, CompSt_expressions, ARRAY_LEN(CompSt_expressions)) == 0);
  const ParseTNode *DefList_node = getChildByName(CompSt_node, "DefList");
  const char *DefList_expressions[] = {
    "",
    "Def DefList"
  };
  while (matchExprPattern(DefList_node, DefList_expressions,ARRAY_LEN(DefList_expressions)) == 1) {
    const ParseTNode *Def_node = getChildByName(DefList_node, "Def");
    const char *Def_expressions[] = {"Specifier DecList SEMI"};
    assert(matchExprPattern(Def_node,Def_expressions,ARRAY_LEN(Def_expressions)) == 0);
    const ParseTNode *DecList_node = getChildByName(Def_node, "DecList");
    const char *DecList_expressions[] = {
      "Dec",
      "Dec COMMA DecList"
    };
    while (1) {
      const ParseTNode *Dec_node = getChildByName(DecList_node, "Dec");
      const char *Dec_expressions[] = {
        "VarDec",
        "VarDec ASSIGNOP Exp"
      };
      if (matchExprPattern(Dec_node, Dec_expressions,ARRAY_LEN(Dec_expressions)) == 1) {
        assert(currentEnv->kind == COMPOUND);
        const Type *type = resolveExp(getChildByName(Dec_node, "Exp"));
        assert(type != NULL);
        const Type *expect; {
          DecGather *gather = NULL; // get the variable name using dec gather
          resolveVarDec(getChildByName(Dec_node, "VarDec"), &gather);
          assert(gather != NULL && gather->next == NULL); // only get one
          const Data *d = searchWithName(currentEnv->vMap, gather->name);
          // searching in current scope is enough
          assert(d != NULL && d->kind == VAR);
          expect = deepCopyType(d->variable.type);
          freeDecGather(gather);
        }
        // ignore error, because it has been dealt with.
        if (type->kind != ERROR && !typeEqual(expect, type)) {
          const int lineNum = getChildByName(Dec_node, "ASSIGNOP")->lineNum;
          error(5, lineNum, "Type mismatched for assignment.\n");
        }
      }
      if (matchExprPattern(DecList_node, DecList_expressions,ARRAY_LEN(DecList_expressions)) == 0) {
        break;
      }
      DecList_node = getChildByName(DecList_node, "DecList");
    }

    DefList_node = getChildByName(DefList_node, "DefList");
  }
}

void resolveDef(const ParseTNode *node, ParamGather **param_gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"Def") == 0);
  const char *expressions[] = {
    "Specifier DecList SEMI"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  const Type *base_type = resolveSpecifier(getChildByName(node, "Specifier"));
  if (base_type->kind == ERROR) {
    freeType((Type *) base_type);
    return;
  }
  DecGather *dec_gather = NULL;
  resolveDecList(getChildByName(node, "DecList"), &dec_gather);
  assert(dec_gather != NULL);
  const DecGather *g = dec_gather;
  // loop through dec gather and construct param gather
  while (g != NULL) {
    Data *d = malloc(sizeof(Data));
    d->kind = VAR;
    d->name = my_strdup(g->name);
    d->variable.type = (Type *) turnDecGather2Type(g, base_type);
    gatherParamInfo(param_gather, d, g->lineNum);
    freeData(d);
    g = g->next;
  }
  freeDecGather(dec_gather);
  freeType((Type *) base_type);
}

/**
 * @brief relay function between 'Def' and ('CompSt', '')
 * @note the sequence in param gather is the reverse order of the original
*/
void resolveDefList(const ParseTNode *node, ParamGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"DefList") == 0);
  const char *expressions[] = {
    "",
    "Def DefList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 0) return;
  resolveDef(getChildByName(node, "Def"), gather);
  resolveDefList(getChildByName(node, "DefList"), gather);
}

void resolveStmt(const ParseTNode *node, const Type *returnType) {
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
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 1) {
    resolveCompSt(getChildByName(node, "CompSt"), returnType);
    return;
  }
  // i == 0 do nothing
  const Type *expType = resolveExp(getChildByName(node, "Exp"));
  if (i == 2 && (expType->kind == ERROR || !typeEqual(returnType, expType))) {
    // not tolerate error in here
    const int lineNum = getChildByName(node, "RETURN")->lineNum;
    error(8, lineNum, "Type mismatched for return.\n");
  } else if (3 <= i && i <= 5) {
    if (expType->kind != INT) {
      // not tolerate error in here
      const int lineNum = getChildByName(node, "Exp")->lineNum;
      error(7, lineNum, "Type mismatched for operands.\n");
    }
    resolveStmt(node->children.container[4], returnType);
    if (i == 4) {
      resolveStmt(node->children.container[6], returnType);
    }
  }
  freeType((Type *) expType);
}

// relay function between 'CompSt' and 'Stmt'
void resolveStmtList(const ParseTNode *node, const Type *returnType) {
  assert(node != NULL);
  assert(strcmp(node->name,"StmtList") == 0);
  const char *expressions[] = {
    "",
    "Stmt StmtList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 0) return;
  // i == 1
  resolveStmt(getChildByName(node, "Stmt"), returnType);
  resolveStmtList(getChildByName(node, "StmtList"), returnType);
}

/**
 * @param node
 * @param returnType the expect return type from 'CompSt'
*/
void resolveCompSt(const ParseTNode *node, const Type *returnType) {
  assert(node != NULL);
  assert(strcmp(node->name,"CompSt") == 0);
  const char *expressions[] = {
    "LC DefList StmtList RC"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  // add another layer of environment
  currentEnv = newEnvironment(currentEnv, COMPOUND);
  ParamGather *gather = NULL;
  resolveDefList(getChildByName(node, "DefList"), &gather);
  // reverse param gather, because it adds from head
  reverseParamGather(&gather);
  const ParamGather *g = gather;
  // loop through gather to define variables
  while (g != NULL) {
    const Data *data = g->data;
    assert(data->kind == VAR);
    if (searchWithName(currentEnv->vMap, data->name) != NULL ||
        findDefinedStruct(definedStructList, data->name) != NULL) { // haven't defined OR collide with struct name
      error(3, g->lineNum, "Redefined variable \"%s\".\n", data->name);
    } else {
      insert(currentEnv->vMap, (Data *) deepCopyData(data));
    }
    g = g->next;
  }
  // check assignment(initialization)
  checkInit(node);

  freeParamGather(gather);
  resolveStmtList(getChildByName(node, "StmtList"), returnType);
  revertEnvironment(&currentEnv);
}

void resolveParamDec(const ParseTNode *node, ParamGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"ParamDec") == 0);
  const char *expressions[] = {
    "Specifier VarDec"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  const Type *type = resolveSpecifier(getChildByName(node, "Specifier"));
  if (type->kind == ERROR) {
    freeType((Type *) type);
    return;
  }
  DecGather *decGather = NULL;
  resolveVarDec(getChildByName(node, "VarDec"), &decGather);
  assert(decGather != NULL);
  assert(decGather->next == NULL); // only gather one variable
  // temporary data
  Data *data = malloc(sizeof(Data));
  data->name = my_strdup(decGather->name);
  data->kind = VAR;
  data->variable.type = (Type *) turnDecGather2Type(decGather, type);
  const int lineNum = getChildByName(node, "VarDec")->lineNum;
  gatherParamInfo(gather, data, lineNum);

  freeData(data);
  freeDecGather(decGather);
  freeType((Type *) type); // suppress warning
}

/**
 * @brief relay function between 'FunDec' and 'ParamDec'
 * @return number of parameters
 */
int resolveVarList(const ParseTNode *node, ParamGather **gather, const int n) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarList") == 0);
  const char *expressions[] = {
    "ParamDec",
    "ParamDec COMMA VarList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  resolveParamDec(getChildByName(node, "ParamDec"), gather);
  if (i == 0) return n;
  return resolveVarList(getChildByName(node, "VarList"), gather, n + 1);
}

/**
 * @brief gather the variable list names and its types and function name
 * @return a number of parameters
 * @note name is a new copy. And guarantee that the sequence of vars is the same as theirs in gather.
*/
int resolveFunDec(const ParseTNode *node, ParamGather **gather, char **name) {
  assert(node != NULL);
  assert(strcmp(node->name,"FunDec") == 0);
  const char *expressions[] = {
    "ID LP VarList RP",
    "ID LP RP"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  assert(*name == NULL); // name should be NULL when passed in
  *name = my_strdup(getChildByName(node, "ID")->value.str_value);
  int num = 0;
  if (i == 0) {
    num = resolveVarList(getChildByName(node, "VarList"), gather, 1);
  }
  reverseParamGather(gather);
  return num;
}

void resolveVarDec(const ParseTNode *node, DecGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"VarDec") == 0);
  const char *expressions[] = {
    "ID",
    "VarDec LB INT RB"
  };
  // flatten array list and add to gather
  int dimension = 0;
  int size_list[MAX_DIMENSION];
  // 'ID' skips this while loop
  while (matchExprPattern(node, expressions,ARRAY_LEN(expressions)) != 0) {
    if (dimension >= MAX_DIMENSION) {
      fprintf(stderr, "The dimension of array can't exceed %d\n",MAX_DIMENSION);
      exit(EXIT_FAILURE);
    }
    size_list[dimension] = getChildByName(node, "INT")->value.int_value;
    dimension++;
    node = getChildByName(node, "VarDec");
  }
  const ParseTNode *IDNode = getChildByName(node, "ID");
  const char *name = IDNode->value.str_value;
  // change the sequence of size in size_list
  reverseArray(size_list, dimension);
  gatherDecInfo(gather, name, dimension, size_list, IDNode->lineNum);
}

/**
 * @return a copy of string.
*/
const char* resolveTag(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Tag") == 0);
  const char *expressions[] = {
    "ID"
  };
  assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
  return my_strdup(getChildByName(node, "ID")->value.str_value);
}

/**
 * @brief if is ID get its name; else get a random string.
 * @return a copy of name.
*/
const char* resolveOptTag(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"OptTag") == 0);
  const char *expressions[] = {
    "",
    "ID"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 0) return randomString(5, "-struct");
  return my_strdup(getChildByName(node, "ID")->value.str_value);
}

/**
 * @brief construct and retrieve OR simply retrieve struct type
 * @warning may return ERROR type.
 * @return a copy of type.
*/
const Type* resolveStructSpecifier(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"StructSpecifier") == 0);
  const char *expressions[] = {
    "STRUCT OptTag LC DefList RC",
    "STRUCT Tag"
  };
  // use environment to check duplication
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 1) {
    const char *name = resolveTag(getChildByName(node, "Tag"));
    const Type *type = findDefinedStruct(definedStructList, name);
    if (type == NULL) { // not register in defined list
      const int lineNum = getChildByName(node, "Tag")->lineNum;
      error(17, lineNum, "Undefined structure \"%s\".\n", name);
      free((char *) name);
      Type *t = malloc(sizeof(Type));
      t->kind = ERROR;
      return t;
    }
    assert(type->kind == STRUCT);
    free((char *) name);
    return deepCopyType(type);
  }
  const char *name = resolveOptTag(getChildByName(node, "OptTag"));
  if (findDefinedStruct(definedStructList, name) != NULL ||
      searchWithName(currentEnv->vMap, name) != NULL) {
    // collide with defined struct OR variable name in current env
    const int lineNum = getChildByName(node, "STRUCT")->lineNum;
    error(16, lineNum, "Duplicated name \"%s\".\n", name);
    free((char *) name);
    Type *t = malloc(sizeof(Type));
    t->kind = ERROR;
    return t;
  }

  currentEnv = newEnvironment(currentEnv, STRUCTURE); // mainly for checking duplication
  ParamGather *gather = NULL;
  resolveDefList(getChildByName(node, "DefList"), &gather);
  // reverse param gather, because it adds from head
  reverseParamGather(&gather);

  const ParamGather *g = gather;
  Type *struct_type = malloc(sizeof(Type));
  struct_type->kind = STRUCT;
  struct_type->structure.struct_name = (char *) name; // directly pointing to name, so no need to free.
  structFieldElement **f = &struct_type->structure.fields;
  // loop through param gather to construct STRUCT
  while (g != NULL) {
    const Data *data = g->data;
    assert(data->kind == VAR);
    if (searchWithName(currentEnv->vMap, data->name) != NULL) { // duplicate variable in current field
      error(15, g->lineNum, "Redefined field \"%s\".\n", data->name);
      g = g->next;
      continue;
    }
    *f = malloc(sizeof(structFieldElement));
    (*f)->name = my_strdup(data->name);
    (*f)->elemType = (Type *) deepCopyType(data->variable.type);
    f = &(*f)->next;
    insert(currentEnv->vMap, (Data *) deepCopyData(data)); // register in struct map
    g = g->next;
  }
  *f = NULL;
  addDefinedStruct(&definedStructList, struct_type); // register struct type in defined list
  revertEnvironment(&currentEnv);
  freeParamGather(gather);
  return struct_type;
}

/**
 * @warning may return ERROR type.
 * @return a copy of Type
*/
const Type* resolveSpecifier(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"Specifier") == 0);
  const char *expressions[] = {
    "TYPE",
    "StructSpecifier"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 0) return createBasicTypeOfNode(getChildByName(node, "TYPE"));
  return resolveStructSpecifier(getChildByName(node, "StructSpecifier"));
}

/**
 * @brief relay function between 'VarDec' and 'ExtDef'
*/
void resolveExtDecList(const ParseTNode *node, DecGather **gather) {
  assert(node != NULL);
  assert(strcmp(node->name,"ExtDecList") == 0);
  const char *expressions[] = {
    "VarDec",
    "VarDec COMMA ExtDecList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  resolveVarDec(getChildByName(node, "VarDec"), gather);
  assert(*gather != NULL);
  if (i == 1) resolveExtDecList(getChildByName(node, "ExtDecList"), gather);
}

// todo extract into separate functions
void resolveExtDef(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name, "ExtDef") == 0);
  const char *expressions[] = {
    "Specifier SEMI",
    "Specifier ExtDecList SEMI",
    "Specifier FunDec CompSt"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  const Type *type = resolveSpecifier(getChildByName(node, "Specifier"));
  assert(type != NULL);
  if (type->kind == ERROR || i == 0) {
    freeType((Type *) type);
    return;
  }
  if (i == 1) {
    DecGather *gather = NULL;
    resolveExtDecList(getChildByName(node, "ExtDecList"), &gather);
    // loop through gather
    const DecGather *g = gather;
    assert(currentEnv != NULL && currentEnv->kind == GLOBAL); // in global environment
    while (g != NULL) {
      // check duplication in current(global) environment
      if (searchWithName(currentEnv->vMap, g->name) != NULL ||
          findDefinedStruct(definedStructList, g->name) != NULL) {
        error(3, g->lineNum, "Redefined variable \"%s\".\n", g->name);
        g = g->next;
        continue;
      }
      Data *data = malloc(sizeof(Data));
      data->name = my_strdup(g->name);
      data->kind = VAR;
      data->variable.type = (Type *) turnDecGather2Type(g, type);

      insert(currentEnv->vMap, data);
      g = g->next;
    }
    freeDecGather(gather);
    freeType((Type *) type);
    return;
  } // i == 2
  ParamGather *gather = NULL;
  char *name = NULL;
  const int argc = resolveFunDec(getChildByName(node, "FunDec"), &gather, &name);
  assert(name != NULL); // function name should be obtained
  assert(funcMap != NULL);
  if (searchWithName(funcMap, name) != NULL) {
    const int lineNum = getChildByName(node, "FunDec")->lineNum;
    error(4, lineNum, "Redefined function \"%s\".\n", name);
    free(name);
    freeParamGather(gather);
    freeType((Type *) type); // suppress warning
    return;
  }
  Data *func_data = malloc(sizeof(Data));
  func_data->kind = FUNC;
  func_data->name = name; // directly pointing to name, so no need to free
  func_data->function.argc = argc;
  func_data->function.returnType = (Type *) deepCopyType(type);
  func_data->function.argvTypes = malloc(sizeof(Type *) * argc);
  // loop through paramGather to fill up argvTypes
  const ParamGather *g = gather;
  int j = 0;
  while (g != NULL) {
    assert(g->data->kind == VAR); // all parameters collected should be variables
    func_data->function.argvTypes[j] = (Type *) deepCopyType(g->data->variable.type);
    j++;
    g = g->next;
  }
  insert(funcMap, func_data);
  currentEnv = newEnvironment(currentEnv, COMPOUND);
  // register all parameters in new environment
  g = gather; // loop through paramGather again
  while (g != NULL) {
    const Data *data = g->data;
    assert(data->kind == VAR && currentEnv != NULL);
    if (searchWithName(currentEnv->vMap, data->name) != NULL ||
        findDefinedStruct(definedStructList, data->name) != NULL) { // duplication in parameters
      error(3, g->lineNum, "Redefined variable \"%s\".\n", data->name);
      Data *tmp = malloc(sizeof(Data));
      tmp->kind = VAR;
      // generate a random new name, and leave type as it is
      tmp->variable.type = (Type *) deepCopyType(data->variable.type);
      tmp->name = (char *) randomString(5, "-param");
      insert(currentEnv->vMap, tmp);
    } else {
      insert(currentEnv->vMap, (Data *) deepCopyData(g->data)); // reminder: copy data
    }
    g = g->next;
  }
  resolveCompSt(getChildByName(node, "CompSt"), type);
  revertEnvironment(&currentEnv);
  freeParamGather(gather);
  freeType((Type *) type); // suppress warning
}

void resolveExtDefList(const ParseTNode *node) {
  assert(node != NULL);
  assert(strcmp(node->name,"ExtDefList") == 0);
  const char *expressions[] = {
    "",
    "ExtDef ExtDefList"
  };
  const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
  if (i == 1) {
    resolveExtDef(getChildByName(node, "ExtDef"));
    resolveExtDefList(getChildByName(node, "ExtDefList"));
  }
}


void buildTable(const ParseTNode *root) {
  assert(root != NULL);
  assert(strcmp(root->name,"Program") == 0);
  char *expressions[] = {
    "ExtDefList"
  };
  definedStructList = NULL;
  currentEnv = newEnvironment(NULL, GLOBAL);
  funcMap = createRedBlackTree();
  assert(matchExprPattern(root, expressions, ARRAY_LEN(expressions)) == 0);
  resolveExtDefList(getChildByName(root, "ExtDefList"));
  assert(currentEnv->kind == GLOBAL);
  freeEnvironment(currentEnv);
  freeRedBlackTree(funcMap);
  freeDefinedStructList(definedStructList);
}

#ifdef SYMBOL_test
int main() {
}
#endif
