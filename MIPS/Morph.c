#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#define PRINT_INFO(op, message...) \
  long _back_fill_pos = -1;\
  fprintf(f, "%*s#++ '%-20s' ", INDENT, "", __FUNCTION__);\
  if (op) {\
    fprintf(f," (");\
    printOp(f, op);\
    fseek(f, -1, SEEK_CUR);\
    fprintf(f,") <->    ");\
    _back_fill_pos = ftell(f) - 3;\
    }\
  fprintf(f, message);\
  fprintf(f, "\n");

#define BACK_FILL(reg) \
    assert(_back_fill_pos != -1);\
    fseek(f, _back_fill_pos, SEEK_SET);\
    fprintf(f, "%s", reg);\
    fseek(f, 0, SEEK_END);

#else
#include "IR"
#include "Morph.h"
#define PRINT_INFO(op, message...)
#define BACK_FILL(reg)
#endif

#define ELEM_SIZE 4

typedef void (*FuncPtr)(const Code *);
typedef const char *RegType;

static FILE *f = NULL;
static use_info *USE_INFO = NULL;

#define CHECK_FREE(i, op_) do { \
    assert(cmp_operand(&(USE_INFO[i].op), &(op_)) == 0);\
    if (!USE_INFO[i].in_use) freeReg(op_);\
  } while (false)

/* variables is an array of pointers which takes charge
 * of variables in the current basic block */
static const Operand **variables = NULL;
static int CNT;

// track the storage location of values
typedef struct {
  /** offset is related to frame pointer($fp),
   * reg_index is the index of register in registers' pool */
  int offset, reg_index;
} AddrDescriptor;

#define is_addr_descriptor_valid(ad) ((ad)->offset != -1)
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

static AddrDescriptor* getAddrDescriptor(const Operand *op) {
#define findInVariables(op) \
  findInArray(&(op), true, variables, CNT, sizeof(Operand*), cmp_operand)

  assert(either(op->kind, O_TEM_VAR, O_VARIABLE));
  const int variable_index = findInVariables(op);
  assert(variable_index != -1);
  return addr_descriptors + variable_index;
#undef findInVariables
}

static bool all_addr_descriptor_valid() {
  for (int i = 0; i < CNT; ++i) {
    const AddrDescriptor *ad = addr_descriptors + i;
    if (!is_addr_descriptor_valid(ad)) return false;
  }
  return true;
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

/** initialize when function starts, increment when encounters a new operand
 * adjusts(increment and decrement) when arguments are more than 4 */
int frameOffset;

#define EXPAND (-1)
#define SHRINK 1

static void adjustPtr(const char mode, const int offset) {
  assert(either(mode, EXPAND, SHRINK) && offset >= 0);
  const int o = mode * offset;
  frameOffset += o;
  printAddImm("$sp", "$sp", o);
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

static void print_spill_absorb(const char T, const AddrDescriptor *ad) {
  PRINT_INFO(NULL, "");
  assert(either(T, 's', 'l'));
  print_save_load(T == 's' ? "sw" : "lw",
                  regsPool[ad->reg_index],
                  ad->offset, "$fp");
}

// helper function for ensureReg and printInvoke
static void init_addr_descriptor(AddrDescriptor *ad) {
  PRINT_INFO(NULL, " OFFSET %d", frameOffset - 4);
  // must be uninitialized
  assert(!is_addr_descriptor_valid(ad));
  adjustPtr(EXPAND, ELEM_SIZE);
  ad->offset = frameOffset;
  assert(!is_in_reg(ad));
}

#define find_in_regs(reg) findInArray(&reg, false, regsPool, LEN, sizeof(RegType), cmp_str)

static bool is_reg_empty(const RegType reg) {
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);
  return is_pair_empty(&get_p(reg_index));
}

// helper function for validation
static bool all_regs_empty() {
  for (int i = 0; i < LEN; ++i) {
    if (!is_pair_empty(&get_p(i))) return false;
  }
  return true;
}

// todo comment
// simply print the mips code without truly modify the
// register and address descriptor
static void fake_flush_restore(const char T, const RegType exception) {
  PRINT_INFO(NULL, "");
  assert(either(T, 's','l'));
  const int except_index = exception == NULL ? -1 : find_in_regs(exception);

  for (int i = 0; i < LEN; ++i) {
    const pair *p = &get_p(i);
    if (i == except_index || is_pair_empty(p)) continue;
    print_spill_absorb(T, getAddrDescriptor(p->op));
  }
}

static void addReg(const int reg_index) {
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
static void removeReg(const int reg_index) {
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

static void unlinkBothSide(const int reg_index) {
  const pair *p = &get_p(reg_index);
  assert(!is_pair_empty(p));
  AddrDescriptor *ad = getAddrDescriptor(p->op);
  assert(is_addr_descriptor_valid(ad));
  assert(is_in_reg(ad) && ad->reg_index == reg_index);

  removeReg(reg_index);
  ad->reg_index = -1;
}

/**
 * todo modify this comment
 * Spills the contents of a register to memory.
 * Finds the variable associated with the register, saves its value to memory,
 * and then relinquishes the register.
 */
static void spillReg(const RegType reg) {
  PRINT_INFO(NULL, "");
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);
  const Operand *op = get_p(reg_index).op;
  const AddrDescriptor *ad = getAddrDescriptor(op);
  assert(ad->reg_index == reg_index);

  print_spill_absorb('s', ad);
  unlinkBothSide(reg_index);
}

// move the ownership and link the register operand to the receiver
// mainly for passing the arguments between function invocation
static void adoptReg(const RegType reg, const bool reg_must_empty,
                     const Operand *receiver_op) {
  PRINT_INFO(receiver_op, "adopt <%s>", reg);
  assert(receiver_op != NULL &&
    either(receiver_op->kind, O_TEM_VAR, O_VARIABLE));
  const int reg_index = find_in_regs(reg);
  assert(reg_index != -1);

  pair *pair_ = &get_p(reg_index);
  if (!is_pair_empty(pair_)) {
    assert(!reg_must_empty);
    /* note: no need to spill, because it guarantees
     *  to spill register before `adoptReg` is called */
    unlinkBothSide(reg_index);
  }
  addReg(reg_index);
  // note: link the register to the receiver operand
  pair_->op = receiver_op;

  // update the receiver's address descriptor
  AddrDescriptor *ad = getAddrDescriptor(receiver_op);
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
#define find_in_pairs(pair_) findInArray(&pair_, false, deque.pairs, LEN, sizeof(pair), cmp_pair);
  PRINT_INFO(NULL, "");
  int reg_index;
  if (target_reg != NULL) {
    reg_index = find_in_regs(target_reg);
    assert(reg_index != -1);
    if (!is_pair_empty(&get_p(reg_index))) {
      // isn't an empty pair
      spillReg(target_reg);
    }
    addReg(reg_index);
    return reg_index;
  }

  if (deque.size == LEN) { // register is full
    reg_index = pairs_sentinel.next_i;
    spillReg(regsPool[reg_index]);
  } else
    reg_index = find_in_pairs(EMPTY_PAIR);

  addReg(reg_index);
  return reg_index;
#undef find_in_pairs
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
  PRINT_INFO(op, "");
  assert(op->kind != O_CONSTANT);
  AddrDescriptor *ad = getAddrDescriptor(op);

  if (!is_addr_descriptor_valid(ad)) {
    assert(!has_defined);
    init_addr_descriptor(ad);
  }
  // already saved on stack
  if (!is_in_reg(ad)) {
    ad->reg_index = seizeReg(NULL);
    get_p(ad->reg_index).op = op; // note: link register here.
    if (has_defined)
      print_spill_absorb('l', ad);
  }
  const RegType reg = regsPool[ad->reg_index];
  BACK_FILL(reg);
  return reg;
}

static void freeReg(const Operand *op) {
  PRINT_INFO(op, "");
  assert(either(op->kind, O_VARIABLE, O_TEM_VAR));
  const AddrDescriptor *ad = getAddrDescriptor(op);
  assert(is_addr_descriptor_valid(ad) && is_in_reg(ad));
  // note: always write back to memory
  const RegType reg = regsPool[ad->reg_index];
  spillReg(reg);
  BACK_FILL(reg);
}
#undef get_p
#undef pairs_sentinel

/** both of parameters' and arguments' counter will be
 * initialized at the beginning of function, while only
 * arguments' counter will be reset after the function invocation. */
static struct { // no more than 4
  u_int8_t arg;
  u_int8_t param;
} counter;

static void printFUNCTION(const Code *code) {
  assert(code->kind == C_FUNCTION);
  fprintf(f, "%s:\n", code->as.unary.value_s);

  frameOffset = 0;
  adjustPtr(EXPAND, 2 * ELEM_SIZE); // Offset.frameOffset will be changed to -4
  print_save_load("sw", "$fp", 4, "$sp");
  print_save_load("sw", "$ra", 0, "$sp");
  printAddImm("$fp", "$sp", 2 * ELEM_SIZE);

  counter.param = counter.arg = 0;
}

/**
 * todo comment
 */
static void printPARAM(const Code *code) {
  assert(code->kind == C_PARAM);
  const Operand *receiver = &code->as.unary;
  assert(receiver->kind == O_VARIABLE);

  AddrDescriptor *ad = getAddrDescriptor(receiver);
  if (counter.param < 4) { // allocate space on stack
    init_addr_descriptor(ad);

    char courier[4];
    snprintf(courier, sizeof(courier), "$a%d", counter.param);
    // pass the register's ownership to the operand
    adoptReg(courier, true, receiver); // ad->reg_index will be set within `adoptReg`
    CHECK_FREE(0, receiver);           // note: remember to free
  } else {
    // the caller has already saved the value on stack
    ad->offset = (counter.param - 4) * ELEM_SIZE;
    assert(!is_in_reg(ad)); // unnecessary to load into register at present
  }
  counter.param++;
}
#undef is_in_reg

// todo comment, the effect of the function is to leave the arguments register
/**
 * @warning I have changed the sequence of argument from reverse
 * order to sequential order in "Compile.c"
 * @param code
 */
static void printARG(const Code *code) {
  assert(code->kind == C_ARG);
  const Operand *provider = &code->as.unary;

  if (provider->kind == O_CONSTANT) {
    if (counter.arg < 4) {
      const char courier[4];
      snprintf(courier, sizeof(courier), "$a%d", counter.arg);
      if (!is_reg_empty(courier)) spillReg(courier);
      printLoadImm(courier, provider->value);
    } else {
      adjustPtr(EXPAND, ELEM_SIZE);
      const int reg_index = seizeReg(NULL);
      printLoadImm(regsPool[reg_index], provider->value);
      print_save_load("sw", regsPool[reg_index], frameOffset, "$fp");
      removeReg(reg_index);
    }
    goto RETURN;
  }

  const RegType reg = ensureReg(provider,true);
  if (counter.arg < 4) {
    const char courier[4];
    snprintf(courier, sizeof(courier), "$a%d", counter.arg);
    if (!is_reg_empty(courier)) spillReg(courier);

    // can't use check free, because the register has unlinked
    if (strcmp(courier, reg) == 0) goto RETURN;
    printBinary("move", courier, reg);
  } else {
    adjustPtr(EXPAND, ELEM_SIZE);
    print_save_load("sw", reg, frameOffset, "$fp");
  }
  CHECK_FREE(0, provider);
RETURN:
  counter.arg++;
}

static void printRETURN(const Code *code) {
  assert(code->kind == C_RETURN);
  const Operand *provider = &code->as.unary;

  const RegType courier = "$v0";
  if (provider->kind == O_CONSTANT) {
    if (!is_reg_empty(courier)) spillReg(courier);
    printLoadImm(courier, provider->value);
  } else {
    const RegType reg = ensureReg(provider, true);
    if (!is_reg_empty(courier)) spillReg(courier);
    if (strcmp(courier, reg) != 0) {
      printBinary("move", courier, reg);
      CHECK_FREE(0, provider);
    }
  }

  // arg counter should be reset
  assert(counter.arg == 0);
  // all registers should be freed before return, including $v0.
  assert(all_regs_empty());
  assert(all_addr_descriptor_valid());

  adjustPtr(SHRINK, -frameOffset);
  print_save_load("lw", "$ra", -2 * ELEM_SIZE, "$sp");
  print_save_load("lw", "$fp", -ELEM_SIZE, "$sp");
  printUnary("jr", "$ra");
  assert(frameOffset == 0);
}

static void printInvoke(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const Operand *result = &code->as.assign.left;
  // the result type must be variable
  assert(either(result->kind, O_TEM_VAR, O_VARIABLE));
  const Operand *invoke = &code->as.assign.right;
  assert(invoke->kind == O_INVOKE);

#ifdef LOCAL
  // guarantee that all argument register is empty
  for (int i = 0; i < counter.arg; ++i) {
    if (i >= 4) break;
    const char courier[4];
    snprintf(courier, sizeof(courier), "$a%d", i);
    assert(is_reg_empty(courier));
  }
#endif

  // pretend to flush all registers while leaving registers(deque) untouched
  fake_flush_restore('s', NULL);
  // for now, all registers' value is in memory.
  printUnary("jal", invoke->value_s);

  /* Allocate space on stack if result operand hasn't been initialized
   * note:
   *  The reason why I don't resort to ensureReg is that the later usage of
   *    adoptReg forces the receiver operand mustn't have any registers attached to it.
   *  In addition, it is also unnecessary to grab a register and then move
   *    the return value to it.
   */
  AddrDescriptor *ad = getAddrDescriptor(result);
  if (!is_addr_descriptor_valid(ad)) {
    init_addr_descriptor(ad);
  }

  /* note:
   *  use `adoptReg` rather than `printBinary("move", result_reg, "$v0");`,
       because the result operand will be refreshed anyway.
   *  adoptReg` doesn't cause a spill, as everything has already been spilled beforehand. */
  adoptReg("$v0", true, result);
  // $v0 has already been adopted, so no need to restore
  fake_flush_restore('l', "$v0");
  CHECK_FREE(0, result); // note: remember to free

  // recover space allocated for arguments
  if (counter.arg > 4)
    adjustPtr(SHRINK, (counter.arg - 4) * ELEM_SIZE);
  counter.arg = 0; // reset arg counter
}

static void printREAD(const Code *code) {
  assert(code->kind == C_READ);
  const Operand *receiver = &code->as.unary;

  /* same with `printInvoke` */
  AddrDescriptor *ad = getAddrDescriptor(receiver);
  if (!is_addr_descriptor_valid(ad)) {
    init_addr_descriptor(ad);
  }
  // note: read function promises only change $v0
  const RegType courier = "$v0";
  if (!is_reg_empty(courier)) spillReg(courier);
  printUnary("jal", "read");
  adoptReg(courier, true, receiver);
  CHECK_FREE(0, receiver); // note: remember to free
}

static void printWRITE(const Code *code) {
  assert(code->kind == C_WRITE);
  const Operand *provider = &code->as.unary;
  const RegType courier = "$a0";

  if (provider->kind == O_CONSTANT) {
    if (!is_reg_empty(courier)) spillReg(courier);
    printLoadImm(courier, provider->value);
  } else {
    const RegType reg = ensureReg(provider, true);

    if (!is_reg_empty(courier)) spillReg(courier);
    if (strcmp(courier, reg) != 0) {
      printBinary("move", courier, reg);
      CHECK_FREE(0, provider);
    }
  }
  // note: write function promises to leave registers untouched
  printUnary("jal", "write");
}
#undef is_addr_descriptor_valid

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
    removeReg(index);
    distill_op(result);
    const RegType result_reg = ensureReg(result, false);
    print_save_load("sw", regsPool[index], 0, result_reg);
  }
  CHECK_FREE(0, result);
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
#define helper_(is_tmp_, reg_index_) (RegHelper){\
  .is_tmp = is_tmp_, .reg_index = reg_index_\
}
  const int kind = (*op)->kind;
  assert(in(kind, 1 + EFFECTIVE_OP, O_CONSTANT));
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
#undef find_in_regs

// the below two macros are related with RegHelper

/** reg_ gets the register name within registers' pool */
#define reg_(op) regsPool[op##_helper.reg_index]
/** free_reg_ checks whether a register needs to be freed.
 *    if is temporary register, then directly mark empty
 *    else use normal `CHECK_FREE` macro
 */
#define free_reg_(i, op) \
  if (op##_helper.is_tmp) {\
    removeReg(op##_helper.reg_index);\
  } else {\
    CHECK_FREE(i, op);\
  }

static void printASSIGN(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const Operand *right = &code->as.assign.right;
  const Operand *left = &code->as.assign.left;

  if (right->kind == O_INVOKE) return printInvoke(code);
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

  const RegType dec = ensureReg(op, false);
  adjustPtr(EXPAND, (int) code->as.dec.size);
  printBinary("move", dec, "$sp");
  CHECK_FREE(0, op);
}

static void print(const Code *code) {
#define elem(T) [C_##T] = print##T
  const FuncPtr dispatch[] = {
    elem(ASSIGN), elem(FUNCTION), elem(GOTO),
    elem(LABEL), elem(RETURN), elem(DEC),
    elem(IFGOTO), elem(ARG), elem(PARAM),
    elem(READ), elem(WRITE),
    [C_ADD] = printArithmatic, [C_SUB] = printArithmatic,
    [C_MUL] = printArithmatic, [C_DIV] = printArithmatic,
  };
  dispatch[code->kind](code);
#undef elem
}

static void initialize();
static void print_read_write();

void printMIPS(const char *file_name, const Block *blocks) {
  f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("can't open file: %s.\n", file_name);
    exit(EXIT_FAILURE);
  }

  // shuffleArray(regsPool, LEN, sizeof(RegType));
  initialize();

  // loop through each basic block
  for (int i = 0; i < blocks->cnt; ++i) {
    const BasicBlock *basic = blocks->container + i;
    // initialize variables and address descriptors
    variables = basic->variables;
    CNT = basic->cnt;
    addr_descriptors = malloc(sizeof(AddrDescriptor) * CNT);
    memset(addr_descriptors, -1, sizeof(AddrDescriptor) * CNT);
    /* note: the length of 'info' array doesn't match
     *  the number of chunks in basic blocks.
     * So it is necessary to spare a variable to keep track of it. */
    int info_index = 0;

    const Chunk *chunk = basic->begin;
    // loop through each line of code
    while (chunk != basic->end->next) {
      const Code *code = &chunk->code;
      const int kind = code->kind;
      if (kind == C_FUNCTION) fprintf(f, "\n");
#ifdef LOCAL
      fprintf(f, "# ");
      printCode(f, code);
#endif
      if (in(kind, EFFECTIVE_CODE)) {
        // only increment the 'info_index' if it is effective code.
        USE_INFO = (basic->info + info_index++)->use;
      }
      print(code);
      chunk = chunk->next;
    }
    free(addr_descriptors);
    assert(all_regs_empty());
  }
  print_read_write();
  fclose(f);
}

static void printUnary(const char *name, const RegType reg) {
  fprintf(f, "%*s%-4s %s\n", INDENT, "", name, reg);
}

static void printBinary(const char *name, const RegType reg1, const RegType reg2) {
  fprintf(f, "%*s%-4s %s, %s\n", INDENT, "", name, reg1, reg2);
}

static void printTernary(const char *name, const RegType result,
                         const RegType reg1, const RegType reg2) {
  fprintf(f, "%*s%-4s %s, %s, %s\n", INDENT, "", name, result, reg1, reg2);
}

static void print_read_write() {
  fprintf(f, "\nread:\n");
  printBinary("addi", "$sp", "$sp, -4");
  print_save_load("sw", "$a0", 0, "$sp");
  printLoadImm("$v0", 4);
  printBinary("la", "$a0", "_prompt");
  fprintf(f, "%*ssyscall\n",INDENT, "");
  printLoadImm("$v0", 5);
  fprintf(f, "%*ssyscall\n",INDENT, "");
  print_save_load("lw", "$a0", 0, "$sp");
  printTernary("addi", "$sp", "$sp", "4");
  printUnary("jr", "$ra");

  fprintf(f, "\nwrite:\n");
  printTernary("addi", "$sp", "$sp", "-8");
  print_save_load("sw", "$v0", 4, "$sp");
  print_save_load("sw", "$a0", 0, "$sp");
  printLoadImm("$v0", 1);
  fprintf(f, "%*ssyscall\n",INDENT, "");
  printLoadImm("$v0", 4);
  printBinary("la", "$a0", "_ret");
  fprintf(f, "%*ssyscall\n",INDENT, "");
  // "move $v0, $0" don't care about return value at all
  print_save_load("lw", "$v0", 4, "$sp");
  print_save_load("lw", "$a0", 0, "$sp");
  printTernary("addi", "$sp", "$sp", "8");
  printUnary("jr", "$ra");
}

static void initialize() {
  fprintf(f, ".data\n");
  fprintf(f, "_prompt: .asciiz \"Enter an integer:\"\n");
  fprintf(f, "_ret: .asciiz \"\\n\"\n");
  fprintf(f, ".globl __start\n");
  fprintf(f, ".text\n\n");

  fprintf(f, "__start:\n");
  printUnary("jal", "main");
  printBinary("li", "$v0", "10");
  fprintf(f, "%*ssyscall\n",INDENT, "");

  fprintf(f, "# END initialization\n");
}

// todo move this else where
#undef CHECK_FREE
#undef ELEM_SIZE
#undef EXPAND
#undef SHRINK

#undef PRINT_INFO
#undef BACK_FILL
