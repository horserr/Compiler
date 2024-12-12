#ifndef COMPILE__H
#define COMPILE__H

#ifdef LOCAL
#include <IR.h>
#include <SymbolTable.h>
#else
#include "SymbolTable.h"
#include "IR.h"
#endif

const Chunk* compile(const ParseTNode *root, const SymbolTable *table);

#endif
