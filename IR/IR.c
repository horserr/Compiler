#include "IR.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>

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
    case VARIABLE:
      fprintf(f, "%s ", op->value_s);
      break;
    case CONSTANT:
      fprintf(f, "#%s ", op->value_s);
      break;
    case TEM_VAR:
      fprintf(f, "t%d ", op->var_no);
      break;
    default:
      break;
  }
}

static void printCode(FILE *f, const Code *c) {
  assert(c != NULL);
  switch (c->kind) {
    case ASSIGN:
      printOp(f, &c->as.assign.left);
      fprintf(f, ":= ");
      printOp(f, &c->as.assign.right);
      break;
    case READ:
      fprintf(f, "READ ");
      printOp(f, &c->as.unary);
      break;
    case WRITE:
      fprintf(f, "WRITE ");
      printOp(f, &c->as.unary);
      break;
    default:
      break;
  }
  fprintf(f, "\n");
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

// a list contains the number of operand for each kind of code
int a[] = {
  [ASSIGN] = 2, // two operand
  [ADD] = 3, [MUL] = 3, [SUB] = 3,
};

void freeOp(const Operand *op) {
  // todo free address op
  switch (op->kind) {
    case VARIABLE:
    case FUNCTION:
    case CONSTANT:
      free((char *) op->value_s);
      break;
    default:
      break;
  }
}

static void freeCode(const Code *code) {
  const Operand *op = (Operand *) &code->as;
  for (int i = 0; i < a[code->kind]; ++i) {
    freeOp(op + i);
  }
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
