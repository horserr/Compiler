#ifndef ENVIRONMENT
#define ENVIRONMENT

#include "RBTree.h"
#include "../ParseTree/ParseTree.h"
#include "../utils/utils.h"


typedef enum { GLOBAL, STRUCTURE, FUNCTION } EnvKind;

typedef struct Environment {
    EnvKind kind;
    struct Environment* parent;
    RedBlackTree* vMap; // stands for variable map
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

typedef struct ParamGather {
    Data* data;
    int lineNum;
    struct ParamGather* next;
} ParamGather;

Environment* newEnvironment(Environment* parent, EnvKind kind);
const Data* searchEntireScopeWithName(const Environment* env, const char* name);
void freeEnvironment(Environment* env);
void revertEnvironment(Environment** current);
void definedStructListAdd(StructTypeWrapper** head, const Type* type);
const Type* findDefinedStructType(const StructTypeWrapper* head, const char* name);

// utility functions
const Type* createBasicTypeOfNode(const ParseTNode* basic_node);
const Type* findDefinedStructType(const StructTypeWrapper* head, const char* name);
// Dec gather
void gatherDecInfo(DecGather** head, const char* name, int dimension, const int size_list[], int lineNum);
const Type* turnDecGather2Type(const DecGather* gather, const Type* base_type);
void freeDecGather(DecGather* head);

// Param gather
void gatherParamInfo(ParamGather** head, const Data* data,  int lineNum);
int checkFuncCallArgs(const Data* func_data, const ParamGather* gather);
void freeParamGather(ParamGather* head);

#endif
