#ifndef AST
#define AST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of ASTNode
typedef struct ASTNode ASTNode;

typedef struct {
    int capacity, num;
    ASTNode **container; // meant to be a list of pointer
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

// AST Tree functions
ASTNode *createASTNode(const char *name, int lineNum, int flag);

ASTNode *createASTNodeWithValue(const char *name, int lineNum, ValueUnion value);

void addChild(ASTNode *parent, ASTNode *child);

void freeASTNode(ASTNode *node);

void printAST(const ASTNode *root);

void printASTRoot();

void cleanAST();

#endif // AST
