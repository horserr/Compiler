#ifndef environment
#define environment
#include "RBTree.h"

typedef struct Environment Environment;

struct Environment {
    Environment* parent;
    RedBlackTree* Map;
};

Environment* newEnvironment(Environment* parent);
void freeEnvironment(Environment* env);

#endif
