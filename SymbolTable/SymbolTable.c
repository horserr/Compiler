#include "SymbolTable.h"
#include <stdio.h>


#ifdef SYMBOL_test
int main() {
    RedBlackTree* rbt = createRedBlackTree();
    printf("hello symbol test!");
    freeRedBlackTree(rbt);
}
#endif
