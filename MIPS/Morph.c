#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif

typedef void (*FuncPtr)(const Code *);
typedef const char *RegType;

static FILE *f = NULL;
static use_info *USE_INFO = NULL;
#define CHECK_FREE(i, op_) do { \
    assert(cmp_operand(USE_INFO[i].op, (op_)) == 0);\
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

static void print_save_load(const char *T, const RegType reg,
                            const unsigned offset, const RegType base_reg) {
  assert(strcmp(T, "sw") == 0 || strcmp(T, "lw") == 0);
  char buffer[16];
  sprintf(buffer, "%u(%s)", offset, base_reg);
  printBinary(T, reg, buffer);
}

// always return the index of available register
static int getAvailReg() {
  if (deque.size == LEN) { // spill register
    // fixme
    assert(false);
  }
  const int index = findInArray(&EMPTY_PAIR, deque.pairs,
                                LEN, sizeof(pair), cmp_pair);
  assert(index != -1);
  return index;
}

static RegType allocateReg(const Operand *op) {
  const int index = getAvailReg();
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
  // fixme move sp and save variables
  print_save_load("lw", result_reg, 0, "hh");
  return result_reg;
}

static void freeReg(const Operand *op) {
  const int index = findInPairs(op);
  assert(index != -1);
  markRegEmpty(index);
}

static void flushReg() {
  // fixme save all values
  pairs_sentinel.next_i = pairs_sentinel.prev_i = -1;
  memset(deque.pairs, 0, sizeof(pair) * LEN);
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
    CHECK_FREE(0, op);
    printBinary("move", "$v0", reg);
  }
  printUnary("jr", "$ra");
}

static void printIFGOTO(const Code *code) {
  assert(code->kind == C_IFGOTO);
#define elem(s, m) { #s, #m }
  const struct {
    const char *symbol;
    const char *mnemonic;
  } map[] = {
        elem(==, beq), elem(!=, bne), elem(>, bgt),
        elem(<, blt), elem(>=, bge), elem(<=, ble),
      };
#undef elem
  const char *relation = code->as.ternary.relation;
  const int index = findInArray(&relation, map,
                                ARRAY_LEN(map), sizeof(map[0]), cmp_str);
  assert(index != -1);
  RegType x_reg, y_reg;
  const Operand *x = &code->as.ternary.x;
  if (x->kind == O_CONSTANT) {
    x_reg = int2String(x->value);
  } else {
    assert(either(x->kind, O_TEM_VAR, O_VARIABLE));
    x_reg = ensureReg(x);
  }
  const Operand *y = &code->as.ternary.y;
  if (y->kind == O_CONSTANT) {
    y_reg = int2String(y->value);
  } else {
    assert(either(y->kind, O_TEM_VAR, O_VARIABLE));
    y_reg = ensureReg(y);
  }
  char *label;
  asprintf(&label, "label%d", code->as.ternary.label.var_no);
  printTernary(map[index].mnemonic, x_reg, y_reg, label);
  free(label);

  if (isInteger(x_reg))
    free((void *) x_reg);
  else
    CHECK_FREE(0, x);
  if (isInteger(y_reg))
    free((void *) y_reg);
  else
    CHECK_FREE(1, y);
}

static void printDEC(const Code *code) {
  assert(code->kind == C_DEC);
  const Operand *op = &code->as.dec.target;

  // todo extract into a function
  fprintf(f, "addi $sp, $sp, -%d\n", code->as.dec.size);
  const RegType dec = ensureReg(op);
  // fixme move stack pointer and allocate space
  CHECK_FREE(0, op);
  printBinary("move", dec, "$sp");
}

/**
 * helper function for `printASSIGN`, `printArithmatic`, `printADDI`
 * @param kind only support assign, add, sub, mul, div
 * @param result the left-hand side operand
 * @param ... All parameters passed in are RegType, which are
 *    right-hand side operand(s).
 * If kind is assignment, then receives one argument; else receives two.
 *
 *  There are four parts of the below code.
 *  First, it deals with a special case which is assignment.
 *    @if The result is `O_DEREF`, then use the given register of the
 *    right-hand side without sparing a new register.
 *    @else The normal case that uses move.
 *  Second, it checks whether the result is `O_DEREF` type.
 *    @if It isn't, the code will only execute the first two parts.
 *    @else It will first tackle the right-hand side operand(s) aided
 *  with previously found temporary register and then save to memory.
 */
static void leftHelper(const int kind, const Operand *result, ...) {
  assert(in(kind, 5, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV));
  va_list regs;
  va_start(regs, kind == C_ASSIGN ? 1 : 2);
  if (kind == C_ASSIGN) {
    const RegType right_reg = va_arg(regs, RegType);
    if (result->kind == O_DEREF) {
      distill_op(result);
      const RegType result_reg = ensureReg(result);
      CHECK_FREE(0, result);
      print_save_load("sw", right_reg, 0, result_reg);
    } else {
      const RegType result_reg = ensureReg(result);
      CHECK_FREE(0, result);
      printBinary("move", result_reg, right_reg);
    }
    return va_end(regs);
  }

  int index;
  if (result->kind == O_DEREF) {
    index = getAvailReg();
    markRegEmpty(index);
  } else {
    ensureReg(result);
    index = findInPairs(result);
    assert(index != -1);
    CHECK_FREE(0, result);
  }

  const RegType op1_reg = va_arg(regs, RegType);
  const RegType op2_reg = va_arg(regs, RegType);
  if (kind == C_DIV) {
    printBinary("div", op1_reg, op2_reg);
    printUnary("mflo", regsPool[index]);
  } else {
    const char *name;
    if (isInteger(op2_reg)) {
      name = "addi";
    } else {
      const char *n[] = {[C_ADD] = "add", [C_SUB] = "sub", [C_MUL] = "mul"};
      name = n[kind];
    }
    printTernary(name, regsPool[index], op1_reg, op2_reg);
  }
  va_end(regs);

  if (result->kind == O_DEREF) {
    distill_op(result);
    const RegType result_reg = ensureReg(result);
    CHECK_FREE(0, result);
    print_save_load("sw", regsPool[index], 0, result_reg);
  }
}

typedef struct {
  bool is_tmp;
  int reg_index;
} RegHelper;

/**
 * helper function for `printASSIGN`, `printArithmatic`, `printADDI`
 * @param op the operand on the right-hand side
 * @return a struct determines whether the allocated register is temporary and its index
 * @note the reason why the parameter is a double pointer is that after passing
 * through this function, the direct pointer of operand needs to be changed, if originally
 * the operand's type if `O_DEREF` and `O_REF`.
 */
static RegHelper rightHelper(const Operand **op) {
  assert(in((*op)->kind,
    1 + EFFECTIVE_OP, O_CONSTANT));
  int index;
  bool is_tmp;
  if ((*op)->kind == O_CONSTANT) {
    is_tmp = true;
    index = getAvailReg();
    printLoadImm(regsPool[index], (*op)->value);
  } else if ((*op)->kind == O_DEREF) {
    is_tmp = true;
    distill_op(*op);
    const RegType op1_reg = ensureReg(*op);
    index = getAvailReg();
    CHECK_FREE(1, *op); // this register isn't needed any more.
    print_save_load("lw", regsPool[index],
                    0, op1_reg);
  } else { // reference or variable
    is_tmp = false;
    if ((*op)->kind == O_REFER)
      distill_op(*op);
    ensureReg(*op);
    index = findInPairs(*op);
    assert(index != -1);
  }
  return (RegHelper){.is_tmp = is_tmp, .reg_index = index};
}

// the below two macros are related with RegHelper
/** reg_ gets the register name within registers' pool */
#define reg_(op) regsPool[op##_helper.reg_index]
/** free_reg_ checks whether a register needs to be freed.
 *    @if is temporary register, then directly mark empty
 *    @else use normal `CHECK_FREE` macro
 */
#define free_reg_(i, op) \
  if (op##_helper.is_tmp) {\
    markRegEmpty(op##_helper.reg_index);\
  } else {\
    CHECK_FREE(i, op);\
  }

static void printASSIGN(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const Operand *right = &code->as.assign.right;
  const Operand *left = &code->as.assign.left;
  /** It is feasible to resort to `rightHelper` if the right
   * operand is constant though. But this leads to redundant
   * `li` and `move` commands which can be replaced with a
   * single `li`. */
  if (right->kind == O_CONSTANT &&
      either(left->kind, O_VARIABLE, O_TEM_VAR)) {
    const RegType left_reg = ensureReg(left);
    CHECK_FREE(0, left);
    return printLoadImm(left_reg, right->value);
  }

  const RegHelper right_helper = rightHelper(&right);
  free_reg_(1, right);

  leftHelper(C_ASSIGN, left, reg_(right));
}

static void printAddI(const Code *code) {
  assert(code->kind == C_ADD);
  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  assert(either(O_CONSTANT, op1->kind, op2->kind));

  if (op1->kind == O_CONSTANT)
    SWAP(op1, op2, sizeof(Operand));

  const RegHelper op1_helper = rightHelper(&op1);
  free_reg_(1, op1);

  const Operand *result = &code->as.binary.result;
  const RegType imme = int2String(op2->value);
  leftHelper(C_ADD, result, reg_(op1), imme);
  free((void *) imme);
}

static void printArithmatic(const Code *code) {
  const int kind = code->kind;
  assert(in(kind, 4, C_ADD, C_SUB, C_MUL, C_DIV));

  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  assert(!either(op2->kind, O_REFER, O_DEREF));
  if (kind == C_ADD && either(O_CONSTANT, op1->kind, op2->kind))
    return printAddI(code);

  const RegHelper op1_helper = rightHelper(&op1);
  const RegHelper op2_helper = rightHelper(&op2);
  free_reg_(1, op1);
  free_reg_(2, op2);

  const Operand *result = &code->as.binary.result;
  assert(result->kind != O_REFER);
  leftHelper(kind, result, reg_(op1), reg_(op2));
}
#undef free_reg_
#undef reg_

static void print(const Code *code) {
#define elem(T) [C_##T] = print##T
  const FuncPtr dispatch[] = {
    elem(ASSIGN),
    elem(FUNCTION), elem(GOTO), elem(LABEL), elem(RETURN), elem(DEC),
    elem(IFGOTO),
    [C_ADD] = printArithmatic,
    [C_SUB] = printArithmatic,
    [C_MUL] = printArithmatic,
    [C_DIV] = printArithmatic,
  };
  // todo remove this
  if (!in(code->kind,
          11,
          C_ASSIGN,
          C_ADD, C_SUB, C_MUL, C_DIV,
          C_RETURN, C_FUNCTION, C_LABEL, C_GOTO, C_DEC,
          C_IFGOTO
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
    flushReg();
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
