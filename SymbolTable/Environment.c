#include "Environment.h"
#include <stdlib.h>

Environment* newEnvironment(Environment* parent) {
    Environment* env = malloc(sizeof(Environment));
    env->parent = parent;
    env->Map = createRedBlackTree();
    return env;
}

void freeEnvironment(Environment* env) {
    freeRedBlackTree(env->Map);
    free(env);
}
