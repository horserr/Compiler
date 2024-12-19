#ifdef LOCAL
#include <IR.h>
#else
#include "IR.h"
#endif

// a list contains the number of operands for each kind of code
const u_int8_t operand_count_per_code[] = {
  [C_READ] = 1, [C_WRITE] = 1, [C_FUNCTION] = 1, [C_PARAM] = 1,
  [C_LABEL] = 1, [C_RETURN] = 1, [C_GOTO] = 1, [C_ARG] = 1,
  [C_ASSIGN] = 2,
  [C_ADD] = 3, [C_MUL] = 3, [C_SUB] = 3, [C_DIV] = 3, [C_IFGOTO] = 3,
  [C_DEC] = 1,
};

int cmp_operand(const void *o1, const void *o2) {
#define allow_num_types \
    2,\
    O_TEM_VAR, O_VARIABLE
  //the order of different operands
  const u_int8_t order[] = {
    [O_TEM_VAR] = 0, [O_VARIABLE] = 1,
  };

  const Operand *op1 = (Operand *) o1, *op2 = (Operand *) o2;
  // no constant
  assert(op1 != NULL && in(op1->kind, allow_num_types));
  assert(op2 != NULL && in(op2->kind, allow_num_types));
#undef allow_num_types
  const int kind1 = op1->kind, kind2 = op2->kind;

  if (kind1 != kind2) return order[kind1] - order[kind2];
  // todo also modify here
  if (kind1 == O_TEM_VAR) return op1->var_no - op2->var_no;
  if (kind1 == O_VARIABLE) return strcmp(op1->value_s, op2->value_s);
  assert(false);
}

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

// todo add comment and check every return and value
// deep copy an operand, especially those pointer value.
Operand copyOperand(const Operand *src) {
  Operand tmp = *src;
  if (in(src->kind, 2, O_VARIABLE, O_INVOKE)) {
    tmp.value_s = my_strdup(src->value_s);
  } else if (in(src->kind, 2, O_REFER, O_DEREF)) {
    Operand *addr_op = malloc(sizeof(Operand));
    *addr_op = copyOperand(src->address);
    tmp.address = addr_op;
  }
  return tmp;
}

static void printOp(FILE *f, const Operand *op) {
  switch (op->kind) {
    case O_VARIABLE:
      fprintf(f, "%s ", op->value_s);
      break;
    case O_CONSTANT:
      fprintf(f, "#%d ", op->value);
      break;
    case O_INVOKE:
      fprintf(f, "CALL %s ", op->value_s);
      break;
    case O_TEM_VAR:
      fprintf(f, "t%d ", op->var_no);
      break;
    case O_LABEL:
      fprintf(f, "label%d ", op->var_no);
      break;
    case O_REFER:
      fprintf(f, "&");
      printOp(f, op->address);
      break;
    case O_DEREF:
      fprintf(f, "*");
      printOp(f, op->address);
      break;
    default:
      break;
  }
}

void printCode(FILE *f, const Code *c) {
  assert(c != NULL);
  enum binary_op { ADD, SUB, MUL, DIV };
  static const char b[] = {
    [ADD] = '+', [SUB] = '-',
    [MUL] = '*', [DIV] = '/',
  };
#define UNARY(T) \
  do {\
    fprintf(f, #T " ");\
    printOp(f, &c->as.unary);\
  } while(false)

#define CASE_UNARY(T) \
  case C_##T:\
    UNARY(T);\
    break

#define CASE_BINARY(T) \
  case C_##T:\
    printOp(f, &c->as.binary.result);\
    fprintf(f, ":= ");\
    printOp(f, &c->as.binary.op1);\
    fprintf(f, "%c ",b[T]);\
    printOp(f, &c->as.binary.op2);\
    break

#define CASES(K, T) CASE_##K(T)

  switch (c->kind) {
    CASES(BINARY, ADD);
    CASES(BINARY, SUB);
    CASES(BINARY, MUL);
    CASES(BINARY, DIV);
    CASES(UNARY, GOTO);
    CASES(UNARY, RETURN);
    CASES(UNARY, READ);
    CASES(UNARY, WRITE);
    CASES(UNARY, PARAM);
    CASES(UNARY, ARG);
    case C_FUNCTION:
      UNARY(FUNCTION);
      fprintf(f, ": ");
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
    case C_DEC:
      fprintf(f, "DEC ");
      printOp(f, &c->as.dec.target);
      fprintf(f, "%u", c->as.dec.size);
      break;
    default:
      break;
  }
  fprintf(f, "\n");

#undef UNARY
#undef CASE_BINARY
#undef CASE_UNARY
#undef CASES
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

void cleanOp(const Operand *op) {
  if (in(op->kind, 2, O_VARIABLE, O_INVOKE))
    return free((char *) op->value_s);

  if (in(op->kind, 2, O_REFER, O_DEREF)) {
    cleanOp(op->address);
    free(op->address);
  }
}

static void cleanCode(const Code *code) {
  for (int i = 0; i < operand_count_per_code[code->kind]; ++i)
    cleanOp((Operand *) &code->as + i);

  if (code->kind == C_IFGOTO)
    free(code->as.ternary.relation);
}

void removeCode(Chunk *chunk) {
  // can't be the sentinel node
  assert(chunk->prev != chunk && chunk != chunk->next);
  Chunk *prev = chunk->prev;
  Chunk *next = chunk->next;
  prev->next = next;
  next->prev = prev;
  cleanCode(&chunk->code);
  free(chunk);
}

void freeChunk(const Chunk *sentinel) {
  assert(sentinel != NULL);
  Chunk *c = sentinel->next;
  while (c != sentinel) {
    c = c->next;
    removeCode(c->prev);
  }
  free(c);
}
