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
// todo change this into an array and non-effective
#define EFFECTIVE_CODE 8, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV, C_RETURN, C_DEC, C_IFGOTO
#define EFFECTIVE_OP   4, O_VARIABLE, O_TEM_VAR, O_DEREF, O_REFER

typedef struct {
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
