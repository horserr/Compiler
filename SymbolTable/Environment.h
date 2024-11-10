#ifndef ENVIRONMENT
#define ENVIRONMENT

#include "RBTree.h"
#include "SymbolTable_private.h"

typedef enum { GLOBAL, STRUCTURE, FUNCTION } EnvKind;

typedef struct Environment {
    EnvKind kind;
    struct Environment* parent;
    RedBlackTree* Map;
} Environment;

// collect struct type and check whether they are defined
typedef struct StructTypeWrapper {
    Type* structType;
    struct StructTypeWrapper* next;
} StructTypeWrapper;

// gather variable declared
#define MAX_DIMENSION 10

typedef struct DecGather {
    char* name;
    int lineNum;
    int dimension; // if dimension is zero, it is a variable; else an array
    int size_list[MAX_DIMENSION]; // only support max 10 dimension for array;
    struct DecGather* next;
} DecGather;

Environment* newEnvironment(Environment* parent, EnvKind kind);
void freeEnvironment(Environment* env);
void revertEnvironment(Environment** current);
void definedStructListAdd(StructTypeWrapper** head, const Type* type);
const Type* findDefinedStructType(const StructTypeWrapper* head, const char* name);

// utility functions
Type* createBasicTypeOfNode(const ParseTNode* basic_node);
const Type* findDefinedStructType(const StructTypeWrapper* head, const char* name);
void gatherDecInfo(DecGather** head, const char* name, int dimension, const int size_list[], int lineNum);
void freeGather(DecGather* head);

#endif
