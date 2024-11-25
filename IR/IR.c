#include "IR.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>
#include <bits/local_lim.h>

void initChunk(Chunk **sentinel) {
  assert(*sentinel == NULL); // sentinel should be NULL at first
  *sentinel = malloc(sizeof(Chunk));
  (*sentinel)->next = (*sentinel)->prev = *sentinel;
}

void addCode(Chunk *sentinel, const Code code) {
  Chunk *chunk = malloc(sizeof(Chunk));
  chunk->code = code;
  chunk->next = sentinel;
  chunk->prev = sentinel->prev;
  sentinel->prev->next = chunk;
  sentinel->prev = chunk;
}

static void printOp(FILE *f, const Operand *op) {
  switch (op->kind) {
    case O_FUNCTION:
    case O_VARIABLE:
      fprintf(f, "%s ", op->value_s);
      break;
    case O_CONSTANT:
      fprintf(f, "#%s ", op->value_s);
      break;
    case O_TEM_VAR:
      fprintf(f, "t%d ", op->var_no);
      break;
    case O_LABEL:
      fprintf(f, "label%d ", op->var_no);
      break;
    default:
      break;
  }
}

static void printCode(FILE *f, const Code *c) {
  assert(c != NULL);
  enum binary_op { ADD, SUB, MUL, DIV };
  const char b[] = {
    [ADD] = '+', [SUB] = '-',
    [MUL] = '*', [DIV] = '/',
  };
#define UNARY(T) \
  do {\
    fprintf(f, #T " ");\
    printOp(f, &c->as.unary);\
  } while(false)

#define BINARY_CASE(T) \
  case C_##T:\
    do {\
      printOp(f, &c->as.binary.result);\
      fprintf(f, ":= ");\
      printOp(f, &c->as.binary.op1);\
      fprintf(f, "%c ",b[T]);\
      printOp(f, &c->as.binary.op2);\
    } while(false);\
    break

  switch (c->kind) {
    BINARY_CASE(ADD);
    BINARY_CASE(SUB);
    BINARY_CASE(MUL);
    BINARY_CASE(DIV);
    case C_READ:
      UNARY(READ);
      break;
    case C_WRITE:
      UNARY(WRITE);
      break;
    case C_FUNCTION:
      UNARY(FUNCTION);
      fprintf(f, ": ");
      break;
    case C_PARAM:
      UNARY(PARAM);
      break;
    case C_LABEL:
      UNARY(LABEL);
      fprintf(f, ": ");
      break;
    case C_ASSIGN:
      printOp(f, &c->as.assign.left);
      fprintf(f, ":= ");
      printOp(f, &c->as.assign.right);
      break;
    case C_IFGOTO:
      fprintf(f, "IF ");
      printOp(f, &c->as.ternary.x);
      fprintf(f, "%s ", c->as.ternary.relation);
      printOp(f, &c->as.ternary.y);
      fprintf(f, "GOTO ");
      printOp(f, &c->as.ternary.label);
      break;
    default:
      break;
  }
  fprintf(f, "\n");

#undef UNARY
#undef BINARY_CASE
}

void printChunk(const char *file_name, const Chunk *sentinel) {
  FILE *f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("Can't open file.\n");
    exit(EXIT_FAILURE);
  }
  const Chunk *c = sentinel->next;
  while (c != sentinel) {
    printCode(f, &c->code);
    c = c->next;
  }
  fclose(f);
}

void freeOp(const Operand *op) {
  // todo free address op
  switch (op->kind) {
    case O_VARIABLE:
    case O_FUNCTION:
    case O_CONSTANT:
      free((char *) op->value_s);
      break;
    default:
      break;
  }
}

// a list contains the number of operand for each kind of code
int a[] = {
  [C_READ] = 1, [C_WRITE] = 1, [C_FUNCTION] = 1, [C_PARAM] = 1, [C_LABEL] = 1,
  [C_ASSIGN] = 2,
  [C_ADD] = 3, [C_MUL] = 3, [C_SUB] = 3, [C_DIV] = 3, [C_IFGOTO] = 3,
};

static void freeCode(const Code *code) {
  const Operand *op = (Operand *) &code->as;
  for (int i = 0; i < a[code->kind]; ++i) {
    freeOp(op + i);
  }
  if (code->kind == C_IFGOTO)
    free(code->as.ternary.relation);
}

void freeChunk(const Chunk *sentinel) {
  assert(sentinel != NULL);
  Chunk *c = sentinel->next;
  while (c != sentinel) {
    Chunk *tmp = c;
    c = c->next;
    freeCode(&tmp->code);
    free(tmp);
  }
  free(c);
}
