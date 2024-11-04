#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ParseTree.h"

#define THRESHOLD 0.75

// the definition of root.
ParseTNode* root = NULL;

// Function to duplicate a string
char* my_strdup(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    // Allocate memory for the new string
    char* dest = (char*)malloc(strlen(src) + 1);
    if (dest == NULL) {
        perror("fail to allocate memory in {my_strdup}.\n");
        exit(EXIT_FAILURE);
    }

    // Copy the contents of the source string to the destination string
    strcpy(dest, src);
    return dest;
}

// Array List functions
static int initArrayList(ArrayList* list) {
    list->capacity = 4;
    list->num = 0;
    list->container = (ParseTNode**)malloc(list->capacity * sizeof(ParseTNode*));
    if (list->container == NULL) {
        perror("fail to initialize array list in {ParseTree.c initArrayList}.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static int resizeArrayList(ArrayList* list) {
    list->capacity *= 2;
    list->container = (ParseTNode**)realloc(list->container, list->capacity * sizeof(ParseTNode*));
    if (list->container == NULL) {
        perror("fail to resize array list in {ParseTree.c resizeArrayList}.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static void addToArrayList(ArrayList* list, ParseTNode* node) {
    if (list == NULL) {
        perror("list is NULL in {ParseTree.c addToArrayList}.\n");
        exit(EXIT_FAILURE);
    }
    if (list->num >= list->capacity * THRESHOLD) {
        resizeArrayList(list);
    }
    list->container[list->num++] = node;
}

static void freeArrayList(const ArrayList* list) {
    if (list == NULL) {
        return;
    }
    for (int i = 0; i < list->num; ++i) {
        freeParseTNode(list->container[i]);
    }
    free(list->container);
    // free(list);
}

// ParseTree Tree functions
// Create a new ParseTree node without a value
ParseTNode* createParseTNode(const char* name, const int lineNum, const int flag) {
    if (flag != 0 && flag != 1) {
        perror("Wrong flag value in {ParseTree.c}.\n");
        exit(EXIT_FAILURE);
    }
    ParseTNode* node = malloc(sizeof(ParseTNode));
    if (node == NULL) {
        perror("fail to allocate memory for ParseTree node in {ParseTree.c createParseTNode}.\n");
        exit(EXIT_FAILURE);
    }
    node->flag = flag;
    node->name = my_strdup(name);
    if (node->name == NULL) {
        perror("fail to duplicate string for ParseTree node name in {ParseTree.c createParseTNode}.\n");
        free(node);
        exit(EXIT_FAILURE);
    }
    node->lineNum = lineNum;
    initArrayList(&node->children);
    return node;
}

// Function to copy ValueUnion
static void copyValueUnion(ValueUnion* dest, const ValueUnion* src, const char* type) {
    if (strcmp(type, "id") == 0) {
        dest->str_value = my_strdup(src->str_value);
    }
    else if (strcmp(type, "int") == 0) {
        dest->int_value = src->int_value;
    }
    else if (strcmp(type, "float") == 0) {
        dest->float_value = src->float_value;
    }
    else {
        fprintf(stderr, "unknown type %s in {ParseTree.c copyValueUnion}\n", type);
        exit(EXIT_FAILURE);
    }
}

// Create a new ParseTree node with a value
ParseTNode* createParseTNodeWithValue(const char* name, const int lineNum, const ValueUnion value) {
    ParseTNode* node = createParseTNode(name, lineNum, 1);
    if (strcmp(name, "INT") == 0) {
        copyValueUnion(&node->value, &value, "int");
    }
    else if (strcmp(name, "FLOAT") == 0) {
        copyValueUnion(&node->value, &value, "float");
    }
    else if (strcmp(name, "ID") == 0 || strcmp(name, "TYPE") == 0) {
        copyValueUnion(&node->value, &value, "id");
    }
    return node;
}

// Add a child node to a parent node
void addChild(ParseTNode* parent, ParseTNode* child) {
    if (child == NULL) {
        perror("child is a null pointer in {ParseTree.c addChild}.\n");
        exit(EXIT_FAILURE);
    }
    addToArrayList(&parent->children, child);
}

// Free an ParseTree node
void freeParseTNode(ParseTNode* node) {
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
static void print(const ParseTNode* root, const int level) {
    if (root->flag) { // token
        printf("%*s%s", level * 2, "", root->name);
        if (strcmp(root->name, "INT") == 0) {
            printf(": %d", root->value.int_value);
        }
        else if (strcmp(root->name, "FLOAT") == 0) {
            printf(": %f", root->value.float_value);
        }
        else if (strcmp(root->name, "TYPE") == 0 || strcmp(root->name, "ID") == 0) {
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
void printParseTree(const ParseTNode* root) {
    if (root == NULL) {
        perror("root is null in {ParseTree.c printParseTree}.");
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

//////test code/////////////////////////////////////////////
#ifdef PARSE_TREE_test
int main(int argc, char const *argv[]) {
    // Test 1: Create a simple ParseTree with a root and two children
    ParseTNode *root = createParseTNode("root", 1, 0);
    ParseTNode *child1 = createParseTNode("child1", 2, 0);
    ParseTNode *child2 = createParseTNode("child2", 3, 0);
    addChild(root, child1);
    addChild(root, child2);

    ParseTNode *child3 = createParseTNode("SEMI", 4, 1);
    ParseTNode *child4 = createParseTNode("COMMA", 5, 1);
    addChild(root, child3);
    addChild(child1, child4);

    ValueUnion value = {.int_value = 3};
    ParseTNode *int_child = createParseTNodeWithValue("INT", 6, value);

    ValueUnion value1 = {.str_value = "hello, wo!"};
    ParseTNode *id_child = createParseTNodeWithValue("ID", 7, value1);

    ValueUnion value3 = {.float_value = 3.01};
    ParseTNode *float_child = createParseTNodeWithValue("FLOAT", 8, value3);

    ValueUnion value4 = {.str_value = "int"};
    ParseTNode *type_child = createParseTNodeWithValue("TYPE", 9, value4);

    addChild(child4, int_child);
    addChild(child4, id_child);
    addChild(child2, float_child);
    addChild(child2, type_child);

    printf("Test 1: Simple Parse Tree\n");
    printParseTree(root);
    freeParseTNode(root);

    return 0;
}
#endif
