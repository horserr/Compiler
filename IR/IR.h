#ifndef IR__H
#define IR__H

typedef struct Operand {
  enum {
    O_TEM_VAR, O_TEM_ADDR, O_VARIABLE, O_ADDRESS,
    O_CONSTANT, O_LABEL, O_FUNCTION
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
    C_READ, C_WRITE, C_FUNCTION, C_PARAM, C_ARG,
    C_ASSIGN,
    C_ADD, C_SUB, C_MUL, C_DIV,
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
