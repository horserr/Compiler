#ifndef OPTIMIZE__H
#define OPTIMIZE__H
#ifdef LOCAL
#include <IR.h>
#include <utils.h>
#else
#include "IR.h"
#include "utils.h"
#endif

typedef struct use_info {
  const Operand *op;
  bool in_use;
} use_info;

typedef struct {
  const Chunk *currentLine;
  use_info use[3]; // no more than 3 operands in an expression
} info;

typedef struct {
  const Chunk *begin, *end;
  int len;    // the number of code inside begin and end
  info *info; // an array
} BasicBlock;

typedef struct {
  int cnt;
  BasicBlock *container;
} Block;

void optimize(const Chunk *sentinel);
void freeBlock(Block *block);

#endif
