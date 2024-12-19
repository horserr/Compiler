#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif

typedef void (*FuncPtr)(const Code *);
typedef const char *RegType;

static void printUnary(const char *name, RegType reg);
static void printBinary(const char *name, RegType reg1, RegType reg2);
static void printTernary(const char *name, RegType result,
                         RegType reg1, RegType reg2);

FILE *f = NULL;

static void printLoadImm(const RegType reg, const int val) {
  char *im = (char *) int2String(val);
  printBinary("li", reg, im);
  free(im);
}

static void printSaveOrLoad(const char *T, const RegType reg,
                            const unsigned offset, const RegType base_reg) {
  assert(strcmp(T, "sw") == 0 || strcmp(T, "lw") == 0);
  char buffer[20];
  sprintf(buffer, "%u(%s)", offset, base_reg);
  printBinary(T, reg, buffer);
}

static RegType getReg(const Operand *op) {
  static const RegType regsPool[] = {
    "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$sp", "$fp", "$ra", "$zero",
  };
  if (op->kind == O_CONSTANT) {
    printLoadImm("t0", op->value);
    return regsPool[20];
  }
  // todo temporary try
  static int i = 1;
  return regsPool[i++];
}

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
  printBinary("move", "$v0", getReg(&code->as.unary));
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
  // todo bsearch
  // search in map
  const char *name = NULL;
  for (int i = 0; i < ARRAY_LEN(map); ++i) {
    if (strcmp(map[i].symbol, code->as.ternary.relation) == 0) {
      name = map[i].mnemonic;
      break;
    }
  }
  assert(name != NULL);
  char *label;
  asprintf(&label, "label%d", code->as.ternary.label.var_no);
  printTernary(name, getReg(&code->as.ternary.x),
               getReg(&code->as.ternary.y), label);
  free(label);
}

static void printASSIGN(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const Operand *right = &code->as.assign.right;
  assert(right->kind != O_REFER);
  if (right->kind == O_CONSTANT) {
    printLoadImm("t1", right->value);
    return;
  }

  const Operand *left = &code->as.assign.left;
  const RegType left_reg = getReg(left);
  const RegType right_reg = getReg(right);

  assert(!(left->kind == O_DEREF && right->kind == O_DEREF));
  if (left->kind == O_DEREF) {
    printSaveOrLoad("sw", left_reg, 0, right_reg);
    return;
  }
  if (right->kind == O_DEREF) {
    printSaveOrLoad("lw", left_reg, 0, right_reg);
    return;
  }
  printBinary("move", left_reg, right_reg);
}

static void printADD(const Code *code) {
  assert(code->kind == C_ADD);
  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  // can't be constant at the same time
  assert(op1->kind != O_CONSTANT || op2->kind != O_CONSTANT);
  const Operand *result = &code->as.binary.result;
  if (op1->kind != O_CONSTANT && op2->kind != O_CONSTANT)
    return printTernary("add", getReg(result),
                        getReg(op1), getReg(op2));

  if (op1->kind == O_CONSTANT) SWAP(&op1, &op2, sizeof(Operand));

  char *immediate = (char *) int2String(op2->value);
  printTernary("addi", getReg(result), getReg(op1), immediate);
  free(immediate);
}

static void printSUB(const Code *code) {
  assert(code->kind == C_SUB);
  const Operand *op1 = &code->as.binary.op1;
  const Operand *op2 = &code->as.binary.op2;
  const Operand *result = &code->as.binary.result;
  // the second operand can't be constant, as it has been changed to 'ADD'
  // with the aid of `organizeOpSequence`
  assert(op2->kind != O_CONSTANT);
  printTernary("sub", getReg(result), getReg(op1), getReg(op2));
}

static void printDIV(const Code *code) {
  assert(code->kind == C_DIV);
  const RegType op1_reg = getReg(&code->as.binary.op1);
  const RegType op2_reg = getReg(&code->as.binary.op2);
  printBinary("div", op1_reg, op2_reg);
  const RegType result_reg = getReg(&code->as.binary.result);
  printUnary("mflo", result_reg);
}

static void printMUL(const Code *code) {
  assert(code->kind == C_MUL);
  const RegType result_reg = getReg(&code->as.binary.result);
  const RegType op1_reg = getReg(&code->as.binary.op1);
  const RegType op2_reg = getReg(&code->as.binary.op2);
  printTernary("mul", result_reg, op1_reg, op2_reg);
}

static void printIR(const Code *code) {
#define elem(T) [C_##T] = print##T
  const FuncPtr printTable[] = {
    elem(ASSIGN), elem(ADD), elem(SUB), elem(MUL), elem(DIV),
    elem(FUNCTION), elem(GOTO), elem(LABEL), elem(RETURN),
    elem(IFGOTO),
  };
#undef elem
  // todo remove this
  if (!in(code->kind,
          6,
          C_ASSIGN,
          C_ADD, C_SUB, C_MUL, C_DIV,
          C_FUNCTION))
    return;

  printTable[code->kind](code);
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

void printMIPS(const char *file_name, const Chunk *sentinel) {
  f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("Can't open file %s.\n", file_name);
    exit(EXIT_FAILURE);
  }
  // initialize();
  const Chunk *c = sentinel->next;
  while (c != sentinel) {
    printIR(&c->code);
    c = c->next;
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
