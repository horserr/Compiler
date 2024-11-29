#ifndef PARSE_TREE
#define PARSE_TREE

// get the expression index
#define EXPRESSION_INDEX(node, expressions) \
  matchExprPattern(node, expressions, ARRAY_LEN(expressions))

#define getChild(node, index) \
  node->children.container[index]

// extract string value from node of ID, no copy
#define getStrFrom(node, where) \
  getChildByName(node, #where)->value.str_value

// extract int value from node of INT, no copy
#define getValFromINT(node) \
  getChildByName(node, "INT")->value.int_value

// extract float value from node of FLOAT, no copy
#define getValFromFLOAT(node) \
  getChildByName(node, "FLOAT")->value.float_value

// Forward declaration of ParseTNode
typedef struct ParseTNode ParseTNode;

typedef struct {
  int capacity, num;
  ParseTNode **container;
} ArrayList;

typedef union {
  char *str_value; // id
  int int_value;
  float float_value;
} ValueUnion;

struct ParseTNode {
  int flag; // 1: is token; 0: is higher expression
  char *name;
  int lineNum;
  ArrayList children;
  ValueUnion value;
};

// PARSE_TREE Tree functions
const ParseTNode* getRoot();
ParseTNode* createParseTNode(const char *name, int lineNum, int flag);
ParseTNode* createParseTNodeWithValue(const char *name, int lineNum, ValueUnion value);
void addChild(ParseTNode *parent, ParseTNode *child);
void freeParseTNode(ParseTNode *node);
void printParseTree(const ParseTNode *root);
void printParseTRoot();
void cleanParseTree();

// utility function
int nodeChildrenNamesEqual(const ParseTNode *node, const char *text);
ParseTNode* getChildByName(const ParseTNode *parent, const char *name);
int matchExprPattern(const ParseTNode *node, const char *expressions[], const int length);

#endif // PARSE_TREE
