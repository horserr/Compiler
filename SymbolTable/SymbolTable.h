#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include "Environment.h"

void buildTable(const ParseTNode* root);
const Type* resolveExp(const ParseTNode* node);
void resolveArgs(const ParseTNode* node, ParamGather** gather);
void resolveVarDec(const ParseTNode* node, DecGather** gather);
const Type* resolveSpecifier(const ParseTNode* node);
void resolveCompSt(const ParseTNode* node, const Type* returnType);

#endif
