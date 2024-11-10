#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "Environment.h"
#include "../utils/utils.h"

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

Environment* currentEnv = NULL;
// a linked list registers those defined struct type
StructTypeWrapper* definedStructList = NULL;

void resolveArgs(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Args") == 0);
    const char* expressions[] = {
        "Exp COMMA Args",
        "Exp"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveExp(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Exp") == 0);
    const char* expressions[] = {
        "Exp ASSIGNOP Exp",
        "Exp AND Exp",
        "Exp OR Exp",
        "Exp RELOP Exp",
        "Exp PLUS Exp",
        "Exp MINUS Exp",
        "Exp STAR Exp",
        "Exp DIV Exp",
        "LP Exp RP",
        "MINUS Exp",
        "NOT Exp",
        "ID LP Args RP",
        "ID LP RP",
        "ID",
        "INT",
        "FLOAT"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

structFieldElement* resolveDec(const ParseTNode* node, Type* type) {
    assert(node != NULL);
    assert(strcmp(node->name,"Dec") == 0);
    const char* expressions[] = {
        "VarDec",
        "VarDec ASSIGNOP Exp"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    if (i == 1) {
        if (currentEnv->kind == STRUCTURE) {
            // todo error
        } else {
            resolveExp(getChildByName(node, "Exp"));
        }
    }
}

// todo change the return to a list of identities
structFieldElement* resolveDecList(const ParseTNode* node, Type* type) {
    assert(node != NULL);
    assert(strcmp(node->name,"DecList") == 0);
    const char* expressions[] = {
        "Dec",
        "Dec COMMA DecList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

structFieldElement* resolveDef(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Def") == 0);
    const char* expressions[] = {
        "Specifier DecList SEMI"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    assert(i == 0);
    Type* type = resolveSpecifier(getChildByName(node, "Specifier"));
    return resolveDecList(getChildByName(node, "DecList"), type);
}

structFieldElement* resolveDefList(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"DefList") == 0);
    const char* expressions[] = {
        "Def",
        "Def DefList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    // depend on environment kind(struct, function, global), it may have different effect
    if (currentEnv->kind == STRUCTURE) {
        // todo gather
        return NULL;
    }
    resolveDef(getChildByName(node, "Def"));
    if (i == 1) {
        resolveDefList(getChildByName(node, "DefList"));
    }
}

void resolveStmt(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Stmt") == 0);
    const char* expressions[] = {
        "Exp SEMI",
        "CompSt",
        "RETURN Exp SEMI",
        "IF LP Exp RP Stmt",
        "IF LP Exp RP Stmt ELSE Stmt",
        "WHILE LP Exp RP Stmt"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveStmtList(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"StmtList") == 0);
    const char* expressions[] = {
        "Stmt",
        "Stmt StmtList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveCompSt(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"CompSt") == 0);
    const char* expressions[] = {
        "LC DefList StmtList RC"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveParamDec(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"ParamDec") == 0);
    const char* expressions[] = {
        "Specifier VarDec"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveVarList(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"VarList") == 0);
    const char* expressions[] = {
        "ParamDec",
        "ParamDec COMMA VarList"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
}

void resolveFunDec(const ParseTNode* node, const Type* returnType) {
    assert(node != NULL);
    assert(strcmp(node->name,"FunDec") == 0);
    const char* expressions[] = {
        "ID LP VarList RP",
        "ID LP RP"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    //todo
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

char* resolveTag(const ParseTNode* node) {
    assert(node != NULL);
    assert(strcmp(node->name,"Tag") == 0);
    const char* expressions[] = {
        "ID"
    };
    assert(matchExprPattern(node, expressions, ARRAY_LEN(expressions)) == 0);
    return my_strdup(getChildByName(node, "ID")->value.str_value);
}

char* resolveOptTag(const ParseTNode* node) {
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

char* randomString(const int len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetSize = sizeof(charset) - 1;
    srand(time(0));
    // len + 7('-struct') + 1(\0)
    char* str = malloc(sizeof(char) * (len + 7 + 1));
    for (int i = 0; i < len; ++i) {
        const int key = rand() % charsetSize;
        str[i] = charset[key];
    }
    str[len] = '\0';
    strcat(str, "-struct");
    return str;
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
            name = randomString(5);
        }
        rt_type = malloc(sizeof(Type));
        rt_type->kind = STRUCT;
        rt_type->structure.struct_name = name;
        rt_type->structure.fields = resolveDefList(getChildByName(node, "DefList"));
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
 * @note return a copy of Type
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
 * @note relay function between 'VarDec' and 'ExtDef'
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
        "Specifier ExtDecList SEMI",
        "Specifier SEMI",
        "Specifier FunDec CompSt"
    };
    const int i = matchExprPattern(node, expressions, ARRAY_LEN(expressions));
    // todo extract the type and check their existence
    if (i == 0) {
        const Type* type = resolveSpecifier(getChildByName(node, "Specifier"));
        DecGather* gather = NULL;
        resolveExtDecList(getChildByName(node, "ExtDecList"), &gather);
        // loop through gather
        const DecGather* g = gather;
        while (g != NULL) {
            // check duplication in current environment
            if (searchWithName(currentEnv->Map, g->name) != NULL) {
                char buffer[50];
                sprintf(buffer, "Redefined variable \"%s\".\n", g->name);
                error(3, g->lineNum, buffer);
                g = g->next;
                continue;
            }

            Data* data = malloc(sizeof(Data));
            data->name = my_strdup(g->name);
            data->kind = VAR;
            // use size_list to create array type
            // get the address of variable type to allocate memory in place
            Type** t = &data->variable.type;
            for (int j = 0; j < g->dimension; ++j) {
                *t = malloc(sizeof(Type));
                (*t)->kind = ARRAY;
                (*t)->array.size = g->size_list[j];
                *t = (*t)->array.elemType;
            }
            *t = deepCopyType(type);

            insert(currentEnv->Map, data);
            g = g->next;
        }
        freeGather(gather);
        freeType(type);
    } else if (i == 1) {
        /* if the specifier is a struct, whether it has name or not,
         * then it will be registered in `resolveSpecifier`;
         * else if the specifier is a type, then we can ignore it directly.
        */
        Type* type = resolveSpecifier(getChildByName(node, "Specifier"));
        freeType(type);
    } else if (i == 2) {
        Type* returnType = resolveSpecifier(getChildByName(node, "Specifier"));
        resolveFunDec(getChildByName(node, "FunDec"), returnType);
        // todo function component and create environment
        //  and check return type
        resolveCompSt(getChildByName(node, "CompSt"));
        // todo remember to free type and change back env
        freeType(returnType);
    }
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
    currentEnv = newEnvironment(NULL, GLOBAL);
    Environment* globalEnv = currentEnv;
    assert(matchExprPattern(root, expressions, ARRAY_LEN(expressions)) == 0);
    resolveExtDefList(getChildByName(root, "ExtDefList"));
    freeEnvironment(globalEnv);
}

#ifdef SYMBOL_test
int main() {
}
#endif
