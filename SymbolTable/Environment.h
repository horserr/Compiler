#ifndef ENVIRONMENT
#define ENVIRONMENT

#ifdef LOCAL
// todo don't understand why
// #include <ParseTree.h>
#include "../ParseTree/ParseTree.h"
#include <RBTree.h>
#else
#include "ParseTree.h"
#include "RBTree.h"
#endif


typedef enum { GLOBAL, STRUCTURE, COMPOUND } EnvKind;

typedef struct Environment {
    EnvKind kind;
    struct Environment *parent;
    RedBlackTree *vMap; // stands for variable map
} Environment;

// collect struct type and check whether they are defined
typedef struct StructRegister {
    Type *structType;
    struct StructRegister *next;
} StructRegister;

#define MAX_DIMENSION 10

typedef struct DecGather {
    char *name;
    int lineNum;
    int dimension;                // if dimension is zero, it is a variable; else an array
    int size_list[MAX_DIMENSION]; // only support max 10 dimension for array;
    struct DecGather *next;
} DecGather;

typedef struct ParamGather {
    Data *data;
    int lineNum;
    struct ParamGather *next;
} ParamGather;

Environment* newEnvironment(Environment *parent, EnvKind kind);
const Data* searchEntireScopeWithName(const Environment *env, const char *name);
void freeEnvironment(Environment *env);
void revertEnvironment(Environment **current);

// utility functions
const Type* createBasicTypeOfNode(const ParseTNode *basic_node);
// struct type register
void addDefinedStruct(StructRegister **head, const Type *type);
const Type* findDefinedStruct(const StructRegister *head, const char *name);
void freeDefinedStructList(StructRegister *head);
// Dec gather
void gatherDecInfo(DecGather **head, const char *name, int dimension, const int size_list[], int lineNum);
const Type* turnDecGather2Type(const DecGather *gather, const Type *base_type);
void freeDecGather(DecGather *head);
// Param gather
void gatherParamInfo(ParamGather **head, const Data *data, int lineNum);
void reverseParamGather(ParamGather **head);
int checkFuncCallArgs(const Data *func_data, const ParamGather *gather);
void freeParamGather(ParamGather *head);

#endif
