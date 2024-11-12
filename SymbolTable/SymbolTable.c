#include "SymbolTable.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

Environment* currentEnv = NULL;
Environment* globalEnv = NULL;
// register all defined functions
RedBlackTree* funcMap = NULL;
// a linked list registers those defined struct type
StructTypeWrapper* definedStructList = NULL;

/**
 * @brief helper function for 'resolveExp'
 * @return a copy of type of expression
*/
const Type* resolveBinaryOperator(const ParseTNode* node) {
    const char* expressions[] = {
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
    const Type* type1 = resolveExp(node->children.container[0]);
    if (type1->kind == ERROR) return type1;
    const Type* type2 = resolveExp(node->children.container[2]);
    if (type2->kind == ERROR) {
        freeType((Type*)type1);
        return type2;
    }
    if (i == 0) { // assignment
        if (!typeEqual(type1, type2)) {
            const int lineNum = getChildByName(node, "ASSIGNOP")->lineNum;
            error(5, lineNum, "Type mismatched for assignment.\n");
            freeType((Type*)type1);
            freeType((Type*)type2);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        if (!(nodeChildrenNameEqualHelper(node->children.container[0], "ID") ||
              nodeChildrenNameEqualHelper(node->children.container[0], "Exp LB Exp RB") ||
              nodeChildrenNameEqualHelper(node->children.container[0], "Exp DOT ID"))) {
            const int lineNum = getChildByName(node, "ASSIGNOP")->lineNum;
            error(6, lineNum, "The left-hand side of an assignment must be a variable.\n");
            freeType((Type*)type1);
            freeType((Type*)type2);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        freeType((Type*)type1);
        return type2;
    }
    if (1 <= i && i <= 3) { // logical operations
        if (type1->kind != INT || type2->kind != INT) {
            const int lineNum = node->children.container[1]->lineNum;
            error(7, lineNum, "Type mismatched for operands.\n");
            freeType((Type*)type1);
            freeType((Type*)type2);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        freeType((Type*)type1);
        return type2;
    } // arithmatic operations: 4 <= i && i <= 7
    if (!((type1->kind == INT || type1->kind == FLOAT) && typeEqual(type1, type2))) {
        const int lineNum = node->children.container[1]->lineNum;
        error(7, lineNum, "Type mismatched for operands.\n");
        freeType((Type*)type1);
        freeType((Type*)type2);
        Type* t = malloc(sizeof(Type));
        t->kind = ERROR;
        return t;
    }
    freeType((Type*)type1);
    return type2;
}

/**
 * @brief helper function for 'resolveExp'
 * @return a copy of type of function's return type.
*/
const Type* resolveFuncInvocation(const ParseTNode* node) {
    const char* expressions[] = {
        "ID LP Args RP",
        "ID LP RP"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));

    const ParseTNode* child = getChildByName(node, "ID");
    const char* name = child->value.str_value;
    const int lineNum = child->lineNum;
    const Data* data = searchWithName(funcMap, name);
    if (data == NULL) { // not find function definition
        char buffer[50];
        sprintf(buffer, "Undefined function \"%s\".\n", name);
        error(2, lineNum, buffer);
        Type* t = malloc(sizeof(Type));
        t->kind = ERROR;
        return t;
    }
    assert(data->kind == FUNC);
    if (i == 0) { // deal with args
        ParamGather* gather = NULL;
        resolveArgs(getChildByName(node, "Args"), &gather);
        assert(gather != NULL);
        if (!checkFuncCallArgs(data, gather)) {
            const char* strType = dataToString(data);
            const char buffer[80];
            sprintf(buffer, "Function %s is not applicable for arguments.\n", strType);
            error(9, lineNum, buffer);
            free((char*)strType);
            // error occurs, but still return the correct type
        }
        freeParamGather(gather);
    }
    return deepCopyType(data->function.returnType);
}

/**
 * @return a copy of type for this expression
 * @note the default type to leave is left expression type.
*/
const Type* resolveExp(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Exp") == 0);
    const char* expressions[] = {
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
    if (i <= 7) return resolveBinaryOperator(node);
    if (i == 8) return resolveExp(getChildByName(node, "Exp"));
    if (i == 9) {
        const Type* expType = resolveExp(getChildByName(node, "Exp"));
        if (!(expType->kind == INT || expType->kind == FLOAT)) {
            const int lineNum = getChildByName(node, "MINUS")->lineNum;
            error(7, lineNum, "Type mismatched for operands.\n");
            freeType((Type*)expType);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        return expType;
    }
    if (i == 10) {
        const Type* expType = resolveExp(getChildByName(node, "Exp"));
        if (expType->kind != INT) {
            const int lineNum = getChildByName(node, "NOT")->lineNum;
            error(7, lineNum, "Type mismatched for operands.\n");
            freeType((Type*)expType);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        return expType;
    }
    if (i == 11 || i == 12) return resolveFuncInvocation(node);
    if (i == 13 || i == 14) {
        // todo struct
        return;
    }
    if (i == 15) {
        const char* name = getChildByName(node, "ID")->value.str_value;
        const Data* d = searchEntireScopeWithName(currentEnv, name);
        if (d == NULL) {
            const int lineNum = getChildByName(node, "ID")->lineNum;
            char buffer[50];
            sprintf(buffer, "Undefined variable \"%s\".\n", name);
            error(1, lineNum, buffer);
            Type* t = malloc(sizeof(Type));
            t->kind = ERROR;
            return t;
        }
        assert(d->kind == VAR);
        return deepCopyType(d->variable.type);
    }
    if (i == 16 || i == 17) {
        return createBasicTypeOfNode(node->children.container[0]);
    }
}

void resolveArgs(const ParseTNode* node, ParamGather** gather) {
    assert(node != NULL);
    assert(strcmp(node->name,"Args") == 0);
    const char* expressions[] = {
        "Exp",
        "Exp COMMA Args"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    const Type* expType = resolveExp(getChildByName(node, "Exp"));
    // temporary data
    Data* data = malloc(sizeof(Data));
    data->kind = VAR;
    data->name = my_strdup("");
    data->variable.type = (Type*)expType; // directly pointing to expType, so no need to free
    const int lineNum = getChildByName(node, "Exp")->lineNum;
    gatherParamInfo(gather, data, lineNum);
    freeData(data);
    if (i == 1) {
        resolveArgs(getChildByName(node, "Args"), gather);
    }
}

/**
 * @return if encounter assignment, a copy of type; else NULL
*/
const Type* resolveDec(const ParseTNode* node, DecGather** gather) {
    assert(node != NULL);
    assert(strcmp(node->name,"Dec") == 0);
    const char* expressions[] = {
        "VarDec",
        "VarDec ASSIGNOP Exp"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    resolveVarDec(getChildByName(node, "VarDec"), gather);
    const Type* expType = NULL;
    if (i == 1) {
        expType = resolveExp(getChildByName(node, "Exp"));
    }
    return expType;
}

void resolveDecList(const ParseTNode* node, const Type* base_type) {
    assert(node != NULL);
    assert(strcmp(node->name,"DecList") == 0);
    const char* expressions[] = {
        "Dec",
        "Dec COMMA DecList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    DecGather* gather = NULL;
    const Type* expType = resolveDec(getChildByName(node, "Dec"), &gather);
    assert(gather != NULL && gather->next == NULL); // only gather one

    assert(currentEnv != NULL && currentEnv->kind != GLOBAL); // not global environment
    // search in current env
    const Data* d = searchWithName(currentEnv->vMap, gather->name);
    // todo check name with those defined struct names
    if (d != NULL) {
        assert(d->kind == VAR);
        const int lineNum = getChildByName(node, "Dec")->lineNum;
        char buffer[50];
        sprintf(buffer, "Redefined variable \"%s\".\n", gather->name);
        error(3, lineNum, buffer);
    } else { // haven't defined
        // todo check name with defined structure names
        const Type* type = turnDecGather2Type(gather, base_type);
        // todo check error type
        if (expType != NULL && !typeEqual(type, expType)) {
            const int lineNum = getChildByName(node, "Dec")->lineNum;
            error(5, lineNum, "Type mismatched for assignment.\n");
        }
        // temporary data
        Data* data = malloc(sizeof(Data));
        data->kind = VAR;
        data->name = my_strdup(gather->name);
        data->variable.type = (Type*)type; // directly pointing to type, so no need to free
        insert(currentEnv->vMap, data);
    }
    if (expType != NULL) freeType((Type*)expType);
    freeDecGather(gather);

    if (i == 1) {
        resolveDecList(getChildByName(node, "DecList"), base_type);
    }
}

void resolveDef(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Def") == 0);
    const char* expressions[] = {
        "Specifier DecList SEMI"
    };
    assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
    const Type* type = resolveSpecifier(getChildByName(node, "Specifier"));
    resolveDecList(getChildByName(node, "DecList"), type);
    freeType((Type*)type);
}

void resolveDefList(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"DefList") == 0);
    const char* expressions[] = {
        "",
        "Def DefList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    if (i == 0) return;
    //i == 1
    resolveDef(getChildByName(node, "Def"));
    resolveDefList(getChildByName(node, "DefList"));
}

void resolveStmt(const ParseTNode* node, const Type* returnType) {
    assert(node != NULL);
    assert(strcmp(node->name,"Stmt") == 0);
    const char* expressions[] = {
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
    const Type* expType = resolveExp(getChildByName(node, "Exp"));
    if (i == 2 && !typeEqual(returnType, expType)) {
        const int lineNum = getChildByName(node, "RETURN")->lineNum;
        error(8, lineNum, "Type mismatched for return.\n");
    } else if (3 <= i && i <= 5) {
        if (expType->kind != INT) {
            // todo tolerant error
            const int lineNum = getChildByName(node, "Exp")->lineNum;
            error(7, lineNum, "Type mismatched for operands.\n");
        }
        resolveStmt(node->children.container[4], returnType);
        if (i == 4) {
            resolveStmt(node->children.container[6], returnType);
        }
    }
    freeType((Type*)expType);
}

// relay function between 'CompSt' and 'Stmt'
void resolveStmtList(const ParseTNode* node, const Type* returnType) {
    assert(node != NULL);
    assert(strcmp(node->name,"StmtList") == 0);
    const char* expressions[] = {
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
void resolveCompSt(const ParseTNode* node, const Type* returnType) {
    assert(node != NULL);
    assert(strcmp(node->name,"CompSt") == 0);
    const char* expressions[] = {
        "LC DefList StmtList RC"
    };
    assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
    resolveDefList(getChildByName(node, "DefList"));
    resolveStmtList(getChildByName(node, "StmtList"), returnType);
}

void resolveParamDec(const ParseTNode* node, ParamGather** gather) {
    assert(node != NULL);
    assert(strcmp(node->name,"ParamDec") == 0);
    const char* expressions[] = {
        "Specifier VarDec"
    };
    assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
    const Type* type = resolveSpecifier(getChildByName(node, "Specifier"));

    DecGather* decGather = NULL;
    resolveVarDec(getChildByName(node, "VarDec"), &decGather);
    assert(decGather != NULL);
    assert(decGather->next == NULL); // only gather one variable
    // temporary data
    Data* data = malloc(sizeof(Data));
    data->name = my_strdup(decGather->name);
    data->kind = VAR;
    data->variable.type = (Type*)turnDecGather2Type(decGather, type);
    const int lineNum = getChildByName(node, "VarDec")->lineNum;
    gatherParamInfo(gather, data, lineNum);

    freeData(data);
    freeDecGather(decGather);
    freeType((Type*)type); // suppress warning
}

/**
 * @brief relay function between 'FunDec' and 'ParamDec'
 * @return number of parameters
 */
int resolveVarList(const ParseTNode* node, ParamGather** gather, const int n) {
    assert(node != NULL);
    assert(strcmp(node->name,"VarList") == 0);
    const char* expressions[] = {
        "ParamDec",
        "ParamDec COMMA VarList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    resolveParamDec(getChildByName(node, "ParamDec"), gather);
    if (i == 1) {
        resolveVarList(getChildByName(node, "VarList"), gather, n + 1);
    }
    return n;
}

/**
 * @brief gather the variable list names and its types and function name
 * @return a number of parameters
 * @note name is a new copy.
*/
int resolveFunDec(const ParseTNode* node, ParamGather** gather, char** name) {
    assert(node != NULL);
    assert(strcmp(node->name,"FunDec") == 0);
    const char* expressions[] = {
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
    return num;
}

void resolveVarDec(const ParseTNode* node, DecGather** gather) {
    assert(node != NULL);
    assert(strcmp(node->name,"VarDec") == 0);
    const char* expressions[] = {
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
    const ParseTNode* IDNode = getChildByName(node, "ID");
    const char* name = IDNode->value.str_value;
    // change the sequence of size in size_list
    reverseArray(size_list, dimension);
    gatherDecInfo(gather, name, dimension, size_list, IDNode->lineNum);
}

const char* resolveTag(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Tag") == 0);
    const char* expressions[] = {
        "ID"
    };
    assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
    return my_strdup(getChildByName(node, "ID")->value.str_value);
}

const char* resolveOptTag(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"OptTag") == 0);
    const char* expressions[] = {
        "",
        "ID"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    if (i == 0) {
        return NULL;
    }
    return my_strdup(getChildByName(node, "ID")->value.str_value);
}

const Type* resolveStructSpecifier(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"StructSpecifier") == 0);
    const char* expressions[] = {
        "STRUCT OptTag LC DefList RC",
        "STRUCT Tag"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    Type* rt_type = NULL;
    if (i == 0) {
        char* name = resolveOptTag(getChildByName(node, "OptTag"));
        if (name == NULL) {
            name = randomString(5, "-struct");
        }
        rt_type = malloc(sizeof(Type));
        rt_type->kind = STRUCT;
        rt_type->structure.struct_name = name;
        // rt_type->structure.fields = resolveDefList(getChildByName(node, "DefList"));
        // todo add this type to defined struct type list
        // todo deep copy struct type?
    } else if (i == 1) {
        char* name = resolveTag(getChildByName(node, "Tag"));
        rt_type = findDefinedStructType(definedStructList, name);
        free(name);
        if (rt_type == NULL) {
            // error doesn't find struct type
        }
    }
    assert(rt_type != NULL);
    return rt_type;
}

/**
 * @return a copy of Type
*/
const Type* resolveSpecifier(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Specifier") == 0);
    const char* expressions[] = {
        "TYPE",
        "StructSpecifier"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    if (i == 0) {
        return createBasicTypeOfNode(getChildByName(node, "TYPE"));
    }
    // i == 1
    const EnvKind prev = currentEnv->kind;
    currentEnv->kind = STRUCTURE;
    const Type* type = resolveStructSpecifier(getChildByName(node, "StructSpecifier"));
    currentEnv->kind = prev;
    return type;
}

/**
 * @brief relay function between 'VarDec' and 'ExtDef'
*/
void resolveExtDecList(const ParseTNode* node, DecGather** gather) {
    assert(node != NULL);
    assert(strcmp(node->name,"ExtDecList") == 0);
    const char* expressions[] = {
        "VarDec",
        "VarDec COMMA ExtDecList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    resolveVarDec(getChildByName(node, "VarDec"), gather);
    assert(*gather != NULL);
    if (i == 1) {
        resolveExtDecList(getChildByName(node, "ExtDecList"), gather);
    }
}

void resolveExtDef(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"ExtDef") == 0);
    const char* expressions[] = {
        "Specifier SEMI",
        "Specifier ExtDecList SEMI",
        "Specifier FunDec CompSt"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    // todo check the type existence for structure
    const Type* type = resolveSpecifier(getChildByName(node, "Specifier"));
    if (i == 0) {
        freeType((Type*)type);
        return;
    }
    if (i == 1) {
        DecGather* gather = NULL;
        resolveExtDecList(getChildByName(node, "ExtDecList"), &gather);
        // loop through gather
        const DecGather* g = gather;
        assert(currentEnv != NULL && currentEnv->kind == GLOBAL); // in global environment
        while (g != NULL) {
            // check duplication in current(global) environment
            const Data* d = searchWithName(currentEnv->vMap, g->name);
            if (d != NULL) {
                assert(d->kind == VAR);
                char buffer[50];
                sprintf(buffer, "Redefined variable \"%s\".\n", g->name);
                error(3, g->lineNum, buffer);
                g = g->next;
                continue;
            }
            Data* data = malloc(sizeof(Data));
            data->name = my_strdup(g->name);
            data->kind = VAR;
            data->variable.type = (Type*)turnDecGather2Type(g, type);

            insert(currentEnv->vMap, data);
            g = g->next;
        }
        freeDecGather(gather);
        freeType((Type*)type);
        return;
    } // i == 2
    ParamGather* gather = NULL;
    char* name = NULL;
    const int argc = resolveFunDec(getChildByName(node, "FunDec"), &gather, &name);
    assert(name != NULL); // function name should be obtained

    assert(funcMap != NULL);
    const Data* d = searchWithName(funcMap, name);
    if (d != NULL) {
        assert(d->kind == FUNC);
        const int lineNum = getChildByName(node, "FunDec")->lineNum;
        char buffer[50];
        sprintf(buffer, "Redefined function \"%s\".\n", name);
        error(4, lineNum, buffer);
        free(name);
    } else {
        Data* func_data = malloc(sizeof(Data));
        func_data->kind = FUNC;
        func_data->name = name; // directly pointing to name, so no need to free
        func_data->function.argc = argc;
        func_data->function.returnType = (Type*)deepCopyType(type);
        func_data->function.argvTypes = malloc(sizeof(Type*) * argc);
        // loop through paramGather
        const ParamGather* g = gather;
        /* reversely copy the type
         * because param gather collects an element from its head */
        int j = argc - 1;
        while (g != NULL) {
            assert(g->data->kind == VAR); // all parameters collected should be variables
            func_data->function.argvTypes[j] = (Type*)deepCopyType(g->data->variable.type);
            j--;
            g = g->next;
        }
        assert(j == -1 && g == NULL);
        insert(funcMap, func_data);
        currentEnv = newEnvironment(currentEnv, FUNCTION);

        // register all parameters in new environment
        g = gather; // loop through paramGather again
        while (g != NULL) {
            insert(currentEnv->vMap, (Data*)deepCopyData(g->data)); // reminder: copy data
            g = g->next;
        }
        resolveCompSt(getChildByName(node, "CompSt"), type);
        revertEnvironment(&currentEnv);
    }
    freeParamGather(gather);
    freeType((Type*)type); // suppress warning
}

void resolveExtDefList(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"ExtDefList") == 0);
    const char* expressions[] = {
        "",
        "ExtDef ExtDefList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    if (i == 1) {
        resolveExtDef(getChildByName(node, "ExtDef"));
        resolveExtDefList(getChildByName(node, "ExtDefList"));
    }
}


void buildTable(const ParseTNode* root) {
    assert(root != NULL);
    assert(strcmp(root->name,"Program") == 0);
    char* expressions[] = {
        "ExtDefList"
    };
    globalEnv = currentEnv = newEnvironment(NULL, GLOBAL);
    funcMap = createRedBlackTree();
    assert(matchExprPattern(root, expressions, ARRAY_LEN(expressions)) == 0);
    resolveExtDefList(getChildByName(root, "ExtDefList"));
    freeEnvironment(globalEnv);
    freeRedBlackTree(funcMap);
    // todo remember to free funcmap and defined struct list;
}

#ifdef SYMBOL_test
int main() {
}
#endif
