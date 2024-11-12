#include "Environment.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Environment* newEnvironment(Environment* parent, const EnvKind kind) {
    Environment* env = malloc(sizeof(Environment));
    env->parent = parent;
    env->vMap = createRedBlackTree();
    env->kind = kind;
    return env;
}

/**
 * @return a pointer directly pointing to the data in map
*/
const Data* searchEntireScopeWithName(const Environment* currentEnv, const char* name) {
    const Data* data = NULL;
    while (currentEnv != NULL) {
        data = searchWithName(currentEnv->vMap, name);
        if (data != NULL) break;
        currentEnv = currentEnv->parent;
    }
    return data;
}

void freeEnvironment(Environment* env) {
    assert(env != NULL);
    freeRedBlackTree(env->vMap);
    free(env);
}

/**
 * @brief change the current environment back to its parent environment
 * @note the pointer of current environment will be changed after calling this function.
*/
void revertEnvironment(Environment** current) {
    assert(current != NULL);
    Environment* parent = (*current)->parent;
    if (parent == NULL) {
        perror("Can't revert global environment. {Environment.c revertEnvironment}.\n");
        exit(EXIT_FAILURE);
    }
    freeEnvironment(*current);
    *current = parent;
}

/**
 * @brief versatile function: get the basic type of current node
 * @param basic_node must contain basic token(int or float).
 * @return copy of type
*/
const Type* createBasicTypeOfNode(const ParseTNode* basic_node) {
    assert(basic_node != NULL);
    if (strcmp(basic_node->name, "TYPE") == 0) { // specifier
        const char* val = basic_node->value.str_value;
        if (strcmp(val, "int") == 0) {
            Type* rt_type = malloc(sizeof(Type));
            rt_type->kind = INT;
            return rt_type;
        }
        if (strcmp(val, "float") == 0) {
            Type* rt_type = malloc(sizeof(Type));
            rt_type->kind = FLOAT;
            return rt_type;
        }
    } else if (strcmp(basic_node->name, "INT") == 0) { // expression
        Type* t = malloc(sizeof(Type));
        t->kind = INT;
        return t;
    } else if (strcmp(basic_node->name, "FLOAT") == 0) { // expression
        Type* t = malloc(sizeof(Type));
        t->kind = FLOAT;
        return t;
    }
    fprintf(stderr, "basic_node:%s.\n Wrong type! Should be 'int' or 'float'. "
            "Checking from {Environment createBasicTypeOfNode}.\n", basic_node->name);
    exit(EXIT_FAILURE);
}

/**
 *  @brief copy and add struct type to the head of the linked list
 *  @note head will be changed after calling this function
 *  @warning remember to free type passed in after use
*/
void definedStructListAdd(StructTypeWrapper** head, const Type* type) {
    assert(type->kind == STRUCT);
    StructTypeWrapper* wrapper = malloc(sizeof(StructTypeWrapper));
    wrapper->structType = deepCopyType(type);
    wrapper->next = *(head);
    *(head) = wrapper;
}

/**
 *  @brief find struct type using name
 *  @return if found, return type; else NULL.
 */
const Type* findDefinedStructType(const StructTypeWrapper* head, const char* name) {
    while (head != NULL) {
        const Type* type = head->structType;
        assert(type->kind == STRUCT);
        if (strcmp(type->structure.struct_name, name) == 0) {
            return type;
        }
        head = head->next;
    }
    return NULL;
}

/**
 *  @brief add variable info to the head of gather
 *  all information will be copied
 *  @note the pointing of head will be changed after calling this function
*/
void gatherDecInfo(DecGather** head, const char* name,
                   const int dimension, const int size_list[], const int lineNum) {
    assert(name != NULL);
    DecGather* gather = malloc(sizeof(DecGather));
    gather->name = my_strdup(name);
    gather->dimension = dimension;
    for (int i = 0; i < dimension; ++i) {
        gather->size_list[i] = size_list[i];
    }
    gather->lineNum = lineNum;
    gather->next = *head;
    *head = gather;
}

/**
 * @brief turn a single variable which is in the form of dec gather to type
 * @return a copy of type
*/
const Type* turnDecGather2Type(const DecGather* gather, const Type* base_type) {
    assert(gather != NULL);
    Type* type = NULL;
    Type** t = &type;
    for (int i = 0; i < gather->dimension; ++i) {
        *t = malloc(sizeof(Type));
        (*t)->kind = ARRAY;
        (*t)->array.size = gather->size_list[i];
        *t = (*t)->array.elemType;
    }
    *t = deepCopyType(base_type);
    return type;
}

void freeDecGather(DecGather* head) {
    while (head != NULL) {
        DecGather* tmp = head;
        head = head->next;
        free(tmp->name);
        free(tmp);
    }
}

void gatherParamInfo(ParamGather** head, const Data* data, const int lineNum) {
    assert(data->kind == VAR); // param info data should only be variable, not function
    ParamGather* gather = malloc(sizeof(ParamGather));
    gather->lineNum = lineNum;
    gather->data = deepCopyData(data);
    gather->next = *head;
    *head = gather;
}

/**
 * @brief check whether function invocation arguments correlate with definitions
 * @param func_data the data of the definition for function
 * @param gather contains function arguments
 * @return 1 if matches; else 0.
*/
int checkFuncCallArgs(const Data* func_data, const ParamGather* gather) {
    assert(func_data != NULL && func_data->kind == FUNC);
    int i = 0; // index for argvTypes
    while (gather != NULL) {
        if (i >= func_data->function.argc) {
            return 0;
        }
        assert(gather->data->kind == VAR); // param gather only collects variable
        const Type* t = gather->data->variable.type;
        if (t->kind == ERROR || !typeEqual(t, func_data->function.argvTypes[i])) {
            return 0;
        }
        i++;
        gather = gather->next;
    }
    return i == func_data->function.argc;
}

void freeParamGather(ParamGather* head) {
    while (head != NULL) {
        ParamGather* tmp = head;
        head = head->next;
        freeData(tmp->data);
        free(tmp);
    }
}
