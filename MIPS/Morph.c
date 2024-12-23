#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif
#include <complex.h>

typedef void (*FuncPtr)(const Code *);
typedef const char *RegType;

static FILE *f = NULL;
static use_info *USE_INFO = NULL;
#define CHECK_FREE(i, op_) do { \
    assert(cmp_operand(USE_INFO[i].op, op_) == 0);\
    if (!USE_INFO[i].in_use) freeReg(op_);\
  } while (false)

static const RegType regsPool[] = {
  "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13",
  "$v0", "$v1",
  "$a0", "$a1", "$a2", "$a3",
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
};
static const int LEN = ARRAY_LEN(regsPool);

typedef struct {
  int next_i, prev_i; // next index and previous index
  const Operand *op;
} pair;

#define EMPTY_PAIR (pair){.next_i = 0, .prev_i = 0}

struct {
  int size;    // the number of allocated register and op pairs
  pair *pairs; // act as an array
} deque = {
  .size = 0,
  /** make pairs pointing to the second element in the
   * anonymous array and spare the first element as sentinel
   * use typeof to retrieve the anonymous type */
  .pairs = (pair[LEN + 1]){{.next_i = -1, .prev_i = -1}} + 1
};

#define pairs_sentinel (deque.pairs[-1])

static void printUnary(const char *name, RegType reg);
static void printBinary(const char *name, RegType reg1, RegType reg2);
static void printTernary(const char *name, RegType result,
                         RegType reg1, RegType reg2);

static int cmp_pair(const void *a, const void *b) {
  const pair *pair_a = (pair *) a, *pair_b = (pair *) b;
  if (pair_a->prev_i != pair_b->prev_i) {
    return pair_a->prev_i - pair_b->prev_i;
  }
  return pair_a->next_i - pair_b->next_i;
}

static void printLoadImm(const RegType reg, const int val) {
  if (val == 0)
    return printBinary("move", reg, "$zero");

  char *im = (char *) int2String(val);
  printBinary("li", reg, im);
  free(im);
}

static void printSaveOrLoad(const char *T, const RegType reg,
                            const unsigned offset, const RegType base_reg) {
  assert(strcmp(T, "sw") == 0 || strcmp(T, "lw") == 0);
  char buffer[16];
  sprintf(buffer, "%u(%s)", offset, base_reg);
  printBinary(T, reg, buffer);
}

// always return the index of available register
static int findAvailReg() {
  if (deque.size == LEN) { // spill register
    // todo
    assert(false);
  }
  const int index = findInArray(&EMPTY_PAIR, deque.pairs,
                                LEN, sizeof(pair), cmp_pair);
  assert(index != -1);
  return index;
}

static RegType allocateReg(const Operand *op) {
  const int index = findAvailReg();
  pair *find = &deque.pairs[index];
  find->next_i = -1;
  find->prev_i = pairs_sentinel.prev_i;
  deque.pairs[pairs_sentinel.prev_i].next_i = index;
  pairs_sentinel.prev_i = index;
  deque.size++;
  find->op = op;
  return regsPool[index];
}

/**
 * @return find, the index of the target pair in chained deque
 * @reutrn -1 if not find
 */
static int findInPairs(const Operand *op) {
  int index = pairs_sentinel.next_i;
  while (index != -1) {
    const pair *pair_ = &deque.pairs[index];
    if (cmp_operand(pair_->op, op) == 0)
      break;
    index = pair_->next_i;
  }
  return index;
}

static void markRegEmpty(const int index) {
  assert(index != -1);
  pair *pair_ = &deque.pairs[index];
  int *prev_i = &pair_->prev_i;
  int *next_i = &pair_->next_i;
  deque.pairs[*prev_i].next_i = *next_i;
  deque.pairs[*next_i].prev_i = *prev_i;

  *prev_i = *next_i = 0;
}

static RegType ensureReg(const Operand *op) {
  assert(op->kind != O_CONSTANT);
  const int index = findInPairs(op);
  if (index != -1)
    return regsPool[index];
  // not find
  const RegType result_reg = allocateReg(op);
  // todo
  printSaveOrLoad("lw", result_reg, 0, "hh");
  return result_reg;
}

static void freeReg(const Operand *op) {
  const int index = findInPairs(op);
  assert(index != -1);
  markRegEmpty(index);
}
#undef pairs_sentinel
#undef EMPTY_PAIR

static void printLABEL(const Code *code) {
  assert(code->kind == C_LABEL);
  fprintf(f, "label%d:\n", code->as.unary.var_no);
}

static void printFUNCTION(const Code *code) {
  assert(code->kind == C_FUNCTION);
  fprintf(f, "%s:\n", code->as.unary.value_s);
}

static void printGOTO(const Code *code) {
  assert(code->kind == C_GOTO);
  char buffer[20];
  sprintf(buffer, "label%d", code->as.unary.var_no);
  printUnary("j", buffer);
}

static void printRETURN(const Code *code) {
  assert(code->kind == C_RETURN);
  const Operand *op = &code->as.unary;
  if (op->kind == O_CONSTANT) {
    printLoadImm("$v0", op->value);
  } else {
    const RegType reg = ensureReg(op);
    CHECK_FREE(0, op); // fixme here is a problem
    printBinary("move", "$v0", reg);
  }
  printUnary("jr", "$ra");
}

// static void printIFGOTO(const Code *code) {
//   assert(code->kind == C_IFGOTO);
// #define elem(s, m) { #s, #m }
//   const struct {
//     const char *symbol;
//     const char *mnemonic;
//   } map[] = {
//         elem(==, beq), elem(!=, bne), elem(>, bgt),
//         elem(<, blt), elem(>=, bge), elem(<=, ble),
//       };
// #undef elem
//   // todo bsearch
//   // search in map
//   const char *name = NULL;
//   for (int i = 0; i < ARRAY_LEN(map); ++i) {
//     if (strcmp(map[i].symbol, code->as.ternary.relation) == 0) {
//       name = map[i].mnemonic;
//       break;
//     }
//   }
//   assert(name != NULL);
//   char *label;
//   asprintf(&label, "label%d", code->as.ternary.label.var_no);
//   printTernary(name, getReg(&code->as.ternary.x),
//                getReg(&code->as.ternary.y), label);
//   free(label);
// }

static void printASSIGN(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const Operand *left = &code->as.assign.left;
  const Operand *right = &code->as.assign.right;
  assert(right->kind != O_REFER);
  assert(!(left->kind == O_DEREF && right->kind == O_DEREF));

  if (right->kind == O_CONSTANT) {
    const RegType left_reg = allocateReg(left);
    CHECK_FREE(0, left);
    printLoadImm(left_reg, right->value);
    return;
  }

  const RegType right_reg = ensureReg(right);
  CHECK_FREE(1, right);
  const RegType left_reg = allocateReg(left);
  CHECK_FREE(0, left);

  if (left->kind == O_DEREF) {
    // todo
    printSaveOrLoad("sw", left_reg, 0, right_reg);
    return;
  }
  if (right->kind == O_DEREF) {
    // todo
    printSaveOrLoad("lw", left_reg, 0, right_reg);
    return;
  }
  printBinary("move", left_reg, right_reg);
}

static void printAddI(const Code *code) {
  assert(code->kind == C_ADD);
  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  assert(in(O_CONSTANT,2, op1->kind, op2->kind));

  if (op1->kind == O_CONSTANT)
    SWAP(op1, op2, sizeof(Operand));
  // op1 is variable while op2 is constant
  const RegType op1_reg = ensureReg(op1);
  CHECK_FREE(1, op1);

  const Operand *result = &code->as.binary.result;
  const RegType result_reg = allocateReg(result);
  CHECK_FREE(0, result);
  char *immediate = (char *) int2String(op2->value);
  printTernary("addi", result_reg, op1_reg, immediate);
  free(immediate);
}

static void printArithmatic(const Code *code) {
  const int kind = code->kind;
  assert(in(kind, 4, C_ADD, C_SUB, C_MUL, C_DIV));

  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  if (kind == C_ADD && in(O_CONSTANT, 2, op1->kind, op2->kind))
    return printAddI(code);

  RegType op1_reg, op2_reg;
  int index1 = -1, index2 = -1;
  // if one of the operands is constant, need to first load immediate
  if (op1->kind == O_CONSTANT) {
    index1 = findAvailReg();
    op1_reg = regsPool[index1];
    printLoadImm(op1_reg, op1->value);
  } else {
    op1_reg = ensureReg(op1);
    CHECK_FREE(1, op1);
  }

  if (op2->kind == O_CONSTANT) {
    index2 = findAvailReg();
    op2_reg = regsPool[index2];
    printLoadImm(op2_reg, op2->value);
  } else {
    op2_reg = ensureReg(op2);
    CHECK_FREE(2, op2);
  }

  const RegType result_reg = allocateReg(&code->as.binary.result);
  if (kind == C_DIV) {
    printBinary("div", op1_reg, op2_reg);
    printUnary("mflo", result_reg);
  } else {
    const char *name[] = {[C_ADD] = "add", [C_SUB] = "sub", [C_MUL] = "mul"};
    printTernary(name[kind], result_reg, op1_reg, op2_reg);
  }
  // free the temporary register if the operand is constant
  if (index1 != -1) markRegEmpty(index1);
  if (index2 != -1) markRegEmpty(index2);
}

static void print(const Code *code) {
#define elem(T) [C_##T] = print##T
  const FuncPtr dispatch[] = {
    elem(ASSIGN),
    elem(FUNCTION), elem(GOTO), elem(LABEL), elem(RETURN),
    // elem(IFGOTO),
    [C_ADD] = printArithmatic,
    [C_SUB] = printArithmatic,
    [C_MUL] = printArithmatic,
    [C_DIV] = printArithmatic,
  };
  // todo remove this
  if (!in(code->kind,
          9,
          C_ASSIGN,
          C_ADD, C_SUB, C_MUL, C_DIV,
          C_RETURN, C_FUNCTION, C_LABEL, C_GOTO
  ))
    return;

  dispatch[code->kind](code);
#undef elem
}

static void initialize() {
#define unary(name, op) \
  "\t" #name " " #op "\n"
#define binary(name, op1, op2) \
  "\t" #name " " #op1 ", " #op2 "\n"

  fprintf(f, ".data\n");
  fprintf(f, "_prompt: .asciiz \"Enter an integer:\"\n");
  fprintf(f, "_ret: .asciiz \"\\n\"\n");
  fprintf(f, "\n.globl __start\n");
  fprintf(f, "\n.text");
  // FUNCTION __start
  fprintf(f, "\n__start:\n");
  fprintf(f, unary(jal, main));
  fprintf(f, binary(li, $v0, 10));
  fprintf(f, "\tsyscall\n");
  // FUNCTION read
  fprintf(f, "\nread:\n");
  fprintf(f, binary(li, $v0, 4));
  fprintf(f, binary(la, $a0, _prompt));
  fprintf(f, "\tsyscall\n");
  fprintf(f, binary(li, $v0, 5));
  fprintf(f, "\tsyscall\n");
  fprintf(f, unary(jr, $ra));
  // FUNCTION write
  fprintf(f, "\nwrite:\n");
  fprintf(f, binary(li, $v0, 1));
  fprintf(f, "\tsyscall\n");
  fprintf(f, binary(li, $v0, 4));
  fprintf(f, binary(la, $a0, _ret));
  fprintf(f, "\tsyscall\n");
  fprintf(f, binary(move, $v0, $0));
  fprintf(f, unary(jr, $ra));
#undef unary
#undef binary
}

void printMIPS(const char *file_name, const Block *blocks) {
  f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("%s %s.\n", strerror(errno), file_name);
    exit(EXIT_FAILURE);
  }
  // initialize();
  // loop through each basic blocks
  for (int i = 0; i < blocks->cnt; ++i) {
    const BasicBlock *basic = blocks->container + i;
    // Note that the length of 'info' array doesn't match the number of chunks in
    // basic blocks. So it is necessary to spare a variable to keep track of it.
    int info_index = 0;

    const Chunk *chunk = basic->begin;
    // loop through each line of code
    while (chunk != basic->end->next) {
      const Code *code = &chunk->code;
      if (in(code->kind, EFFECTIVE_CODE)) {
        // only increment the 'info_index' if it is effective code.
        USE_INFO = (basic->info + info_index++)->use;
      }
      print(code);
      chunk = chunk->next;
    }
  }
  fclose(f);
}

static void printUnary(const char *name, const RegType reg) {
  fprintf(f, "\t%s %s\n", name, reg);
}

static void printBinary(const char *name, const RegType reg1, const RegType reg2) {
  fprintf(f, "\t%s %s, %s\n", name, reg1, reg2);
}

static void printTernary(const char *name, const RegType result,
                         const RegType reg1, const RegType reg2) {
  fprintf(f, "\t%s %s, %s, %s\n", name, result, reg1, reg2);
}

// todo move this else where
#undef CHECK_FREE
