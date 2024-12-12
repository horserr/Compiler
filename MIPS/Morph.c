#ifdef LOCAL
#include <IR.h>
#include <Morph.h>
#else
#include "IR"
#include "Morph.h"
#endif

void initialize(FILE *fp) {
  fprintf(fp, ".data\n");
  fprintf(fp, "_prompt: .asciiz \"Enter an integer:\"\n");
  fprintf(fp, "_ret: .asciiz \"\\n\"\n");
  fprintf(fp, "\n.globl main\n");
  fprintf(fp, "\n.text");
  // FUNCTION read
  fprintf(fp, "\nread:\n");
  fprintf(fp, "\tli $v0, 4\n");
  fprintf(fp, "\tla $a0, _prompt\n");
  fprintf(fp, "\tsyscall\n");
  fprintf(fp, "\tli $v0, 5\n");
  fprintf(fp, "\tsyscall\n");
  fprintf(fp, "\tjr $ra\n");
  // FUNCTION write
  fprintf(fp, "\nwrite:\n");
  fprintf(fp, "\tli $v0, 1\n");
  fprintf(fp, "\tsyscall\n");
  fprintf(fp, "\tli $v0, 4\n");
  fprintf(fp, "\tla $a0, _ret\n");
  fprintf(fp, "\tsyscall\n");
  fprintf(fp, "\tmove $v0, $0\n");
  fprintf(fp, "\tjr $ra\n\n");
}

void printMIPS(const char *file_name, const Chunk *sentinel) {
  FILE *f = fopen(file_name, "w");
  if (f == NULL) {
    DEBUG_INFO("Can't open file %s.\n", file_name);
    exit(EXIT_FAILURE);
  }
  initialize(f);
  // const Chunk *c = sentinel->next;
  // while (c != sentinel) {
  //   printCode(f, &c->code);
  //   c = c->next;
  // }
  fclose(f);
}
