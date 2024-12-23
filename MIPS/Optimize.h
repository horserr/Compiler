#ifndef OPTIMIZE__H
#define OPTIMIZE__H
#ifdef LOCAL
#include <IR.h>
#include <utils.h>
#else
#include "IR.h"
#include "utils.h"
#endif

// remainder: only use these macros inside 'in' function
#define EFFECTIVE_CODE 5, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV
#define EFFECTIVE_OP   2, O_VARIABLE, O_TEM_VAR

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

Block* optimize(const Chunk *sentinel);
void freeBlock(Block *block);

#endif
