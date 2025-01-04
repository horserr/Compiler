#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#define PRINT_INFO(op, message...) \
  long _back_fill_pos = -1;\
  do {\
    fprintf(f, "%*s#++ '%-20s' ", INDENT, "", __FUNCTION__);\
    if (op) {\
      fprintf(f," (");\
      printOp(f, op);\
      fseek(f, -1, SEEK_CUR);\
      fprintf(f,") <->    ");\
      _back_fill_pos = ftell(f) - 3;\
      }\
    fprintf(f, message);\
    fprintf(f, "\n");\
  } while(false)

#define BACK_FILL(reg) do {\
    assert(_back_fill_pos != -1);\
    fseek(f, _back_fill_pos, SEEK_SET);\
    fprintf(f, "%s", reg);\
    fseek(f, 0, SEEK_END);\
  } while(false)

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
static size_t CNT;

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

// fix: to shuffle the regsPool, need to remove `const`, else cause segmentation fault
static const RegType regsPool[] = {
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$v1", "$v0",
  "$a0", "$a1", "$a2", "$a3",
};
static const size_t LEN = ARRAY_LEN(regsPool);

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
   * anonymous array and spare the first element as sentinel */
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
  snprintf(buffer, sizeof(buffer), "%d(%s)", offset, base_reg);
  printBinary(T, reg, buffer);
}

static void print_spill_absorb(const char T, const AddrDescriptor *ad) {
  PRINT_INFO(NULL, "");
  assert(either(T, 's', 'l'));
  print_save_load(T == 's' ? "sw" : "lw",
                  regsPool[ad->reg_index],
                  ad->offset, "$fp");
}

// helper function for `ensureReg` and `printInvoke`
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

/**
 * This function simulates flushing and restoring registers without actually
 * modifying the register and address descriptors.
 *
 * It prints the MIPS code for spilling and absorbing
 * register values to/from memory.
 *
 * @param T The type of operation: 's' for spill, 'l' for load.
 * @param exception The register to be excluded from the operation.
 */
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
 * Removing a register from the deque.
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

/**
 * @brief Unlinks a register from both sides of the
 *  deque and updates the address descriptor.
 *
 * This function removes a register from the deque, ensuring that the links
 * of the previous and next pairs in the deque are updated correctly.
 * It also updates the address descriptor to indicate that the register is no longer
 * in use.
 *
 * @param reg_index The index of the register to be unlinked.
 */
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
 * @brief Finds the variable associated with the register, saves
 * its value to memory, and then relinquishes the register.
 * @param reg The register to be spilled.
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

/**
 * @brief Moves the ownership of a register to a new operand and links the register
 * to the receiver operand.
 *
 * This function is mainly used for passing arguments between function invocations.
 * It moves the ownership of a register to a new operand, ensuring that the register
 * is either empty or can be adopted by the receiver operand.
 * It then links the register to the receiver operand.
 *
 * @param reg The register to be adopted.
 * @param reg_must_empty A flag indicating whether the register must be empty.
 * @param receiver_op The operand that will receive the register.
 */
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
 * @brief Seizes a register for use, ensuring that it is available and not currently in use.
 *
 * This function finds an available register, spills its contents if necessary,
 * and marks it as in use by the specified operand.
 * It is used to allocate a register for temporary use or for storing a specific operand.
 *
 * @param avoidance The operand that guarantees to be avoided.
 * @return The index of the seized register.
 */
static int seizeReg(const RegType avoidance) {
#define find_in_pairs(pair_, begin, len) \
  findInArray(&pair_, false, deque.pairs + begin, len, sizeof(pair), cmp_pair)

  PRINT_INFO(NULL, "");
  int reg_index = 0;

  for (int i = 0; i < LEN - deque.size; ++i) {
    reg_index += find_in_pairs(EMPTY_PAIR, reg_index, LEN - reg_index);
    if (avoidance == NULL || strcmp(regsPool[reg_index], avoidance) != 0)
      goto RETURN;
  }
  // deque.size == LEN or happen to find avoidance register
  reg_index = pairs_sentinel.next_i;
  if (avoidance && strcmp(regsPool[reg_index], avoidance) == 0) // fix a bug
    reg_index = get_p(reg_index).next_i;
  spillReg(regsPool[reg_index]);

RETURN:
  addReg(reg_index);
  return reg_index;
#undef find_in_pairs
}
#undef is_pair_empty
#undef EMPTY_PAIR

/**
 * Ensures that a register is allocated for the given operand.
 *
 * This function checks if the operand already has a valid address descriptor.
 * If not, it initializes the address descriptor.
 * It then checks if the operand is already in a register.
 * If not, it seizes a new register for the operand.
 * It ensures that the operand is correctly linked to a register and that the
 * register is marked as in use.
 *
 * If the allocated register is the same as the
 * avoidance register, it spills the register and tries again.
 *
 * @param op The operand for which a register is to be ensured.
 * @param has_defined A flag indicating whether the operand has been defined.
 * @param avoidance The register to be avoided during allocation.
 * @return The register allocated for the operand.
 */
static RegType ensureReg(const Operand *op, const bool has_defined, const RegType avoidance) {
  PRINT_INFO(op, "");
  assert(op->kind != O_CONSTANT);
  AddrDescriptor *ad = getAddrDescriptor(op);

  if (!is_addr_descriptor_valid(ad)) {
    assert(!has_defined);
    init_addr_descriptor(ad);
  }
  // already saved on stack
SEARCH:
  if (!is_in_reg(ad)) {
    ad->reg_index = seizeReg(avoidance);
    get_p(ad->reg_index).op = op; // note: link register here.
    if (has_defined)
      print_spill_absorb('l', ad);
  }
  const RegType reg = regsPool[ad->reg_index];
  // spill the register if it is avoidance
  if (avoidance && strcmp(avoidance, reg) == 0) {
    spillReg(reg);
    goto SEARCH;
  }
  BACK_FILL(reg);
  return reg;
}
#undef get_p
#undef pairs_sentinel

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
#undef PRINT_INFO
#undef BACK_FILL

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

  if (counter.arg < 4) {
    const char courier[4];
    snprintf(courier, sizeof(courier), "$a%d", counter.arg);
    const RegType reg = ensureReg(provider, true, courier);
    if (!is_reg_empty(courier)) spillReg(courier);

    // can't use check free, because the register has unlinked
    if (strcmp(courier, reg) == 0) goto RETURN;
    printBinary("move", courier, reg);
  } else {
    const RegType reg = ensureReg(provider, true, NULL);
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
    const RegType reg = ensureReg(provider, true, courier);
    if (!is_reg_empty(courier)) spillReg(courier);
    printBinary("move", courier, reg);
    CHECK_FREE(0, provider);
  }

  // arg counter should be reset
  assert(counter.arg == 0);
  // all registers should be freed before return, including $v0.
  assert(all_regs_empty());
  print_save_load("lw", "$ra", -2 * ELEM_SIZE, "$fp");
  print_save_load("lw", "$fp", -ELEM_SIZE, "$fp");
  printUnary("jr", "$ra");
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

#undef ELEM_SIZE

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

#undef is_addr_descriptor_valid

/**
 * @brief Handles the left-hand side of an assignment or arithmetic operation.
 *
 * This function processes the left-hand side operand of an assignment or arithmetic operation.
 * For assignments, it directly moves the value from the right-hand side operand to the left-hand side.
 * For arithmetic operations, it performs the specified operation and stores the result in the left-hand side operand.
 *
 * @param kind The type of operation (C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV).
 * @param result The left-hand side operand.
 * @param ... The right-hand side operand(s) as RegType.
 * If kind is C_ASSIGN, one operand is expected; otherwise, two operands are expected.
 */
static void leftHelper(const int kind, const Operand *result, ...) {
  assert(in(kind, 5, C_ASSIGN, C_ADD, C_SUB, C_MUL, C_DIV));
  va_list regs;
  va_start(regs, kind == C_ASSIGN ? 1 : 2);

  if (kind == C_ASSIGN) {
    const RegType right_reg = va_arg(regs, RegType);
    if (result->kind == O_DEREF) {
      distill_op(result);
      const RegType result_reg = ensureReg(result, false, NULL);
      print_save_load("sw", right_reg, 0, result_reg);
    } else {
      const RegType result_reg = ensureReg(result, false, NULL);
      printBinary("move", result_reg, right_reg);
    }
    CHECK_FREE(0, result);
    return va_end(regs);
  }

  int index;
  if (result->kind == O_DEREF) {
    index = seizeReg(NULL);
  } else {
    const RegType result_reg = ensureReg(result, false, NULL);
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
    const RegType result_reg = ensureReg(result, false, NULL);
    print_save_load("sw", regsPool[index], 0, result_reg);
  }
  CHECK_FREE(0, result);
}

typedef struct {
  bool is_tmp;
  int reg_index;
} RegHelper;

/**
 * @brief Allocates a register for the right-hand side operand.
 *
 * This function allocates a register for the right-hand side operand of an assignment or arithmetic operation.
 * It handles different types of operands, including constants, dereferenced values, and variables.
 *
 * @param index The index of the operand in the operand array.
 * @param op The operand on the right-hand side.
 * @param avoidance The register to be avoided during allocation.
 * @return A struct containing information about whether the allocated register is temporary and its index.
 */
static RegHelper rightHelper(const int index, const Operand **op, const RegType avoidance) {
#define helper_(is_tmp_, reg_index_) (RegHelper){\
  .is_tmp = is_tmp_, .reg_index = reg_index_\
}
  const int kind = (*op)->kind;
  assert(in(kind, 1 + EFFECTIVE_OP, O_CONSTANT));
  int reg_index;
  if (kind == O_CONSTANT) {
    reg_index = seizeReg(avoidance);
    printLoadImm(regsPool[reg_index], (*op)->value);
    return helper_(true, reg_index);
  }
  if (kind == O_DEREF) {
    distill_op(*op);
    const RegType op1_reg = ensureReg(*op, true, avoidance);
    reg_index = seizeReg(avoidance);
    print_save_load("lw", regsPool[reg_index], 0, op1_reg);
    // fix a bug
    CHECK_FREE(index, *op); // this register may not need any more.
    return helper_(true, reg_index);
  }
  // reference or variable
  if (kind == O_REFER)
    distill_op(*op);
  const RegType reg = ensureReg(*op, true, avoidance);
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
    const RegType left_reg = ensureReg(left, false, NULL);
    printLoadImm(left_reg, right->value);
    CHECK_FREE(0, left);
    return;
  }

  const RegHelper right_helper = rightHelper(1, &right, NULL);
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

  const RegHelper op1_helper = rightHelper(1, &op1, NULL);
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
  // fix a bug, the second operand may also be reference or dereference type
  // assert(!either(op2->kind, O_REFER, O_DEREF));
  if (kind == C_ADD && either(O_CONSTANT, op1->kind, op2->kind))
    return printAddI(code);

  const RegHelper op1_helper = rightHelper(1, &op1, NULL);
  const RegHelper op2_helper = rightHelper(2, &op2, reg_(op1));
  free_reg_(1, op1);
  free_reg_(2, op2);

  const Operand *result = &code->as.binary.result;
  assert(result->kind != O_REFER);
  leftHelper(kind, result, reg_(op1), reg_(op2));
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
  const Operand *x = &code->as.ternary.x;
  const RegHelper x_helper = rightHelper(0, &x, NULL);
  const Operand *y = &code->as.ternary.y;
  const RegHelper y_helper = rightHelper(1, &y, reg_(x));

  free_reg_(0, x);
  free_reg_(1, y);

  char label[16];
  snprintf(label, sizeof(label), "label%d", code->as.ternary.label.var_no);
  printTernary(map[index].mnemonic, reg_(x), reg_(y), label);
}

static void printWRITE(const Code *code) {
  assert(code->kind == C_WRITE);
  const Operand *provider = &code->as.unary;
  const RegType courier = "$a0";

  if (provider->kind == O_CONSTANT) {
    if (!is_reg_empty(courier)) spillReg(courier);
    printLoadImm(courier, provider->value);
  } else {
    // fix a bug, provider can also be dereferenced type
    /* note: there exists a case that provider's register happens to be courier.
     *  This register will be spilled within `ensureReg` which will be called
     *  inside `rightHelper` . */
    const RegHelper provider_helper = rightHelper(0, &provider, courier);
    assert(strcmp(courier, reg_(provider)) != 0);
    if (!is_reg_empty(courier)) spillReg(courier);
    printBinary("move", courier, reg_(provider));
    free_reg_(0, provider);
  }
  // note: write function promises to leave registers untouched
  printUnary("jal", "write");
}
#undef free_reg_
#undef reg_

static void printLABEL(const Code *code) {
  assert(code->kind == C_LABEL);
  fprintf(f, "label%d:\n", code->as.unary.var_no);
}

static void printGOTO(const Code *code) {
  assert(code->kind == C_GOTO);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "label%d", code->as.unary.var_no);
  printUnary("j", buffer);
}

static void printDEC(const Code *code) {
  assert(code->kind == C_DEC);
  const Operand *op = &code->as.dec.target;

  const RegType dec = ensureReg(op, false, NULL);
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

#define begin_kind(basic) basic->begin->code.kind

// garner all variables within the current function block
static void initialize(const Block *block, int current) {
#define size(cnt) (sizeof(Operand *) * (cnt))
  assert(current < block->cnt);
  // only redirect `variables` pointer for new function scope
  const BasicBlock *basic = block->container + current;
  if (begin_kind(basic) != C_FUNCTION) return;

  assert(variables == NULL && addr_descriptors == NULL);
  CNT = basic->cnt;
  variables = malloc(size(CNT));
  memcpy(variables, basic->variables, size(CNT));

  while (++current < block->cnt) {
    const BasicBlock *b = block->container + current;
    if (begin_kind(b) == C_FUNCTION) break;

    variables = realloc(variables, size(CNT + b->cnt));
    memcpy(variables + CNT, b->variables, size(b->cnt));
    CNT += b->cnt;

    qsort(variables, CNT, sizeof(Operand *), cmp_operand);
    removeDuplicates(variables, &CNT, sizeof(Operand *), cmp_operand);
  }

  // initialize address descriptor
  addr_descriptors = malloc(sizeof(AddrDescriptor) * CNT);
  memset(addr_descriptors, -1, sizeof(AddrDescriptor) * CNT);
#undef size
}

static void finalize(const Block *block, const int current) {
  // after each basic block, all registers are empty.
  assert(all_regs_empty());
  assert(current < block->cnt);
  if (current != block->cnt - 1) {
    const BasicBlock *next_basic = block->container + current + 1;
    if (begin_kind(next_basic) != C_FUNCTION) return;
  }

  assert(variables != NULL && addr_descriptors != NULL);
  assert(all_regs_empty());
  // all variables must have space on stack
  assert(all_addr_descriptor_valid());
#ifdef LOCAL
  fprintf(f, "# -- FINALIZE\n");
#endif

  free(variables);
  variables = NULL;
  free(addr_descriptors);
  addr_descriptors = NULL;

  adjustPtr(SHRINK, -frameOffset);
  assert(frameOffset == 0);
}
#undef begin_kind

static void init_MIPS();
static void print_read_write();

void printMIPS(const char *file_name, const Block *blocks) {
  f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("can't open file: %s.\n", file_name);
    exit(EXIT_FAILURE);
  }

  // note: remember to remove `const` keyword for `regsPool` if uncomment shuffle
  // shuffleArray(regsPool, LEN, sizeof(RegType));
  init_MIPS();

  // loop through each basic block
  for (int i = 0; i < blocks->cnt; ++i) {
    const BasicBlock *basic = blocks->container + i;
    // initialize variables and address descriptors
    initialize(blocks, i);
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
    finalize(blocks, i);
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

static void init_MIPS() {
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
#undef CHECK_FREE
#undef EXPAND
#undef SHRINK
