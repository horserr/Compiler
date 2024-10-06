#ifndef AST
#define AST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of ASTNode
typedef struct ASTNode ASTNode;
typedef union ValueUnion ValueUnion;

// AST Tree functions
ASTNode *createASTNode(const char *name, int lineNum, int flag);

ASTNode *createASTNodeWithValue(const char *name, int lineNum, ValueUnion value);

void addChild(ASTNode *parent, ASTNode *child);

void freeASTNode(ASTNode *node);

void printAST(const ASTNode *root);

void printASTRoot();

#endif // AST
