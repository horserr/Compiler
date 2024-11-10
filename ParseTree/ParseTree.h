#ifndef PARSE_TREE
#define PARSE_TREE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of ParseTNode
typedef struct ParseTNode ParseTNode;

typedef struct {
    int capacity, num;
    ParseTNode** container;
} ArrayList;

typedef union {
    char* str_value; // id
    int int_value;
    float float_value;
} ValueUnion;

struct ParseTNode {
    int flag; // 1: is token; 0: is higher expression
    char* name;
    int lineNum;
    ArrayList children;
    ValueUnion value;
};

// PARSE_TREE Tree functions
ParseTNode* createParseTNode(const char* name, int lineNum, int flag);
ParseTNode* createParseTNodeWithValue(const char* name, int lineNum, ValueUnion value);
void addChild(ParseTNode* parent, ParseTNode* child);
void freeParseTNode(ParseTNode* node);
void printParseTree(const ParseTNode* root);
void printParseTRoot();
void cleanParseTree();

// utility function
int nodeChildrenNameEqualHelper(const ParseTNode* node, const char* text);
ParseTNode* getChildByName(const ParseTNode* parent, const char* name);
int matchExprPattern(const ParseTNode* node, const char* expressions[], const int length);

#endif // PARSE_TREE
