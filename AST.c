#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AST.h"

#define THRESHOLD 0.75

// the definition of root.
ASTNode *root = NULL;

// Array List functions
static int initArrayList(ArrayList *list) {
    list->capacity = 4;
    list->num = 0;
    list->container = (ASTNode **) malloc(list->capacity * sizeof(ASTNode *));
    if (list->container == NULL) {
        perror("fail to init array list in {AST.c initArrayList}.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static int resizeArrayList(ArrayList *list) {
    list->capacity *= 2;
    list->container =
            (ASTNode **) realloc(list->container, list->capacity * sizeof(ASTNode *));
    if (list->container == NULL) {
        perror("fail to resize array list in {AST.c resizeArrayList}.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

static void addToArrayList(ArrayList *list, ASTNode *node) {
    if (list == NULL) {
        perror("node added is null in {AST.c addToArrayList}.\n");
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
        freeASTNode(list->container[i]);
    }
    free(list->container);
    // free(list);
}

// AST Tree functions
// Create a new AST node without a value
ASTNode *createASTNode(const char *name, const int lineNum, const int flag) {
    ASTNode *node = (ASTNode *) malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Failed to allocate memory for ASTNode\n");
        exit(EXIT_FAILURE);
    }
    node->flag = flag;
    node->name = strdup(name);
    if (node->name == NULL) {
        perror("failed to allocate memory for node name in {AST.c createASTNode).");
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
        dest->identifier = strdup(src->identifier);
    } else if (strcmp(type, "int") == 0) {
        dest->int_value = src->int_value;
    } else if (strcmp(type, "float") == 0) {
        dest->float_value = src->float_value;
    } else {
        fprintf(stderr, "unknown type %s in {AST.c copyValueUnion}\n", type);
        exit(EXIT_FAILURE);
    }
}

// Create a new AST node with a value
ASTNode *createASTNodeWithValue(const char *name, const int lineNum, const ValueUnion value) {
    ASTNode *node = createASTNode(name, lineNum, 1);
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
void addChild(ASTNode *parent, ASTNode *child) {
    if (child == NULL) {
        perror("child is a null pointer in {AST.c addChild}.\n");
        exit(EXIT_FAILURE);
    }
    addToArrayList(&parent->children, child);
}

// Free an AST node
void freeASTNode(ASTNode *node) {
    if (node == NULL) {
        return;
    }
    freeArrayList(&node->children);
    if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0) {
        free(node->value.identifier);
    }
    free(node->name);
    free(node);
}

// printAST helper function
static void print(const ASTNode *root, const int level) {
    if (root->flag) {
        printf("%*s%s", level * 2, "", root->name);
        if (strcmp(root->name, "INT") == 0) {
            printf(": %d", root->value.int_value);
        } else if (strcmp(root->name, "FLOAT") == 0) {
            printf(": %f", root->value.float_value);
        } else if (strcmp(root->name, "TYPE") == 0 || strcmp(root->name, "ID") == 0) {
            printf(": %s", root->value.identifier);
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

// print out entire AST tree
void printAST(const ASTNode *root) {
    if (root == NULL) {
        perror("root is null in {AST.c printAST}.");
        exit(EXIT_FAILURE);
    }
    print(root, 0);
}

void printASTRoot() {
    printAST(root);
}

void cleanAST() {
    freeASTNode(root);
}

//////test code/////////////////////////////////////////////
#ifdef AST_test
int main(int argc, char const *argv[]) {
    // Test 1: Create a simple AST with a root and two children
    ASTNode *root = createASTNode("root", 1, 0);
    ASTNode *child1 = createASTNode("child1", 2, 0);
    ASTNode *child2 = createASTNode("child2", 3, 0);
    addChild(root, child1);
    addChild(root, child2);

    ASTNode *child3 = createASTNode("SEMI", 4, 1);
    ASTNode *child4 = createASTNode("COMMA", 5, 1);
    addChild(root, child3);
    addChild(child1, child4);

    ValueUnion value = {.int_value = 3};
    ASTNode *int_child = createASTNodeWithValue("INT", 6, value);

    ValueUnion value1 = {.identifier = "hello, wo!"};
    ASTNode *id_child = createASTNodeWithValue("ID", 7, value1);

    ValueUnion value3 = {.float_value = 3.01};
    ASTNode *float_child = createASTNodeWithValue("FLOAT", 8, value3);

    ValueUnion value4 = {.identifier = "int"};
    ASTNode *type_child = createASTNodeWithValue("TYPE", 9, value4);

    addChild(child4, int_child);
    addChild(child4, id_child);
    addChild(child2, float_child);
    addChild(child2, type_child);

    printf("Test 1: Simple AST\n");
    printAST(root);
    freeASTNode(root);

    return 0;
}
#endif
