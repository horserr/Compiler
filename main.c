#include <stdio.h>

// Declare the external functions
extern int yylex(void);
extern void yyrestart(FILE *input_file);

extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc < 2) {
        yylex();
        return 0;
    }
    if (argc > 2) {
        perror("Too many arguments. Suppose to be one.");
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f); // yyin = f;
    yylex();
    fclose(f);

    return 0;
}
