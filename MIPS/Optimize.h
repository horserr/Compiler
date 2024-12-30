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
#define EFFECTIVE_CODE 10, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV, \
                           C_RETURN, C_DEC, C_IFGOTO, C_PARAM, C_ARG

#define EFFECTIVE_OP   4, O_VARIABLE, O_TEM_VAR, O_DEREF, O_REFER

typedef struct {
  Operand *op;
  bool in_use;
} use_info;

typedef struct {
  const Chunk *currentLine;
  use_info use[3]; // no more than 3 operands in an expression
} info;

typedef struct {
  const Chunk *begin, *end;
  int len;             // the amount of code
  info *info;          // an array
  int cnt;             // the number of variables
  Operand **variables; // a list of pointers
} BasicBlock;

typedef struct {
  int cnt;
  BasicBlock *container;
} Block;

Block* optimize(const Chunk *sentinel);
void freeBlock(Block *block);

#endif
