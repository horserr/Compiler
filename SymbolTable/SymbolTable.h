#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#ifdef LOCAL
#include <Environment.h>
#else
#include "Environment.h"
#endif

// proj 3 add this! guarantee that no duplication in name.
typedef struct SymbolTable {
  RedBlackTree *vars;
  RedBlackTree *funcs;
} SymbolTable;

const SymbolTable* buildTable(const ParseTNode *root);
void freeTable(SymbolTable *table);

#endif
