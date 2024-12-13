#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif

typedef void (*FuncPtr)(const Code *);

static void printUnary(const char *name, const char *reg);
static void printBinary(const char *name, const char *reg1, const char *reg2);
static void printTernary(const char *name, const char *result,
                         const char *reg1, const char *reg2);

FILE *f = NULL;

static const char* getReg(const Operand *op) {
  static const char *regsPool[] = {
    "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12", "m13",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$sp", "$fp", "$ra", "$zero",
  };
  if (op->kind == O_CONSTANT) {
    char *im = (char *) int2String(op->value);
    printBinary("li", "t0", im);
    free(im);
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

static void printASSIGN(const Code *code) {
  assert(code->kind == C_ASSIGN);
  const char *x = getReg(&code->as.assign.left);
  const char *y = getReg(&code->as.assign.right);
  printBinary("move", x, y);
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

  if (op1->kind == O_CONSTANT)
    swap(Operand*, op1, op2);
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
  const char *op1_reg = getReg(&code->as.binary.op1);
  const char *op2_reg = getReg(&code->as.binary.op2);
  printBinary("div", op1_reg, op2_reg);
  const char *result_reg = getReg(&code->as.binary.result);
  printUnary("mflo", result_reg);
}

static void printMUL(const Code *code) {
  assert(code->kind == C_MUL);
  const char *result_reg = getReg(&code->as.binary.result);
  const char *op1_reg = getReg(&code->as.binary.op1);
  const char *op2_reg = getReg(&code->as.binary.op2);
  printTernary("mul", result_reg, op1_reg, op2_reg);
}

static void printREAD(const Code *code) {
  assert(code->kind == C_READ);
  // todo modify this
  // terniary
  fprintf(f, "\t subu $sp, $sp, 4\n");
}

static void printCode(const Code *code) {
#define elem(T) [C_##T] = print##T
  static const FuncPtr printTable[] = {
    elem(ASSIGN), elem(ADD), elem(SUB), elem(MUL), elem(DIV),
    elem(FUNCTION),
  };
  // todo remove this
  if (!in(code->kind,
          5,
          // C_ASSIGN,
          C_ADD, C_SUB, C_MUL, C_DIV,
          C_FUNCTION))
    return;

  printTable[code->kind](code);
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

void printMIPS(const char *file_name, const Chunk *sentinel) {
  f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("Can't open file %s.\n", file_name);
    exit(EXIT_FAILURE);
  }
  // initialize();
  const Chunk *c = sentinel->next;
  while (c != sentinel) {
    printCode(&c->code);
    c = c->next;
  }
  fclose(f);
}

static void printUnary(const char *name, const char *reg) {
  fprintf(f, "\t%s %s\n", name, reg);
}

static void printBinary(const char *name, const char *reg1, const char *reg2) {
  fprintf(f, "\t%s %s, %s\n", name, reg1, reg2);
}

static void printTernary(const char *name, const char *result,
                         const char *reg1, const char *reg2) {
  fprintf(f, "\t%s %s, %s, %s\n", name, result, reg1, reg2);
}
