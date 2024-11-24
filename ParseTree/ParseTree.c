#include "ParseTree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LOCAL
#include <utils.h>
#else
#include "utils.h"
#endif

// #define THRESHOLD 0.75
#define THRESHOLD 1
// the definition of root.
ParseTNode *root = NULL;

// Array List functions
static int initArrayList(ArrayList *list) {
  list->capacity = 4;
  list->num = 0;
  list->container = (ParseTNode **) malloc(list->capacity * sizeof(ParseTNode *));
  if (list->container == NULL) {
    DEBUG_INFO("fail to initialize array list.\n");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static int resizeArrayList(ArrayList *list) {
  list->capacity *= 2;
  list->container = (ParseTNode **) realloc(list->container, list->capacity * sizeof(ParseTNode *));
  if (list->container == NULL) {
    DEBUG_INFO("fail to resize array list.\n");
    exit(EXIT_FAILURE);
  }
  return 0;
}

static void addToArrayList(ArrayList *list, ParseTNode *node) {
  if (list == NULL) {
    DEBUG_INFO("list is NULL.\n");
    exit(EXIT_FAILURE);
  }
  if (list->num >= list->capacity * THRESHOLD) {
    resizeArrayList(list);
  }
  list->container[list->num++] = node;
}

static void freeArrayList(const ArrayList *list) {
  if (list == NULL) {
    return;
  }
  for (int i = 0; i < list->num; ++i) {
    freeParseTNode(list->container[i]);
  }
  free(list->container);
}

// ParseTree Tree functions
// Create a new ParseTree node without a value
ParseTNode* createParseTNode(const char *name, const int lineNum, const int flag) {
  if (flag != 0 && flag != 1) {
    DEBUG_INFO("Wrong flag value.\n");
    exit(EXIT_FAILURE);
  }
  ParseTNode *node = malloc(sizeof(ParseTNode));
  if (node == NULL) {
    DEBUG_INFO("fail to allocate memory for ParseTree node.\n");
    exit(EXIT_FAILURE);
  }
  node->flag = flag;
  node->name = my_strdup(name);
  if (node->name == NULL) {
    DEBUG_INFO("Fail to duplicate string for ParseTree node name.\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->lineNum = lineNum;
  initArrayList(&node->children);
  return node;
}

// Function to copy ValueUnion
static void copyValueUnion(ValueUnion *dest, const ValueUnion *src, const char *type) {
  if (strcmp(type, "id") == 0) {
    dest->str_value = my_strdup(src->str_value);
  } else if (strcmp(type, "int") == 0) {
    dest->int_value = src->int_value;
  } else if (strcmp(type, "float") == 0) {
    dest->float_value = src->float_value;
  } else {
    fprintf(stderr, "unknown type %s.\n", type);
    DEBUG_INFO("");
    exit(EXIT_FAILURE);
  }
}

// Create a new ParseTree node with a value
ParseTNode* createParseTNodeWithValue(const char *name, const int lineNum, const ValueUnion value) {
  ParseTNode *node = createParseTNode(name, lineNum, 1);
  if (strcmp(name, "INT") == 0) {
    copyValueUnion(&node->value, &value, "int");
  } else if (strcmp(name, "FLOAT") == 0) {
    copyValueUnion(&node->value, &value, "float");
  } else if (strcmp(name, "ID") == 0 || strcmp(name, "TYPE") == 0) {
    copyValueUnion(&node->value, &value, "id");
  }
  return node;
}

// Add a child node to a parent node
void addChild(ParseTNode *parent, ParseTNode *child) {
  if (child == NULL) {
    DEBUG_INFO("Child is a null pointer.\n");
    exit(EXIT_FAILURE);
  }
  addToArrayList(&parent->children, child);
}

// Free an ParseTree node
void freeParseTNode(ParseTNode *node) {
  if (node == NULL) {
    return;
  }
  freeArrayList(&node->children);
  if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0) {
    free(node->value.str_value);
  }
  free(node->name);
  free(node);
}

// printParseTree helper function
static void print(const ParseTNode *root, const int level) {
  if (root->flag) { // token
    printf("%*s%s", level * 2, "", root->name);
    if (strcmp(root->name, "INT") == 0) {
      printf(": %d", root->value.int_value);
    } else if (strcmp(root->name, "FLOAT") == 0) {
      printf(": %f", root->value.float_value);
    } else if (strcmp(root->name, "TYPE") == 0 || strcmp(root->name, "ID") == 0) {
      printf(": %s", root->value.str_value);
    }
    printf("\n");
    return;
  }
  // higher expression
  if (root->children.num == 0) {
    // yield epsilon (empty expression)
    return;
  }

  printf("%*s%s (%d)\n", level * 2, "", root->name, root->lineNum);
  for (int i = 0; i < root->children.num; i++) {
    print(root->children.container[i], level + 1);
  }
}

// print out entire ParseTree tree
void printParseTree(const ParseTNode *root) {
  if (root == NULL) {
    DEBUG_INFO("Root is null.\n");
    exit(EXIT_FAILURE);
  }
  print(root, 0);
}

void printParseTRoot() {
  printParseTree(root);
}

void cleanParseTree() {
  freeParseTNode(root);
}

/**
 * @brief compare expression's children name with the given text
 * @param node the parse tree node to be compared
 * @param text the name or expression string as comparison target
 * @return 1 if equal; 0 not equal
*/
int nodeChildrenNameEqualHelper(const ParseTNode *node, const char *text) {
  assert(node != NULL);
  // compare with children names
  char *text_copy = my_strdup(text);
  const char *delim = " ";
  const char *t = strtok(text_copy, delim);
  int i = 0;
  while (t != NULL) {
    if (i >= node->children.num) {
      // text_copy is longer than node's children names
      free(text_copy);
      return 0;
    }
    const char *child_name = node->children.container[i]->name;
    if (strcmp(child_name, t) != 0) {
      // mismatch word
      free(text_copy);
      return 0;
    }
    t = strtok(NULL, delim);
    i++;
  }
  free(text_copy);
  // check whether text is shorter than node's children names or not
  return i == node->children.num;
}

/**
 *  @brief get the child node by its name
 *  @return child node if found; else error occurred
*/
ParseTNode* getChildByName(const ParseTNode *parent, const char *name) {
  assert(parent != NULL);
  for (int i = 0; i < parent->children.num; ++i) {
    ParseTNode *child = parent->children.container[i];
    if (strcmp(child->name, name) == 0) {
      return child;
    }
  }
  fprintf(stderr, "Wrong child name: %s. The parent node is %s\n."
          " Checking from {ParseTree.c getChildByName}.\n",
          name, parent->name);
  exit(EXIT_FAILURE);
}

/**
 * @brief find which kind of expression the node's children match
 * @return if found, return the index of expr fitted in expressions; else error occurred
*/
int matchExprPattern(const ParseTNode *node, const char *expressions[], const int length) {
  int i = 0;
  while (i < length) {
    if (nodeChildrenNameEqualHelper(node, expressions[i])) {
      break;
    }
    i++;
  }
  if (i == length) {
    DEBUG_INFO("There must be a typo in expression list.\n");
    fprintf(stderr, "node: %s\n", node->name);
    for (int j = 0; j < length; ++j) {
      fprintf(stderr, "<%02d.>\t%s\n", j, expressions[j]);
    }
    exit(EXIT_FAILURE);
  }
  return i;
}

const ParseTNode* getRoot() {
  return root;
}

//////test code/////////////////////////////////////////////
#ifdef PARSE_TREE_test
int main(int argc, char const* argv[]) {
}
#endif

#undef THRESHOLD
