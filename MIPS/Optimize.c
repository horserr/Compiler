#ifdef LOCAL
#include <Optimize.h>
#else
#include "Optimize.h"
#endif

// change op - #constant into op + #-constant
static void organizeOpSequence(Code *c) {
  if (c->kind != C_SUB || c->as.binary.op2.kind != O_CONSTANT)
    return;
  c->kind = C_ADD;
  c->as.binary.op2.value = -c->as.binary.op2.value;
}

// merge #1 + #2 into #3
static void fuseConstant(Code *c) {
  if (!in(c->kind, 4, C_ADD, C_SUB, C_MUL, C_DIV))
    return;

  const Operand *op1 = &c->as.binary.op1;
  const Operand *op2 = &c->as.binary.op2;
  if (op1->kind != O_CONSTANT || op2->kind != O_CONSTANT)
    return;

  int *right = &c->as.assign.right.value;
  if (c->kind == C_ADD)
    *right = op1->value + op2->value;
  else if (c->kind == C_SUB)
    *right = op1->value - op2->value;
  else if (c->kind == C_MUL)
    *right = op1->value * op2->value;
  else if (c->kind == C_DIV)
    *right = op1->value / op2->value;
  // no need to change assign.left because it aligns with binary.result
  c->kind = C_ASSIGN;
}


void optimize(const Chunk *sentinel) {
  Chunk *c = sentinel->next;
  while (c != sentinel) {
    // these optimizations don't add or remove code, only modify them
    fuseConstant(&c->code);
    organizeOpSequence(&c->code);
    c = c->next;
  }
}
