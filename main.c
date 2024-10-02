#include <stdio.h>

// Declare the external variables and functions
extern int yylineno;
extern void yyrestart(FILE *input_file);
extern void yyparse();

void rewind(FILE *f) {
    if (!f) {
        perror("file can't open {main.c rewind}.");
        return;
    }
    yylineno = 1;
    yyrestart(f);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        yyparse();
        return 0;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    rewind(f);
    yyparse();

    return 0;
}
