#include <stdio.h>
#include <stdlib.h>
#ifdef LOCAL
#include <ParseTree.h>
#include <SymbolTable.h>
#else
#include "ParseTree.h"
#include "SymbolTable.h"
#endif

#ifdef PARSER_DEBUG
extern int yydebug;
#endif

// Declare the external variables and functions
extern int yylineno;
extern void yyrestart(FILE *input_file);
extern int yyparse();
extern ParseTNode *root;

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
    if (argc == 1) {
        parse();
        return 0;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    rewind(f);
    const int rst = parse();
    if (rst == 0) {
#ifdef LOCAL
        // Redirect stdout to the output file
        if (freopen("test/out/out.txt", "w", stdout) == NULL) {
            DEBUG_INFO("Something wrong while redirect stdout.\n");
            exit(EXIT_FAILURE);
        }
        printParseTRoot();
        // revert stdout to terminal
        freopen("/dev/tty", "w",stdout);
#endif
        // printf("hello console.\n");
        buildTable(root);

        cleanParseTree();
    }

    return 0;
}
