#ifndef IR__H
#define IR__H

typedef struct Operand {
  enum {
    TEM_VAR, TEM_ADDR,
    VARIABLE, ADDRESS,
    CONSTANT,
    LABEL, FUNCTION
  } kind;

  union {
    int var_no;              // TMP_VAR
    const char *value_s;     // VARIABLE, CONSTANT, FUNCTION
    struct Operand *address; // TMP_ADDR, ADDRESS
  };
} Operand;

// a single line of code
typedef struct {
  enum {
    READ, WRITE,
    ASSIGN,
    ADD, SUB, MUL, DIV,
  } kind;

  union {
    Operand unary;

    struct {
      Operand left, right;
    } assign;

    struct {
      Operand result, op1, op2;
    } binary;
  } as;
} Code;

// a doubly linked list of codes
typedef struct Chunk {
  Code code;
  struct Chunk *prev, *next;
} Chunk;


void initChunk(Chunk **sentinel);
void addCode(Chunk *sentinel, Code code);
void printChunk(const char *file_name, const Chunk *sentinel);
void freeOp(const Operand *op);
void freeChunk(const Chunk *sentinel);

#endif
