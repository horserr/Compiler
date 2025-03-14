#ifdef LOCAL
#include <Optimize.h>
#else
#include "Optimize.h"
#endif

typedef struct {
  size_t len;
  use_info *use;
} UseInfoTable;

extern const uint8_t operand_count_per_code[];
int cmp_use_info(const void *u1, const void *u2);
/* print code is only for debug purpose, so declare it as
 `extern` rather than add it in the header file. */

/**
 * <pre>
 * IF a == b GOTO label1  |  IF a != b GOTO label2
 * GOTO label2            |  LABEL label1
 * LABEL label1           |
 * </pre>
 */
static void flipCondition(const Chunk *sentinel) {
  Chunk *chunk = (Chunk *) sentinel;
  while (chunk->next != sentinel) {
    chunk = chunk->next;

    Code *c = &chunk->code;
    if (c->kind != C_IFGOTO) continue;
    if (chunk->next->code.kind != C_GOTO) continue;
    if (chunk->next->next->code.kind != C_LABEL) continue;
    const Code *c2 = &chunk->next->next->code;
    if (c->as.ternary.label.var_no != c2->as.unary.var_no) continue;

    const int len = 6;
    const char *flipList[] = {"==", "<", ">", "<=", ">=", "!="};
    char **relation = &c->as.ternary.relation;
    // search in flipList
    const int index = findInArray(relation, false, flipList,
                                  len, sizeof(char *), cmp_str);
    free(*relation); // maybe cause free after use warning.
    *relation = my_strdup(flipList[len - 1 - index]);

    // update IFGOTO label NO.
    c->as.ternary.label.var_no = chunk->next->code.as.unary.var_no;
    // remove the redundant 'GOTO' code
    removeCode(chunk->next);
  }
}

// remove redundant labels
static void deleteLabels(const Chunk *sentinel) {
  // collect all labels referred by IFGOTO and GOTO
  const Chunk *chunk = sentinel->next;
  int cnt = 0, capacity = 20;
  int *container = malloc(sizeof(int) * capacity);
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    if (cnt >= capacity) goto RESIZE;
    if (c->kind == C_IFGOTO) {
      container[cnt++] = c->as.ternary.label.var_no;
    } else if (c->kind == C_GOTO) {
      container[cnt++] = c->as.unary.var_no;
    }
    chunk = chunk->next;
    continue;
  RESIZE:
    RESIZE(capacity);
    container = realloc(container, sizeof(int) * capacity);
  }
  qsort(container, cnt, sizeof(int), cmp_int);
  assert(chunk == sentinel);
  // loop through again to remove those redundant labels
  chunk = chunk->next;
  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    chunk = chunk->next;
    if (c->kind != C_LABEL) continue;
    if (findInArray(&c->as.unary.var_no, true, container,
                    cnt, sizeof(int), cmp_int) != -1)
      continue;
    removeCode(chunk->prev);
  }
  free(container);
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

///// Blocks ///////////////////////////////////////////////

/**
 * @brief create a use-info table which contains the use_info of variables <b>"in order"</b>
 * for each basic block
 * @return a use-info table.
 * <p>
 * helper function of `setBasicBlock`
 */
static UseInfoTable* createUseInfoTable(const BasicBlock *basic) {
  UseInfoTable *table = malloc(sizeof(UseInfoTable));
  size_t capacity = 5, *len = &table->len;
  use_info **use = &table->use;
  *len = 0;
  *use = malloc(sizeof(use_info) * capacity);

  // loop from begging to end
  const Chunk *chunk = basic->begin;
  while (chunk != basic->end->next) {
    const Code *code = &chunk->code;
    chunk = chunk->next;
    const int kind = code->kind;
    if (!in(kind, EFFECTIVE_CODE)) continue;

    uint8_t op_cnt = operand_count_per_code[kind];
    // the third operand for `C_IFGOTO` is label
    if (kind == C_IFGOTO) op_cnt--;

    // iterate through all operands per code
    for (int i = 0; i < op_cnt; ++i) {
      const Operand *op = (Operand *) &code->as + i;
      // ignore constant
      if (!in(op->kind, EFFECTIVE_OP)) continue;

      if (*len >= capacity) { //resize
        RESIZE(capacity);
        *use = realloc(*use, sizeof(use_info) * capacity);
      }
      (*use)[*len].in_use = false;
      (*use)[(*len)++].op = op;
      if (either(op->kind, O_REFER, O_DEREF))
        distill_op((*use)[*len - 1].op);
    }
  }
  // sort and remove duplicates
  qsort(*use, *len, sizeof(use_info), cmp_use_info);
  removeDuplicates(*use, len, sizeof(use_info), cmp_use_info);
  *use = realloc(*use, *len * sizeof(use_info));
  return table;
}

static void freeUseInfoTable(UseInfoTable *table) {
  assert(table != NULL);
  free(table->use);
  free(table);
}

#define search(key, table, cmp_fn) \
    bsearch(&(key), (table)->use, (table)->len, sizeof(use_info), (cmp_fn))

#define check_distill_op(op) \
  either((op)->kind, O_REFER, O_DEREF) ? (op)->address : (op)

/**
 * @brief update use-info table with <b>'effective'</b> variables of the given current code
 * <p>
 * * helper function of `setBasicBlock`
 */
static void updateUseInfoTable(const Code *code, const UseInfoTable *table) {
#define update(target_op, is_alive, table, cmp_fn) do {\
    use_info *find = search((use_info){.op = (target_op)}, (table), (cmp_fn));\
    assert(find != NULL);\
    find->in_use = (is_alive);\
  } while (false)

  assert(table != NULL);
  const int kind = code->kind;
  assert(in(kind, EFFECTIVE_CODE));

  // result operand is always the first operand lower in memory
  const Operand *result = (Operand *) &code->as;
  if (in(kind, 3, C_RETURN, C_WRITE, C_ARG) && result->kind == O_CONSTANT) return;

  int start_index = 0;
  uint8_t op_cnt = operand_count_per_code[kind];
  // Note: if kind is C_RETURN update it to be true
  if (kind == C_IFGOTO) {
    op_cnt--;
  } else if (!(kind == C_RETURN || either(result->kind, O_REFER, O_DEREF))) {
    // if kind includes `C_DEC`, then use info set to false
    update(result, false, table, cmp_use_info);
    start_index = 1;
  }

  for (int i = start_index; i < op_cnt; ++i) {
    const Operand *op = result + i;
    // do not update variables whose kind isn't within effective operands
    if (!in(op->kind, EFFECTIVE_OP)) continue;
    update(check_distill_op(op), true, table, cmp_use_info);
  }
#undef update
}

/**
 * @brief copy the current state of use-info table into the info list.
 * for each code, only copy those effective variables that appear in it.
 */
static void copyUseInfo(info *info, const Chunk *chunk, const UseInfoTable *table) {
  info->currentLine = chunk;
  const int kind = chunk->code.kind;
  assert(in(kind, EFFECTIVE_CODE));

  uint8_t op_cnt = operand_count_per_code[kind];
  // the third operand for `C_IFGOTO` is label
  if (kind == C_IFGOTO) op_cnt--;

  for (int i = 0; i < op_cnt; ++i) {
    const Operand *op = (Operand *) &chunk->code.as + i;
    // only search effective operands
    if (!in(op->kind, EFFECTIVE_OP)) continue;

    const use_info *find = search((use_info){.op = check_distill_op(op)},
                                  table, cmp_use_info);
    assert(find != NULL);
    memcpy(info->use + i, find, sizeof(use_info));
  }
}

#undef check_distill_op
#undef search

// set info for each line of code inside the basic block
static void setInfo(BasicBlock *basic) {
  int capacity = 5, *len = &basic->len;
  info **info_ = &basic->info;
  *len = 0;
  *info_ = malloc(sizeof(info) * capacity);
  // initialize use-info table
  UseInfoTable *table = createUseInfoTable(basic);

  // loop from end to begin
  const Chunk *chunk = basic->end;
  while (chunk != basic->begin->prev) {
    if (!in(chunk->code.kind, EFFECTIVE_CODE)) goto CONTINUE;
    if (*len >= capacity) goto RESIZE;
    // get a snapshot of the current state of use-info table
    copyUseInfo(&(*info_)[(*len)++], chunk, table);
    // update the use-info table with the current code
    updateUseInfoTable(&chunk->code, table);
  CONTINUE:
    chunk = chunk->prev;
    continue;
  RESIZE:
    RESIZE(capacity);
    *info_ = realloc(*info_, sizeof(info) * capacity);
  }
  reverseArray(*info_, *len, sizeof(info));

  // copy variables from table to basic block
  basic->cnt = (int) table->len;
  basic->variables = malloc(sizeof(Operand *) * table->len);
  for (int i = 0; i < table->len; ++i) {
    basic->variables[i] = table->use[i].op;
  }
  freeUseInfoTable(table);
}

// for each basic block, set their info array
static void setBlocksInfo(const Block *block) {
  assert(block != NULL);
  // loop through all basic blocks
  for (int i = 0; i < block->cnt; ++i) {
    BasicBlock *basic = &block->container[i];
    setInfo(basic);
  }
}

/**
 * @brief partition chunk into basic blocks
 * @note: after delete redundant labels, all labels are necessary.
 * @return a list of basic blocks
 */
static Block* partitionChunk(const Chunk *sentinel) {
  Block *block = malloc(sizeof(Block));
  int *cnt = &block->cnt, capacity = 5;
  BasicBlock **container = &block->container;
  *cnt = 0;
  *container = malloc(sizeof(BasicBlock) * capacity);

  const Chunk *chunk = sentinel->next;
  (*container)[(*cnt)++].begin = chunk;

  while (chunk != sentinel) {
    const Code *c = &chunk->code;
    if (!in(c->kind, 4, C_LABEL, C_IFGOTO, C_GOTO, C_FUNCTION))
      goto CONTINUE;
    if (*cnt >= capacity) goto RESIZE;

    const Chunk *target = either(c->kind, C_FUNCTION, C_LABEL)
                            ? chunk
                            : chunk->next;
    // check for duplication before adding
    if ((*container)[*cnt - 1].begin != target)
      (*container)[(*cnt)++].begin = target;
  CONTINUE:
    chunk = chunk->next;
    continue;
  RESIZE:
    RESIZE(capacity);
    *container = realloc(*container, sizeof(BasicBlock) * capacity);
  }

  for (int i = 0; i < *cnt - 1; ++i)
    (*container)[i].end = (*container)[i + 1].begin->prev;
  (*container)[*cnt - 1].end = sentinel->prev;

  setBlocksInfo(block);
  return block;
}

/** cascade (adjacent) arithmetic operations that only include
 * <b>"temporary"</b> constants
 * @return true if there are some constants that can be folded
 *  @example:
 *  <pre>
 *  t0 := #2      |  t1 := #6
 *  t1 := t0 * 3  |
 *  </pre>
 *  @example:
 *  <pre>
 *  a := 0        |  a := #0
 *  t0 := a * b   |  t0 := #0
 *  </pre>
 *  @example:
 *  <pre>
 *  a := 1        |  a := #0
 *  t0 := a * b   |  t0 := b
 *  </pre>
 */
static bool optimizeArithmatic(const Chunk *sentinel) {
  bool flag = false;
  Chunk *chunk = sentinel->next;
  while (chunk != sentinel) {
    bool need_remove = true;
    Code *c = &chunk->code;
    chunk = chunk->next; // chunk is now pointing to the next code
    foldConstant(c);
    organizeOpSequence(c);

    if (c->kind != C_ASSIGN) continue;
    const Operand *prev_right = &c->as.assign.right;
    if (prev_right->kind != O_CONSTANT) continue;
    const Operand *prev_left = &c->as.assign.left;
    // the previous line of code is assignment of constant

    Operand *target = NULL;
    // check assignment
    if (chunk->code.kind == C_ASSIGN) {
      Operand *right = &chunk->code.as.assign.right;
      if (right->kind == O_TEM_VAR && prev_left->kind == O_TEM_VAR
          && right->var_no == prev_left->var_no) {
        target = right;
        goto FOUND;
      }
      if (right->kind == O_VARIABLE && prev_left->kind == O_VARIABLE
          && strcmp(right->value_s, prev_left->value_s) == 0) {
        need_remove = false;
        target = right;
        goto FOUND;
      }
    }
    // check binary operation
    if (!in(chunk->code.kind, 4, C_ADD, C_SUB, C_MUL, C_DIV)) continue;
    Operand *op1 = &chunk->code.as.binary.op1;
    Operand *op2 = &chunk->code.as.binary.op2;


    if (prev_left->kind == O_TEM_VAR && either(O_TEM_VAR, op1->kind, op2->kind)) {
      if (either(prev_left->var_no, op1->var_no, op2->var_no)) {
        target = op1->var_no == prev_left->var_no ? op1 : op2;
        goto FOUND;
      }
    } else if (prev_left->kind == O_VARIABLE && either(O_VARIABLE, op1->kind, op2->kind)) {
      need_remove = false;
      if (op1->kind == O_VARIABLE &&
          strcmp(op1->value_s, prev_left->value_s) == 0) {
        target = op1;
        goto FOUND;
      }
      if (op2->kind == O_VARIABLE &&
          strcmp(op2->value_s, prev_left->value_s) == 0) {
        target = op2;
        goto FOUND;
      }
    }
    continue;

  FOUND:
    if (target->kind == O_VARIABLE)
      free((void *) target->value_s);
    target->value = prev_right->value;
    target->kind = O_CONSTANT;
    flag = true;
    if (need_remove)
      removeCode(chunk->prev);
  }
  return flag;
}

static void printBasicBlock(const BasicBlock *basic) {
  printf("all variables inside block.\n");
  for (int i = 0; i < basic->cnt; ++i) {
    printOp(stdout, basic->variables[i]);
  }
  printf("\n");

  for (int i = 0; i < basic->len; ++i) {
    const info *info_ = basic->info + i;
    const Chunk *chunk = info_->currentLine;
    // all code is effective if they are inside 'info' array
    assert(in(chunk->code.kind, EFFECTIVE_CODE));
    printf("As for line %d: \t", i);
    printCode(stdout, &chunk->code);
    printf("use info is: \t");
    const Code *code = &chunk->code;
    const int kind = code->kind;

    for (int e = 0; e < operand_count_per_code[kind]; ++e) {
      const Operand *op = (Operand *) &code->as + e;
      // jump over non-effective operand, but always increment index
      if (!in(op->kind, EFFECTIVE_OP)) {
        printf("i:[%d] Oops\t", e);
        continue;
      }
      printOp(stdout, info_->use[e].op);
      printf("\b[%d]--> in use or not: (%d)  ", e, info_->use[e].in_use);
    }
    printf("\n");
  }
}

// debug utility
__attribute__((unused))
static void printBlock(const Block *block) {
  assert(block != NULL);
  for (int i = 0; i < block->cnt; ++i) {
    printf("\n+++Block %d:\n", i);
    printCode(stdout, &block->container[i].begin->code);
    printCode(stdout, &block->container[i].end->code);
  }
  // print the first block info list
  printf("\n+++ FIRST basic block:\n");
  printBasicBlock(block->container);
}

Block* optimize(const Chunk *sentinel) {
  while (optimizeArithmatic(sentinel)) { /* do nothing */ }
  flipCondition(sentinel);
  deleteLabels(sentinel);
  Block *block = partitionChunk(sentinel);

#ifdef LOCAL
  printBlock(block);
#endif
  return block;
}

void freeBlock(Block *block) {
  assert(block != NULL);
  for (int i = 0; i < block->cnt; ++i) {
    // free basic block
    const BasicBlock *basic = block->container + i;
    free(basic->info);
    free(basic->variables);
  }
  free(block->container);
  free(block);
}

int cmp_use_info(const void *u1, const void *u2) {
  return cmp_operand(&((use_info *) u1)->op, &((use_info *) u2)->op);
}

#undef EFFECTIVE_CODE
#undef EFFECTIVE_OP
