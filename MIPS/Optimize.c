#ifdef LOCAL
#include <Optimize.h>
#else
#include "Optimize.h"
#endif

/**
 * IF a == b GOTO label1  |  IF a != b GOTO label2
 * GOTO label2            |  LABEL label1
 * LABEL label1           |
 */
static void flipCondition(Chunk *chunk) {
  Code *c = &chunk->code;
  if (c->kind != C_IFGOTO) return;
  if (chunk->next->code.kind != C_GOTO) return;
  if (chunk->next->next->code.kind != C_LABEL) return;
  const Code *c2 = &chunk->next->next->code;
  if (c->as.ternary.label.var_no != c2->as.unary.var_no) return;

  const int len = 6;
  char *flipList[len] = {"==", "<", ">", "<=", ">=", "!="};
  char **relation = &c->as.ternary.relation;
  // search in flipList
  for (int i = 0; i < len; ++i) {
    if (strcmp(*relation, flipList[i]) == 0) {
      free(*relation);
      // update relation
      *relation = my_strdup(flipList[len - 1 - i]);
      break;
    }
  }
  // update IFGOTO label
  c->as.ternary.label.var_no = chunk->next->code.as.unary.var_no;
  removeCode(chunk->next);
}

// change (op - #constant) into (op + #-constant)
static void organizeOpSequence(Code *c) {
  if (c->kind != C_SUB || c->as.binary.op2.kind != O_CONSTANT)
    return;
  c->kind = C_ADD;
  c->as.binary.op2.value = -c->as.binary.op2.value;
}

// merge (#1 + #2) into (#3)
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

// helper function for deleteLabel
static int cmp(const void *a, const void *b) {
  return *(int *) a - *(int *) b;
}

// remove redundant labels
static void deleteLabel(const Chunk *sentinel) {
  // collect all labels referred by IFGOTO and GOTO
  const Chunk *chunk = sentinel->next;
  int cnt = 0, capacity = 20;
  int *container = malloc(sizeof(int) * capacity);
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;

    if (cnt >= capacity) {
      capacity *= 2;
      container = realloc(container, sizeof(int) * capacity);
    }

    if (c->kind == C_IFGOTO) {
      container[cnt++] = c->as.ternary.label.var_no;
    } else if (c->kind == C_GOTO) {
      container[cnt++] = c->as.unary.var_no;
    }
  }
  qsort(container, cnt, sizeof(int), cmp);
  assert(chunk == sentinel);
  // loop through again to remove those redundant labels
  chunk = chunk->next;
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;
    if (c->kind != C_LABEL) continue;
    if (bsearch(&c->as.unary.var_no, container,
                cnt, sizeof(int), cmp) != NULL)
      continue;
    removeCode(chunk->prev);
  }
  free(container);
}

void optimize(const Chunk *sentinel) {
  Chunk *c = sentinel->next;
  while (c != sentinel) {
    // these optimizations don't add or remove code, only modify them
    fuseConstant(&c->code);
    organizeOpSequence(&c->code);
    flipCondition(c);
    c = c->next;
  }
  assert(c == sentinel);
  deleteLabel(c);
}
