#ifdef LOCAL
#include <Optimize.h>
#else
#include "Optimize.h"
#endif

// remainder: only use these macros inside 'in' function
#define EFFECTIVE_CODE 5, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV
#define EFFECTIVE_OP   2, O_VARIABLE, O_TEM_VAR

typedef struct {
  size_t len;
  use_info *use;
} UseInfoTable;

extern const u_int8_t operand_count_per_code[];
int cmp_use_info(const void *u1, const void *u2);
// print code is only for debug purpose, so declare it as
// `extern` rather than add it in header file.
extern void printCode(FILE *f, const Code *c);

///// Conditions and Labels ////////////////////////////////

/**
 * IF a == b GOTO label1  |  IF a != b GOTO label2
 * GOTO label2            |  LABEL label1
 * LABEL label1           |
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
    const char *flipList[len] = {"==", "<", ">", "<=", ">=", "!="};
    char **relation = &c->as.ternary.relation;
    // search in flipList
    char **find = findInArray(relation, flipList,
                              len, sizeof(char *), cmp_str);
    assert(find != NULL);
    const int index = INDEX(flipList, find, sizeof(char *));
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
    if (bsearch(&c->as.unary.var_no, container,
                cnt, sizeof(int), cmp_int) != NULL)
      continue;
    removeCode(chunk->prev);
  }
  free(container);
}

///// Arithmatic ///////////////////////////////////////////

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

  // loop from begin to end
  const Chunk *chunk = basic->begin;
  while (chunk != basic->end->next) {
    const Code *code = &chunk->code;
    chunk = chunk->next;
    const int kind = code->kind;
    if (!in(kind, EFFECTIVE_CODE)) continue;

    // iterate through all operands per code
    for (int i = 0; i < operand_count_per_code[kind]; ++i) {
      const Operand *op = (Operand *) &code->as + i;
      // ignore constant
      if (!in(op->kind, EFFECTIVE_OP)) continue;

      if (*len >= capacity) { //resize
        RESIZE(capacity);
        *use = realloc(*use, sizeof(use_info) * capacity);
      }
      (*use)[*len].in_use = false;
      (*use)[(*len)++].op = op;
    }
  }
  // sort and remove duplicates
  qsort(*use, *len, sizeof(use_info), cmp_use_info);
  removeDuplicates(*use, len, sizeof(use_info), cmp_use_info);
  return table;
}

static void freeUseInfoTable(UseInfoTable *table) {
  assert(table != NULL);
  free(table->use);
  free(table);
}

#define search(key, table, cmp_fn) \
    bsearch(&key, table->use, table->len, sizeof(use_info), cmp_fn)

/**
 * @brief update use-info table with <b>'effective'</b> variables of the given current code
 * <p>
 * * helper function of `setBasicBlock`
 */
static void updateUseInfoTable(const Code *code, const UseInfoTable *table) {
#define update(target_op, is_alive, table, cmp_fn) do {\
    use_info *find = search((use_info){.op = target_op}, table, cmp_fn);\
    assert(find != NULL);\
    find->in_use = is_alive;\
  } while (false)

  assert(table != NULL);
  const int kind = code->kind;
  assert(in(kind, EFFECTIVE_CODE));

  // result operand is always the first operand lower in memory
  const Operand *result = (Operand *) &code->as;
  assert(in(result->kind, EFFECTIVE_OP));
  update(result, false, table, cmp_use_info);

  // iterate through those operands that are on the right side of equal sign
  for (int i = 1; i < operand_count_per_code[kind]; ++i) {
    const Operand *op = result + i;
    // do not update variables whose kind aren't within effective operands
    if (!in(op->kind, EFFECTIVE_OP)) continue;
    update(op, true, table, cmp_use_info);
  }
#undef update
}

/**
 * @brief copy the current state of use-info table into the info list.
 * for each code, only copy those effective variables that appear in it.
 */
static void setUseInfo(info *info, const Chunk *chunk, const UseInfoTable *table) {
  info->currentLine = chunk;
  const int kind = chunk->code.kind;
  assert(in(kind, EFFECTIVE_CODE));

  for (int i = 0; i < operand_count_per_code[kind]; ++i) {
    const Operand *op = (Operand *) &chunk->code.as + i;
    // only search effective operands
    if (!in(op->kind, EFFECTIVE_OP)) continue;

    const use_info *find = search((use_info){.op = op}, table, cmp_use_info);
    assert(find != NULL);
    memcpy(info->use + i, find, sizeof(use_info));
  }
}

#undef search

// set info for each line of code inside basic block
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
    // update the use-info table with the current code
    updateUseInfoTable(&chunk->code, table);
    // get a snapshot of the current state of use-info table
    setUseInfo(&(*info_)[(*len)++], chunk, table);
  CONTINUE:
    chunk = chunk->prev;
    continue;
  RESIZE:
    RESIZE(capacity);
    *info_ = realloc(*info_, sizeof(info) * capacity);
  }
  reverseArray(*info_, *len, sizeof(info));
  freeUseInfoTable(table);
}

// for each basic blocks of block, set their info array
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
    // fixme include function call and other types as well
    if (!in(c->kind, 3, C_LABEL, C_IFGOTO, C_GOTO))
      goto CONTINUE;
    if (*cnt >= capacity) goto RESIZE;

    const Chunk *target = c->kind == C_LABEL ? chunk : chunk->next;
    // check for duplication before add
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
 *
 * <p>
 *  t0 := #2      |  t1 := 6
 *  t1 := t0 * 3  |
 */
static bool optimizeArithmatic(const Chunk *sentinel) {
  bool flag = false;
  Chunk *chunk = sentinel->next;
  while (chunk != sentinel) {
    Code *c = &chunk->code;
    chunk = chunk->next; // chunk is now pointing to next code
    foldConstant(c);
    organizeOpSequence(c);

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
  const BasicBlock *basic = block->container;
  for (int i = 0; i < basic->len; ++i) {
    const Chunk *chunk = basic->info[i].currentLine;
    // all code are effective if they are inside 'info' array
    assert(in(chunk->code.kind, EFFECTIVE_CODE));
    printCode(stdout, &chunk->code);
  }
}

Block* optimize(const Chunk *sentinel) {
  while (optimizeArithmatic(sentinel)) { /* do nothing */ }
  flipCondition(sentinel);
  deleteLabels(sentinel);
  Block *block = partitionChunk(sentinel);

  printBlock(block);
  return block;
}

///// utilities functions //////////////////////////////////
void freeBlock(Block *block) {
  assert(block != NULL);
  for (int i = 0; i < block->cnt; ++i) {
    // free basic block
    free((block->container + i)->info);
  }
  free(block->container);
  free(block);
}

int cmp_use_info(const void *u1, const void *u2) {
  return cmp_operand(((use_info *) u1)->op, ((use_info *) u2)->op);
}

#undef EFFECTIVE_CODE
#undef EFFECTIVE_OP
