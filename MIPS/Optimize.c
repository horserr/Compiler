#ifdef LOCAL
#include <Optimize.h>
#else
#include "Optimize.h"
#endif

typedef struct {
  int cnt;
  int capacity;
  int *container;
} ArrayList;

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
  const char *flipList[len] = {"==", "<", ">", "<=", ">=", "!="};
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
static void foldConstant(Code *c) {
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

// remove redudent labels
static void deleteLabel(const Chunk *sentinel) {
  // collect all labels referred by IFGOTO and GOTO
  const Chunk *chunk = sentinel->next;
  int cnt = 0, capacity = 20;
  int *container = malloc(sizeof(int) * capacity);
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;
    if (cnt >= capacity) { // resize
      capacity *= 2;
      container = realloc(container, sizeof(int) * capacity);
    }
    if (c->kind == C_IFGOTO) {
      container[cnt++] = c->as.ternary.label.var_no;
    } else if (c->kind == C_GOTO) {
      container[cnt++] = c->as.unary.var_no;
    }
  }
  qsort(container, cnt, sizeof(int), cmp_int);
  assert(chunk == sentinel);
  // loop through again to remove those redundant labels
  chunk = chunk->next;
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;
    if (c->kind != C_LABEL) continue;
    if (bsearch(&c->as.unary.var_no, container,
                cnt, sizeof(int), cmp_int) != NULL)
      continue;
    removeCode(chunk->prev);
  }
  free(container);
}

// note: after delete redundant labels, all labels are necessary.
static Block* createBlocks(const Chunk *sentinel) {
  Block *block = malloc(sizeof(Block));
  int *cnt = &block->cnt, *capacity = &block->capacity;
  BasicBlock **container = &block->container;
  *cnt = 0;
  *capacity = 5;
  *container = malloc(sizeof(BasicBlock) * *capacity);

  const Chunk *chunk = sentinel->next;
  (*container)[(*cnt)++].begin = chunk;

  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;
    // todo maybe include function call as well
    if (!in(c->kind, 3, C_LABEL, C_IFGOTO, C_GOTO))
      continue;

    if (*cnt >= *capacity) { // resize
      *capacity *= 2;
      *container = realloc(*container, sizeof(BasicBlock) * *capacity);
    }

    const Chunk *target = c->kind == C_LABEL ? chunk->prev : chunk;
    // check for duplication before add
    if ((*container)[*cnt - 1].begin != target)
      (*container)[(*cnt)++].begin = target;
  }

  for (int i = 0; i < *cnt - 1; ++i)
    (*container)[i].end = (*container)[i + 1].begin->prev;
  (*container)[*cnt - 1].end = sentinel->prev;

  return block;
}

/** cascade (adjacent) arithmetic operations that only include "temporary" constants
 * @return true if there are some constants that can be folded
 *
 * <p>
 *  t0 := #2      |  t1 := 6
 *  t1 := t0 * 3  |
 */
static bool forwardConstant(const Chunk *sentinel) {
  bool flag = false;
  Chunk *chunk = sentinel->next;
  while (chunk != sentinel) {
    Code *c = &chunk->code;
    chunk = chunk->next; // chunk is now pointing to next code
    foldConstant(c);

    if (c->kind != C_ASSIGN) continue;
    const Operand *prev_right = &c->as.assign.right;
    if (prev_right->kind != O_CONSTANT) continue;
    const Operand *prev_left = &c->as.assign.left;

    Operand *target = NULL;
    // check assignment
    if (chunk->code.kind == C_ASSIGN) {
      Operand *right = &chunk->code.as.assign.right;
      if (right->kind == O_TEM_VAR && right->var_no == prev_left->var_no) {
        target = right;
        goto FOUND;
      }
    }
    // check binary operation
    if (!in(chunk->code.kind, 4, C_ADD, C_SUB, C_MUL, C_DIV)) continue;
    Operand *op1 = &chunk->code.as.binary.op1;
    Operand *op2 = &chunk->code.as.binary.op2;

    if (!in(O_TEM_VAR, 2, op1->kind, op2->kind)) continue;
    if (!in(prev_left->var_no, 2, op1->var_no, op2->var_no)) continue;

    target = op1->var_no == prev_left->var_no ? op1 : op2;
  FOUND: //label
    target->value = prev_right->value;
    target->kind = O_CONSTANT;
    removeCode(chunk->prev);
    flag = true;
  }
  return flag;
}

// todo remove this!!
// debug utility
static void printBlock(const Block *block) {
  assert(block != NULL);
  for (int i = 0; i < block->cnt; ++i) {
    printf("\n+++Block %d:\n", i);
    printCode(stdout, &block->container[i].begin->code);
    printCode(stdout, &block->container[i].end->code);
  }
}

void optimize(const Chunk *sentinel) {
  while (forwardConstant(sentinel)) { /* do nothing */ }
  // todo extract this
  Chunk *c = sentinel->next;
  while (c != sentinel) {
    organizeOpSequence(&c->code);
    flipCondition(c);
    c = c->next;
  }
  assert(c == sentinel);
  deleteLabel(c);
  Block *block = createBlocks(c);

  printBlock(block);
  // todo remove
  free_block(block);
}

void free_block(Block *block) {
  assert(block != NULL);
  free(block->container);
  free(block);
}
