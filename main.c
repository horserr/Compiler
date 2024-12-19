#ifdef LOCAL
#include <Compile.h>
#include <IR.h>
#include <Morph.h>
#include <Optimize.h>
#include <ParseTree.h>
#include <SymbolTable.h>
#else
#include "ParseTree.h"
#include "SymbolTable.h"
#include "Compile.h"
#include "IR.h"
#include "Optimize.h"
#include "Morph.h"
#endif

#ifdef PARSER_DEBUG
extern int yydebug;
#endif

// Declare the external variables and functions
extern int yylineno;
extern void yyrestart(FILE *input_file);
extern int yyparse();

void rewind(FILE *f) {
  if (!f) {
    DEBUG_INFO("File can't open.\n");
    return;
  }
  yylineno = 1;
  yyrestart(f);
}

int parse() {
#ifdef PARSER_DEBUG
    yydebug = 1;
#endif
  return yyparse();
}

int main(const int argc, char **argv) {
  if (argc != 3) {
    DEBUG_INFO("Arguments should be input test file and **output** file.\n");
    return 0;
  }
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  rewind(f);
  const int rst = parse();
  if (rst != 0) return 0;
  const ParseTNode *root = getRoot();

#ifdef LOCAL
  // Redirect stdout to the output file
  if (freopen("test/out/out.txt", "w", stdout) == NULL) {
    DEBUG_INFO("Something wrong while redirecting stdout.\n");
    exit(EXIT_FAILURE);
  }
  printParseTRoot();
  // redirect stdout to terminal
  freopen("/dev/tty", "w", stdout);
#endif

  const SymbolTable *table = buildTable(root);
  const Chunk *chunk = compile(root, table);
  optimize(chunk);

#ifdef LOCAL
  printChunk("test/out/out.ir", chunk);
#endif

  printMIPS(argv[2], chunk);

  // todo change free chunk to static function
  freeChunk(chunk);
  freeTable((SymbolTable *) table);
  cleanParseTree();
  return 0;
}
