#include <stdio.h>
#include "ParseTree/ParseTree.h"

#ifdef PARSER_DEBUG
extern int yydebug;
#endif

// Declare the external variables and functions
extern int yylineno;
extern void yyrestart(FILE* input_file);
extern int yyparse();

void rewind(FILE* f) {
    if (!f) {
        perror("file can't open {main.c rewind}.");
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

int main(const int argc, char** argv) {
    if (argc == 1) {
        parse();
        return 0;
    }
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    rewind(f);
    const int rst = parse();
    if (rst == 0) {
        // print and free
        printParseTRoot();
        cleanParseTree();
    }

    return 0;
}
