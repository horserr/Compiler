#include "Environment.h"
#include <assert.h>
#include <stdlib.h>
#include "../utils/utils.h"

Environment* newEnvironment(Environment* parent, const EnvKind kind) {
    Environment* env = malloc(sizeof(Environment));
    env->parent = parent;
    env->Map = createRedBlackTree();
    env->kind = kind;
    return env;
}

void freeEnvironment(Environment* env) {
    assert(env != NULL);
    freeRedBlackTree(env->Map);
    free(env);
}

/**
 * @brief change the current environment back to its parent environment
 * @note the pointer of current environment will be changed after calling this function.
*/
void revertEnvironment(Environment** current) {
    Environment* parent = (*current)->parent;
    if (parent == NULL) {
        perror("Can't revert global environment. {Environment.c revertEnvironment}.\n");
        exit(EXIT_FAILURE);
    }
    freeEnvironment(*current);
    *current = parent;
}

Type* createBasicTypeOfNode(const ParseTNode* basic_node) {
    assert(basic_node != NULL);
    const char* val = basic_node->value.str_value;
    Type* rt_type = NULL;
    if (strcmp(val, "int") == 0) {
        rt_type = malloc(sizeof(Type));
        rt_type->kind = INT;
    } else if (strcmp(val, "float") == 0) {
        rt_type = malloc(sizeof(Type));
        rt_type->kind = FLOAT;
    } else {
        fprintf(stderr, "Wrong type: %s. Should be 'int' or 'float'. "
                "Checking from {SymbolTable.c resolveSpecifier}.\n", val);
        exit(EXIT_FAILURE);
    }
    return rt_type;
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
 *  @note the pointing of head will be changed after calling this function
*/
void gatherDecInfo(DecGather** head, const char* name,
                   const int dimension, const int size_list[], int lineNum) {
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

void freeGather(DecGather* head) {
    while (head != NULL) {
        DecGather* tmp = head;
        head = head->next;
        free(tmp->name);
        free(tmp);
    }
}
