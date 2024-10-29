#ifndef PARSE_TREE
#define PARSE_TREE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of ASTNode
typedef struct ASTNode ParseTNode;

typedef struct {
    int capacity, num;
    ParseTNode **container; // meant to be a list of pointer
} ArrayList;

typedef union {
    char *identifier; // id
    int int_value;
    float float_value;
} ValueUnion;

struct ASTNode {
    int flag; // 1: is token; 0: is higher expression
    char *name;
    int lineNum;
    ArrayList children;
    ValueUnion value;
};

// PARSE_TREE Tree functions
ParseTNode *createParseTNode(const char *name, int lineNum, int flag);

ParseTNode *createParseTNodeWithValue(const char *name, int lineNum, ValueUnion value);

void addChild(ParseTNode *parent, ParseTNode *child);

void freeParseTNode(ParseTNode *node);

void printParseTree(const ParseTNode *root);

void printParseTRoot();

void cleanParseTree();

#endif // PARSE_TREE
