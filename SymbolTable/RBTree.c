#include "RBTree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

typedef struct structFieldElement {
    char* name;
    Type* elemType;
    struct structFieldElement* next; // todo be cautious about null pointer
} structFieldElement;

struct {
    enum { INT, FLOAT, ARRAY, STRUCTURE } kind;

    union {
        // array needs element type and size
        struct {
            Type* elemType;
            int size;
        } array;

        // structure needs a list of fields
        structFieldElement* fields;
    };
} TYPE;

typedef struct {
    char* name;

    enum { VAR, FUNC } kind;

    union {
        struct {
            Type* type;
        } variable;

        struct {
            Type* returnType;
            int argc;
            Type** argvTypes;
        } function;
    };
} Data;

///// Red Black Tree /////////////////////////////////////////
// Node structure for the Red-Black Tree
typedef struct Node {
    int data;
    char color[6];
    struct Node *left, *right, *parent;
} Node;

// Red-Black Tree class
typedef struct {
    Node* root;
    Node* NIL;
} RedBlackTree;

/**
 *@return positive if n1 > data
 *@return negative if n1 < data
 *@return zero     if n1 = data
*/
static int nodeCompareWithData(const Node* n1, const int data) {
    return n1->data - data;
}

static int copyNodeData(int data) {
    return data;
}

// Utility function to create a new node
static Node* createNode(int data, Node* NIL) {
    Node* newNode = malloc(sizeof(Node));
    newNode->data = data;
    strcpy(newNode->color, "RED");
    newNode->left = NIL;
    newNode->right = NIL;
    newNode->parent = NULL;
    return newNode;
}

// Utility function to perform left rotation
static void leftRotate(RedBlackTree* tree, Node* x) {
    Node* y = x->right;
    x->right = y->left;
    if (y->left != tree->NIL) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        tree->root = y;
    }
    else if (x == x->parent->left) {
        x->parent->left = y;
    }
    else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

// Utility function to perform right rotation
static void rightRotate(RedBlackTree* tree, Node* x) {
    Node* y = x->left;
    x->left = y->right;
    if (y->right != tree->NIL) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        tree->root = y;
    }
    else if (x == x->parent->right) {
        x->parent->right = y;
    }
    else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

// Function to fix Red-Black Tree properties after insertion
static void fixInsert(RedBlackTree* tree, Node* k) {
    while (k != tree->root && strcmp(k->parent->color, "RED") == 0) {
        if (k->parent == k->parent->parent->left) {
            Node* u = k->parent->parent->right; // uncle
            if (strcmp(u->color, "RED") == 0) {
                strcpy(k->parent->color, "BLACK");
                strcpy(u->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                k = k->parent->parent;
            }
            else {
                if (k == k->parent->right) {
                    k = k->parent;
                    leftRotate(tree, k);
                }
                strcpy(k->parent->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                rightRotate(tree, k->parent->parent);
            }
        }
        else {
            Node* u = k->parent->parent->left; // uncle
            if (strcmp(u->color, "RED") == 0) {
                strcpy(k->parent->color, "BLACK");
                strcpy(u->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                k = k->parent->parent;
            }
            else {
                if (k == k->parent->left) {
                    k = k->parent;
                    rightRotate(tree, k);
                }
                strcpy(k->parent->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                leftRotate(tree, k->parent->parent);
            }
        }
    }
    strcpy(tree->root->color, "BLACK");
}

// Inorder traversal helper function
static void inorderHelper(const Node* node, Node* NIL) {
    if (node != NIL) {
        inorderHelper(node->left, NIL);
        printf("%d ", node->data);
        inorderHelper(node->right, NIL);
    }
}

// Search helper function
static Node* searchHelper(Node* node, int data, Node* NIL) {
    if (node == NIL || nodeCompareWithData(node, data) == 0/*node->data == data*/) {
        return node;
    }
    if (nodeCompareWithData(node, data) > 0) {
        return searchHelper(node->left, data, NIL);
    }
    return searchHelper(node->right, data, NIL);
}

// Constructor
RedBlackTree* createRedBlackTree() {
    RedBlackTree* tree = malloc(sizeof(RedBlackTree));
    tree->NIL = createNode(0, NULL);
    strcpy(tree->NIL->color, "BLACK");
    tree->NIL->left = tree->NIL->right = tree->NIL;
    tree->root = tree->NIL;
    return tree;
}

// Insert function
void insert(RedBlackTree* tree, int data) {
    Node* new_node = createNode(data, tree->NIL);

    Node* parent = NULL;
    Node* current = tree->root;

    // BST insert
    while (current != tree->NIL) {
        parent = current;
        if (nodeCompareWithData(current, data) > 0/*new_node->data < current->data*/) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    new_node->parent = parent;

    if (parent == NULL) {
        tree->root = new_node;
    }
    else if (nodeCompareWithData(parent, data) > 0/*new_node->data < parent->data*/) {
        parent->left = new_node;
    }
    else {
        parent->right = new_node;
    }

    if (new_node->parent == NULL) {
        strcpy(new_node->color, "BLACK");
        return;
    }

    if (new_node->parent->parent == NULL) {
        return;
    }

    fixInsert(tree, new_node);
}

// Inorder traversal
void inorder(const RedBlackTree* tree) {
    inorderHelper(tree->root, tree->NIL);
}

// Search function
Node* search(const RedBlackTree* tree, int data) {
    return searchHelper(tree->root, data, tree->NIL);
}

// Helper function to free nodes
static void freeNodes(Node* node, Node* NIL) {
    if (node != NIL) {
        freeNodes(node->left, NIL);
        freeNodes(node->right, NIL);
        free(node);
    }
}

// Function to free the Red-Black Tree
void freeRedBlackTree(RedBlackTree* tree) {
    freeNodes(tree->root, tree->NIL);
    free(tree->NIL);
    free(tree);
}

#ifdef RBTREE_test
int main() {
    RedBlackTree* rbt = createRedBlackTree();

    // Inserting elements
    insert(rbt, 10);
    insert(rbt, 20);
    insert(rbt, 30);
    insert(rbt, 15);
    insert(rbt, 25);
    insert(rbt, 5);
    insert(rbt, 1);

    // Inorder traversal
    printf("Inorder traversal:\n");
    inorder(rbt); // Expected Output: 1 5 10 15 20 25 30
    printf("\n");

    // Search for nodes
    printf("Search for 15: %d\n", search(rbt, 15) != rbt->NIL); // Expected Output: 1 (true)
    printf("Search for 25: %d\n", search(rbt, 25) != rbt->NIL); // Expected Output: 1 (true)
    printf("Search for 100: %d\n", search(rbt, 100) != rbt->NIL); // Expected Output: 0 (false)

    // Edge cases
    printf("Search for root (10): %d\n", search(rbt, 10) != rbt->NIL); // Expected Output: 1 (true)
    printf("Search for minimum (1): %d\n", search(rbt, 1) != rbt->NIL); // Expected Output: 1 (true)
    printf("Search for maximum (30): %d\n", search(rbt, 30) != rbt->NIL); // Expected Output: 1 (true)

    // Insert duplicate
    insert(rbt, 10);
    printf("Inorder traversal after inserting duplicate 10:\n");
    inorder(rbt); // Expected Output: 1 5 10 10 15 20 25 30
    printf("\n");

    // Insert more elements
    insert(rbt, 35);
    insert(rbt, 40);
    insert(rbt, 50);
    printf("Inorder traversal after inserting more elements:\n");
    inorder(rbt); // Expected Output: 1 5 10 10 15 20 25 30 35 40 50
    printf("\n");

    freeRedBlackTree(rbt);

    return 0;
}
#endif
