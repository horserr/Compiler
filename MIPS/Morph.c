#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif

#define ELEM_SIZE 4

typedef void (*FuncPtr)(const Code *);
typedef const char *RegType;

static FILE *f = NULL;
static use_info *USE_INFO = NULL;

#define CHECK_FREE(i, op_) do { \
    assert(cmp_operand(&(USE_INFO[i].op), &(op_)) == 0);\
    if (!USE_INFO[i].in_use) freeRegForOp(op_);\
  } while (false)

/* variables is an array of pointers which takes charge
 * of variables in the current basic block */
static const Operand **variables = NULL;
static int CNT;

#define findInVariables(op) \
  findInArray(&(op), true, variables, CNT, sizeof(Operand*), cmp_operand)

// track the storage location of values
typedef struct {
  /** offset is related to frame pointer($fp),
   * reg_index is the index of register in registers' pool */
  int offset, reg_index;
} AddrDescriptor;

#define is_addr_descriptor_valid(ad) ((ad)->offset != 0)
#define is_in_reg(ad) ((ad)->reg_index != -1)
// an array of address descriptor, which is the same length of `variables`
AddrDescriptor *addr_descriptors = NULL;

static const RegType regsPool[] = {
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$v1", "$v0",
  "$a0", "$a1", "$a2", "$a3",
};
static const int LEN = ARRAY_LEN(regsPool);

// todo remove redundant op pointer in pair
typedef struct {
  int next_i, prev_i; // next index and previous index
  const Operand *op;
} pair;

static struct {
  int size;    // the number of allocated register and op pairs
  pair *pairs; // act as an array
} deque = {
  .size = 0,
  /** make pairs pointing to the second element in the
   * anonymous array and spare the first element as sentinel
   * use typeof to retrieve the anonymous type */
  .pairs = (pair[LEN + 1]){{.next_i = -1, .prev_i = -1}} + 1
};

// get the pair
#define get_p(i) (deque.pairs[(i)])
#define pairs_sentinel get_p(-1)
#define EMPTY_PAIR (pair){}
#define is_pair_empty(pair) (cmp_pair(&EMPTY_PAIR, pair) == 0)

static int cmp_pair(const void *a, const void *b) {
  return memcmp((pair *) a, (pair *) b, sizeof(pair));
}

static void printUnary(const char *name, RegType reg);
static void printBinary(const char *name, RegType reg1, RegType reg2);
static void printTernary(const char *name, RegType result,
                         RegType reg1, RegType reg2);

static void printAddImm(const RegType dst_reg, const RegType src_reg, const int val) {
  char *im = (char *) int2String(val);
  printTernary("addi", dst_reg, src_reg, im);
  free(im);
}

// fixme how to initialize this?
static struct {
  int frame;
  int stack;
} Offset;

#define EXPAND (-1)
#define SHRINK 1

// fixme may be remove fp option
static void adjustPtr(const char *T, const char mode, const int offset) {
  assert(either(mode, EXPAND, SHRINK) && offset >= 0);
  const int o = mode * offset;
  if (strcmp(T, "sp") == 0) {
    Offset.frame += o;
    Offset.stack += o;
    printAddImm("$sp", "$sp", o);
  } else {
    assert(strcmp(T, "fp") == 0);
    Offset.frame += o;
  }
}

static void printLoadImm(const RegType reg, const int val) {
  if (val == 0)
    return printBinary("move", reg, "$zero");

  char *im = (char *) int2String(val);
  printBinary("li", reg, im);
  free(im);
}

static void print_save_load(const char *T, const RegType reg,
                            const int offset, const RegType base_reg) {
  assert(strcmp(T, "sw") == 0 || strcmp(T, "lw") == 0);
  char buffer[16];
  sprintf(buffer, "%d(%s)", offset, base_reg);
  printBinary(T, reg, buffer);
}

// todo comment
static void print_spill_absorb(const char T, const AddrDescriptor *ad) {
  assert(either(T, 's', 'l'));
  print_save_load(T == 's' ? "sw" : "lw",
                  regsPool[ad->reg_index],
                  ad->offset, "$fp");
}

#define find_in_regs(reg) findInArray(&reg, false, regsPool, LEN, sizeof(RegType), cmp_str)

static bool is_reg_empty(const RegType reg) {
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);
  return is_pair_empty(&get_p(reg_index));
}

// helper function for validation
static bool all_regs_empty(const RegType exception) {
  assert(find_in_regs(exception) != -1);

  for (int i = 0; i < LEN; ++i) {
    if (strcmp(exception, regsPool[i]) == 0) continue;
    if (!is_pair_empty(&get_p(i))) return false;
  }
  return true;
}

static void linkReg(const int reg_index) {
  assert(reg_index != -1);
  // link to the rear
  pair *find = &get_p(reg_index);
  assert(is_pair_empty(find));
  find->next_i = -1;
  find->prev_i = pairs_sentinel.prev_i;
  // note: unable to set find->op right here.
  get_p(pairs_sentinel.prev_i).next_i = reg_index;
  pairs_sentinel.prev_i = reg_index;

  deque.size++;
}

/**
 * Relinquishes a register, removing it from the deque.
 * Updates the links of the previous and next pairs in the deque.
 *
 * @param reg_index The index of the register to relinquish.
 */
static void unlinkReg(const int reg_index) {
  assert(reg_index != -1);
  pair *pair_ = &get_p(reg_index);
  assert(!is_pair_empty(pair_));

  const int prev_i = pair_->prev_i;
  const int next_i = pair_->next_i;
  get_p(prev_i).next_i = next_i;
  get_p(next_i).prev_i = prev_i;
  // empty
  memset(pair_, 0, sizeof(pair));
  deque.size--;
}

/**
 * todo modify this comment
 * Spills the contents of a register to memory.
 * Finds the variable associated with the register, saves its value to memory,
 * and then relinquishes the register.
 */
static void spillReg(const RegType reg) {
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);
  const Operand *op = get_p(reg_index).op;
  const int variable_index = findInVariables(op);
  assert(variable_index != -1);
  AddrDescriptor *ad = addr_descriptors + variable_index;
  assert(ad->reg_index == reg_index);
  print_spill_absorb('s', ad);
  ad->reg_index = -1;
  unlinkReg(reg_index);
}

// move the ownership and link the register operand to the receiver
// mainly for passing the arguments between function invocation
static void adoptReg(const RegType reg, const Operand *receiver_op) {
  assert(receiver_op != NULL);
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);

  pair *pair_ = &get_p(reg_index);
  assert(is_pair_empty(pair_)); // the target register should be empty
  linkReg(reg_index);
  // note: link the register to the receiver operand
  pair_->op = receiver_op;
  const int variable_index = findInVariables(receiver_op);
  assert(variable_index != -1);

  // update the receiver's address descriptor
  AddrDescriptor *ad = addr_descriptors + variable_index;
  // has valid addr descriptor and register attaches to it
  assert(is_addr_descriptor_valid(ad) && !is_in_reg(ad));
  ad->reg_index = reg_index;
}

/**
 * Seizes a register for use.
 * If `target_reg` is not `NULL`, it attempts to seize the specified register.
 * If all registers are full, it spills the least recently used register to memory.
 *
 * @param target_reg The specific register to seize, or `NULL` to seize any available register.
 * @return The index of the seized register.
 */
static int seizeReg(const RegType target_reg) {
  int reg_index;
  if (target_reg != NULL) {
    reg_index = find_in_regs(target_reg);
    assert(reg_index != -1);
    if (!is_pair_empty(&get_p(reg_index))) {
      // isn't an empty pair
      spillReg(target_reg);
    }
    linkReg(reg_index);
    return reg_index;
  }

  if (deque.size == LEN) { // register is full
    reg_index = pairs_sentinel.next_i;
    spillReg(regsPool[reg_index]);
  } else
    reg_index = findInArray(&EMPTY_PAIR, false, deque.pairs, LEN, sizeof(pair), cmp_pair);

  linkReg(reg_index);
  return reg_index;
}
#undef is_pair_empty
#undef EMPTY_PAIR

/**
 * Allocates a register for the given operand.
 * If the operand is not already in a register, it allocates space on the stack
 * and assigns a register to it.
 * If the operand is already in a register, it simply returns the register.
 *
 * @param op The operand for which to allocate a register.
 * @param has_defined Indicates whether the operand has been defined.
 * @return The register type allocated for the operand.
 */
static RegType ensureReg(const Operand *op, const bool has_defined) {
  assert(op->kind != O_CONSTANT);
  const int variable_index = findInVariables(op);
  assert(variable_index != -1);

  AddrDescriptor *ad = addr_descriptors + variable_index;
  if (!is_addr_descriptor_valid(ad)) {
    // allocate space on stack
    adjustPtr("sp",EXPAND, ELEM_SIZE);
    ad->offset = Offset.frame;
  }
  // already saved on stack
  if (!is_in_reg(ad)) {
    ad->reg_index = seizeReg(NULL);
    get_p(ad->reg_index).op = op; // note: link register here.
    if (has_defined)
      print_spill_absorb('l', ad);
  }
  return regsPool[ad->reg_index];
}

static void freeRegForOp(const Operand *op) {
  assert(either(op->kind, O_VARIABLE, O_TEM_VAR));
  const int variable_index = findInVariables(op);
  assert(variable_index != -1);
  AddrDescriptor *ad = addr_descriptors + variable_index;
  assert(is_addr_descriptor_valid(ad) && is_in_reg(ad));
  // note: always write back to memory
  print_spill_absorb('s', ad);

  unlinkReg(ad->reg_index);
  ad->reg_index = -1;
}

// fixme complete this
static void flushReg() {
  // fixme save all values
  pairs_sentinel.next_i = pairs_sentinel.prev_i = -1;
  memset(deque.pairs, 0, sizeof(pair) * LEN);
}

#undef get_p
#undef pairs_sentinel

// todo when to initialize this
static struct { // no more than 4
  u_int8_t arg;
  u_int8_t param;
} counter;

static void printFUNCTION(const Code *code) {
  assert(code->kind == C_FUNCTION);
  fprintf(f, "%s:\n", code->as.unary.value_s);
#ifdef LOCAL
  fprintf(f, "# Prologue\n");
#endif

  Offset.frame = 0;
  adjustPtr("sp", EXPAND, ELEM_SIZE); // Offset.frame will be changed to -4
  print_save_load("sw", "$fp", 0, "$sp");
  printAddImm("$fp", "$sp",ELEM_SIZE);
  adjustPtr("sp", EXPAND, ELEM_SIZE);
  print_save_load("sw", "$ra", Offset.frame, "$fp");

  counter.param = counter.arg = 0;
}

/**
 * todo comment
 */
static void printPARAM(const Code *code) {
  assert(code->kind == C_PARAM);
  const Operand *op = &code->as.unary;
  assert(op->kind == O_VARIABLE);

  const int variable_index = findInVariables(op);
  assert(variable_index != -1);
  AddrDescriptor *ad = addr_descriptors + variable_index;

  if (counter.param < 4) { // allocate space on stack
    adjustPtr("sp", EXPAND, ELEM_SIZE);
    ad->offset = Offset.frame;

    char courier[4];
    sprintf(courier, "$a%d", counter.param);
    // pass the register's ownership to the operand
    adoptReg(courier, op); // ad->reg_index will be set within `adoptReg`
  } else {
    // the caller has already saved the value on stack
    ad->offset = (counter.param - 5) * ELEM_SIZE;
    ad->reg_index = -1; // unnecessary to load into register at present
  }
  counter.param++;
}

/**
 * @warning I have changed the sequence of argument from reverse
 * order to sequential order in "Compile.c"
 * @param code
 */
static void printARG(const Code *code) {
  assert(code->kind == C_ARG);
  const Operand *op = &code->as.unary;
  const RegType reg = ensureReg(op,true);

  if (counter.arg < 4) {
    const char courier[4];
    sprintf(courier, "$a%d", counter.arg);
    if (!is_reg_empty(courier)) spillReg(courier);
    printBinary("move", courier, reg);
  } else {
    adjustPtr("sp",EXPAND, ELEM_SIZE);
    print_save_load("sw", reg, Offset.frame, "$fp");
  }
  CHECK_FREE(0, op);
  counter.arg++;
}

static void printRETURN(const Code *code) {
  assert(code->kind == C_RETURN);
  const Operand *op = &code->as.unary;

  if (!is_reg_empty("$v0")) spillReg("$v0");
  if (op->kind == O_CONSTANT) {
    printLoadImm("$v0", op->value);
  } else {
    const RegType reg = ensureReg(op, true);
    printBinary("move", "$v0", reg);
    CHECK_FREE(0, op);
  }

  // param counter should be reset
  assert(counter.param == 0);
  // all registers should be freed before return.
  assert(all_regs_empty("$v0"));

  fprintf(f, "# epilogue\n");
  adjustPtr("sp", SHRINK, -Offset.frame);
  print_save_load("lw", "$fp", -ELEM_SIZE, "$sp");
  print_save_load("lw", "$ra", -2 * ELEM_SIZE, "$sp");
  printUnary("jr", "$ra");
  assert(Offset.frame == 0);
}

typedef struct {
  bool is_tmp;
  int reg_index;
} RegHelper;

static RegHelper printInvoke(const Operand *op) {
  /* - how to flush registers before return clause and function call
   * - function call leave arguments register alone and write back all used register ('$s')
   * - after the call the stack will be retrieved back, especially for those on-stack arguments
   * - use assert to check the arg value back to zero after call
   * - reinitialize arg counter when encounter call   */
  assert(op->kind == O_INVOKE);

  printUnary("jal", op->value_s);
  fprintf(f, "function returns\n");
  counter.arg = 0;
}

/**
 * helper function for `printASSIGN`, `printArithmatic`, `printADDI`
 * @param op the operand on the right-hand side
 * @return a struct determines whether the allocated register is temporary and its index
 * @note the reason why the parameter is a double pointer is that after passing
 * through this function, the direct pointer of operand needs to be changed, if originally
 * the operand's type if `O_DEREF` and `O_REF`.
 */
static RegHelper rightHelper(const Operand **op) {
#define helper_(is_tmp_, reg_index_) (RegHelper){\
  .is_tmp = is_tmp_, .reg_index = reg_index_\
}
  const int kind = (*op)->kind;
  assert(in(kind, 1 + EFFECTIVE_OP, O_CONSTANT));
  if (kind == O_INVOKE) return printInvoke(*op);

  int reg_index;
  if (kind == O_CONSTANT) {
    reg_index = seizeReg(NULL);
    printLoadImm(regsPool[reg_index], (*op)->value);
    return helper_(true, reg_index);
  }
  if (kind == O_DEREF) {
    distill_op(*op);
    const RegType op1_reg = ensureReg(*op, true);
    reg_index = seizeReg(NULL);
    print_save_load("lw", regsPool[reg_index], 0, op1_reg);
    CHECK_FREE(1, *op); // this register may not need any more.
    return helper_(true, reg_index);
  }
  // reference or variable
  if (kind == O_REFER)
    distill_op(*op);
  const RegType reg = ensureReg(*op, true);
  reg_index = find_in_regs(reg);
  assert(reg_index != -1);
  return helper_(false, reg_index);
#undef helper_
}

// todo move somewhere else
#undef findInVariables
#undef is_addr_descriptor_valid
#undef is_in_reg

// the below two macros are related with RegHelper

/** reg_ gets the register name within registers' pool */
#define reg_(op) regsPool[op##_helper.reg_index]
/** free_reg_ checks whether a register needs to be freed.
 *    if is temporary register, then directly mark empty
 *    else use normal `CHECK_FREE` macro
 */
#define free_reg_(i, op) \
  if (op##_helper.is_tmp) {\
    unlinkReg(op##_helper.reg_index);\
  } else {\
    CHECK_FREE(i, op);\
  }

/**
 * helper function for `printASSIGN`, `printArithmatic`, `printADDI`
 * @param kind only support assign, add, sub, mul, div
 * @param result the left-hand side operand
 * @param ... All parameters passed in are RegType, which are
 *    right-hand side operand(s).
 * If kind is assignment, then receives one argument; else receives two.
 *
 *  There are four parts of the below code. <p>
 *  First, it deals with a special case which is assignment.
 *    @if The result is `O_DEREF`, then use the given register of the
 *    right-hand side without sparing a new register.
 *    @else The normal case that uses move.</p><p>
 *  Second, it checks whether the result is `O_DEREF` type.
 *    @if It isn't, the code will only execute the first two parts.
 *    @else It will first tackle the right-hand side operand(s) aided
 *    with previously found temporary register and then save to memory.</p>
 */
static void leftHelper(const int kind, const Operand *result, ...) {
  assert(in(kind, 5, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV));
  va_list regs;
  va_start(regs, kind == C_ASSIGN ? 1 : 2);

  if (kind == C_ASSIGN) {
    const RegType right_reg = va_arg(regs, RegType);
    if (result->kind == O_DEREF) {
      distill_op(result);
      const RegType result_reg = ensureReg(result, false);
      print_save_load("sw", right_reg, 0, result_reg);
    } else {
      const RegType result_reg = ensureReg(result, false);
      printBinary("move", result_reg, right_reg);
    }
    CHECK_FREE(0, result);
    return va_end(regs);
  }

  int index;
  if (result->kind == O_DEREF) {
    index = seizeReg(NULL);
  } else {
    const RegType result_reg = ensureReg(result,false);
    index = find_in_regs(result_reg);
    assert(index != -1);
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
    unlinkReg(index);
    distill_op(result);
    const RegType result_reg = ensureReg(result, false);
    print_save_load("sw", regsPool[index], 0, result_reg);
  }
  CHECK_FREE(0, result);
}
#undef find_in_regs

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
    const RegType left_reg = ensureReg(left, false);
    printLoadImm(left_reg, right->value);
    CHECK_FREE(0, left);
    return;
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

static void printLABEL(const Code *code) {
  assert(code->kind == C_LABEL);
  fprintf(f, "label%d:\n", code->as.unary.var_no);
}

static void printGOTO(const Code *code) {
  assert(code->kind == C_GOTO);
  char buffer[20];
  sprintf(buffer, "label%d", code->as.unary.var_no);
  printUnary("j", buffer);
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
  const int index = findInArray(&relation, false, map,
                                ARRAY_LEN(map), sizeof(map[0]), cmp_str);
  assert(index != -1);
  RegType x_reg, y_reg;
  const Operand *x = &code->as.ternary.x;
  if (x->kind == O_CONSTANT) {
    x_reg = int2String(x->value);
  } else {
    assert(either(x->kind, O_TEM_VAR, O_VARIABLE));
    x_reg = ensureReg(x,true);
  }
  const Operand *y = &code->as.ternary.y;
  if (y->kind == O_CONSTANT) {
    y_reg = int2String(y->value);
  } else {
    assert(either(y->kind, O_TEM_VAR, O_VARIABLE));
    y_reg = ensureReg(y, true);
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

  const RegType dec = ensureReg(op,false);
  adjustPtr("sp", EXPAND, (int) code->as.dec.size);
  printBinary("move", dec, "$sp");
  CHECK_FREE(0, op);
}

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
    elem(ARG), elem(PARAM),
  };
  // todo remove this
  if (!in(code->kind,
          13,
          C_ASSIGN,
          C_ADD, C_SUB, C_MUL, C_DIV,
          C_RETURN, C_FUNCTION, C_LABEL, C_GOTO, C_DEC,
          C_IFGOTO, C_ARG, C_PARAM
  ))
    return;

  dispatch[code->kind](code);
#undef elem
}

static void initialize();

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
    // initialize variables and address descriptors
    variables = basic->variables;
    CNT = basic->cnt;
    addr_descriptors = calloc(CNT, sizeof(AddrDescriptor));
    for (int j = 0; j < CNT; ++j) addr_descriptors[j].reg_index = -1;
    /* Note that the length of 'info' array doesn't match
     * the number of chunks in basic blocks.
     * So it is necessary to spare a variable to keep track of it. */
    int info_index = 0;

    const Chunk *chunk = basic->begin;
    // loop through each line of code
    while (chunk != basic->end->next) {
      const Code *code = &chunk->code;
#ifdef LOCAL
      fprintf(f, "# ");
      printCode(f, code);
#endif
      if (in(code->kind, EFFECTIVE_CODE)) {
        // only increment the 'info_index' if it is effective code.
        USE_INFO = (basic->info + info_index++)->use;
      }
      print(code);
      chunk = chunk->next;
    }
    flushReg();
    free(addr_descriptors);
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

static void initialize() {
  const char *initialize_code =
      "\t.data\n"
      "_prompt: .asciiz \"Enter an integer:\"\n"
      "_ret: .asciiz \"\\n\"\n"
      "\n.globl __start\n"
      "\n.text\n"
      // FUNCTION __start
      "__start:\n"
      "\tjal main\n"
      "\tli $v0, 10\n"
      "\tsyscall\n"
      // FUNCTION read
      "\nread:\n"
      "\tli $v0, 4\n"
      "\tla $a0, _prompt\n"
      "\tsyscall\n"
      "\tli $v0, 5\n"
      "\tsyscall\n"
      "\tjr $ra\n"
      // FUNCTION write
      "\nwrite:\n"
      "\tli $v0, 1\n"
      "\tsyscall\n"
      "\tli $v0, 4\n"
      "\tla $a0, _ret\n"
      "\tsyscall\n"
      "\tmove $v0, $0\n"
      "\tjr $ra\n";
  fprintf(f, initialize_code);
}

// todo move this else where
#undef CHECK_FREE
#undef ELEM_SIZE
#undef EXPAND
#undef SHRINK
