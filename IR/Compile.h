#ifndef COMPILE__H
#define COMPILE__H

#include "IR.h"
#ifdef LOCAL
#include <SymbolTable.h>
#else
#include "SymbolTable.h"
#endif

const Chunk* compile(const ParseTNode *root, const SymbolTable *table);

#endif
