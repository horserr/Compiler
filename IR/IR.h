#ifndef IR__H
#define IR__H

#ifdef LOCAL
#include <utils.h>
#else
#include "utils.h"
#endif

typedef struct Operand {
  enum {
    O_TEM_VAR, O_LABEL,
    O_CONSTANT,
    O_VARIABLE, O_INVOKE,
    O_DEREF, O_REFER,
  } kind;

  union {
    int var_no;              // TMP_VAR, LABEL
    int value;               // CONSTANT
    const char *value_s;     // VARIABLE, INVOKE
    struct Operand *address; // DEREF(dereference), REFER(reference)
  };
} Operand;

// a single line of code
typedef struct {
  enum {
    C_READ, C_WRITE, C_FUNCTION, C_PARAM, C_ARG, C_LABEL, C_RETURN, C_GOTO,
    C_ASSIGN,
    C_ADD, C_SUB, C_MUL, C_DIV,
    C_IFGOTO,
    C_DEC,
  } kind;

  union {
    Operand unary;

    struct {
      Operand left, right;
    } assign;

    struct {
      Operand result, op1, op2;
    } binary;

    struct {
      Operand x;
      Operand label;
      Operand y;
      char *relation;
    } ternary;

    struct {
      Operand target;
      unsigned size;
    } dec;
  } as;
} Code;

// a doubly linked list of codes
typedef struct Chunk {
  Code code;
  struct Chunk *prev, *next;
} Chunk;

int cmp_operand(const void *o1, const void *o2);

void initChunk(Chunk **sentinel);
Operand copyOperand(const Operand *src);
void addCode(Chunk *sentinel, Code code);
void printChunk(const char *file_name, const Chunk *sentinel);
void cleanOp(const Operand *op);
void removeCode(Chunk *chunk);
void freeChunk(const Chunk *sentinel);

#endif
