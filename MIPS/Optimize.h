#ifndef OPTIMIZE__H
#define OPTIMIZE__H
#ifdef LOCAL
#include <IR.h>
#include <utils.h>
#else
#include "IR.h"
#include "utils.h"
#endif

typedef struct {
  const Chunk *begin, *end;
} BasicBlock;

typedef struct {
  int cnt, capacity;
  BasicBlock *container;
} Block;

void optimize(const Chunk *sentinel);
void free_block(Block *block);

#endif
