/** Credit:
This code is provided by @author costheta_z

    C implementation for Red-Black Tree Insertion.
    visit https://www.geeksforgeeks.org/introduction-to-red-black-tree/?ref=header_outind for more information
**/
#include "RBTree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void freeType(Type* t);

///// Red Black Tree /////////////////////////////////////////
// Node structure for the Red-Black Tree
typedef struct Node {
    Data* data;
    char color[6];
    struct Node *left, *right, *parent;
} Node;

// Red-Black Tree class
struct RedBlackTree {
    Node* root;
    Node* NIL;
};

/**
 *@return positive if n1 > data
 *@return negative if n1 < data
 *@return zero     if n1 = data
*/
static int nodeCompareWithData(const Node* n1, const Data* data) {
    return strcmp(n1->data->name, data->name);
}

// Utility function to create a new node
static Node* createNode(Data* data, Node* NIL) {
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
        dataToString(node->data);
        inorderHelper(node->right, NIL);
    }
}

// Search helper function
static Node* searchHelper(Node* node, Data* data, Node* NIL) {
    if (node == NIL || nodeCompareWithData(node, data) == 0) {
        return node;
    }
    if (nodeCompareWithData(node, data) > 0) {
        return searchHelper(node->left, data, NIL);
    }
    return searchHelper(node->right, data, NIL);
}

// Constructor or Red Black Tree
RedBlackTree* createRedBlackTree() {
    RedBlackTree* tree = malloc(sizeof(RedBlackTree));
    tree->NIL = createNode(NULL, NULL);
    strcpy(tree->NIL->color, "BLACK");
    tree->NIL->left = tree->NIL->right = tree->NIL;
    tree->root = tree->NIL;
    return tree;
}

// Insert function
void insert(RedBlackTree* tree, Data* data) {
    Node* new_node = createNode(data, tree->NIL);

    Node* parent = NULL;
    Node* current = tree->root;

    // BST insert
    while (current != tree->NIL) {
        parent = current;
        if (nodeCompareWithData(current, data) > 0) {
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
    else if (nodeCompareWithData(parent, data) > 0) {
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
static Node* search(const RedBlackTree* tree, Data* data) {
    return searchHelper(tree->root, data, tree->NIL);
}

const Data* searchWithName(const RedBlackTree* tree, char* name) {
    Data* data = malloc(sizeof(Data));
    data->name = name;
    const Data* rst = search(tree, data)->data;
    free(data);
    return rst;
}

///// utilities functions of free ////////////////////////////
static void freeStructFieldElement(structFieldElement* e) {
    while (e != NULL) {
        structFieldElement* tmp = e;
        e = e->next;
        free(tmp->name);
        freeType(tmp->elemType);
        free(tmp);
    }
}

static void freeType(Type* t) {
    if (t == NULL) {
        perror("Can't free a null pointer. {RBTree.c freeType}\n");
        exit(EXIT_FAILURE);
    }
    switch (t->kind) {
    case INT:
    case FLOAT:
        break;
    case ARRAY:
        freeType(t->array.elemType);
        break;
    case STRUCTURE:
        freeStructFieldElement(t->fields);
        break;
    default:
        perror("false type to free in {RBTree.c freeType}.\n");
        exit(EXIT_FAILURE);
    }
    free(t);
}

static void freeData(Data* d) {
    if (d == NULL) {
        perror("Can't free null Data. {RBTree.c freeData}\n");
        exit(EXIT_FAILURE);
    }
    switch (d->kind) {
    case VAR:
        freeType(d->variable.type);
        break;
    case FUNC:
        freeType(d->function.returnType);
        for (int i = 0; i < d->function.argc; ++i) {
            freeType(d->function.argvTypes[i]);
        }
        free(d->function.argvTypes);
        break;
    default:
        perror("false type to free in {RBTree.c freeData}.\n");
        exit(EXIT_FAILURE);
    }
    free(d->name);
    free(d);
}

// Helper function to free nodes
static void freeNodes(Node* node, Node* NIL) {
    if (node != NIL) {
        freeNodes(node->left, NIL);
        freeNodes(node->right, NIL);
        freeData(node->data);
        free(node);
    }
}

// Function to free the Red-Black Tree
void freeRedBlackTree(RedBlackTree* tree) {
    freeNodes(tree->root, tree->NIL);
    free(tree->NIL);
    free(tree);
}

///// print helpers //////////////////////////////////////////
void typeToString(Type* t) {
    // todo
}

void dataToString(Data* d) {
    if (d == NULL) {
        perror("Can't print out null data. {RBTree.c dataToString}\n");
        exit(EXIT_FAILURE);
    }
    switch (d->kind) {
    case VAR:
        printf("Data<variable>: \n");
        break;
    case FUNC:
        printf("Data<function>: \n");
        break;
    default:
        perror("false type of data. {RBTree.c dataToString}\n");
        exit(EXIT_FAILURE);
    }
    printf("%*s%s\n", 4, "", d->name);
}

#ifdef RBTREE_test
void testInsertAndSearchIntFloat() {
    const RedBlackTree* rbt = createRedBlackTree();

    Data* data1 = malloc(sizeof(Data));
    data1->name = strdup("data1");
    data1->kind = VAR;
    data1->variable.type = malloc(sizeof(Type));
    data1->variable.type->kind = INT;

    Data* data2 = malloc(sizeof(Data));
    data2->name = strdup("data2");
    data2->kind = VAR;
    data2->variable.type = malloc(sizeof(Type));
    data2->variable.type->kind = FLOAT;

    insert(rbt, data1);
    insert(rbt, data2);

    Data searchData;
    searchData.name = "data1";
    Node* foundNode = search(rbt, &searchData);
    assert(foundNode != rbt->NIL);
    assert(strcmp(foundNode->data->name, "data1") == 0);

    Data* foundData = searchWithName(rbt, "data2");
    assert(foundData != NULL);
    assert(strcmp(foundData->name, "data2") == 0);

    searchData.name = "data3";
    foundNode = search(rbt, &searchData);
    assert(foundNode == rbt->NIL);

    foundData = searchWithName(rbt, "data4");
    assert(foundData == NULL);

    freeRedBlackTree(rbt);
}

void testInsertAndSearchArray() {
    RedBlackTree* rbt = createRedBlackTree();

    Data* data = malloc(sizeof(Data));
    data->name = strdup("arrayData");
    data->kind = VAR;
    data->variable.type = malloc(sizeof(Type));
    data->variable.type->kind = ARRAY;
    data->variable.type->array.elemType = malloc(sizeof(Type));
    data->variable.type->array.elemType->kind = INT;
    data->variable.type->array.size = 10;

    insert(rbt, data);

    Data searchData;
    searchData.name = "arrayData";
    Node* foundNode = search(rbt, &searchData);
    assert(foundNode != rbt->NIL);
    assert(strcmp(foundNode->data->name, "arrayData") == 0);

    freeRedBlackTree(rbt);
}

void testInsertAndSearchStructure() {
    RedBlackTree* rbt = createRedBlackTree();

    Data* data = malloc(sizeof(Data));
    data->name = strdup("structData");
    data->kind = VAR;
    data->variable.type = malloc(sizeof(Type));
    data->variable.type->kind = STRUCTURE;

    structFieldElement* field1 = malloc(sizeof(structFieldElement));
    field1->name = strdup("field1");
    field1->elemType = malloc(sizeof(Type));
    field1->elemType->kind = INT;
    field1->next = NULL;

    structFieldElement* field2 = malloc(sizeof(structFieldElement));
    field2->name = strdup("field2");
    field2->elemType = malloc(sizeof(Type));
    field2->elemType->kind = FLOAT;
    field2->next = field1;

    data->variable.type->fields = field2;

    insert(rbt, data);

    Data searchData;
    searchData.name = "structData";
    Node* foundNode = search(rbt, &searchData);
    assert(foundNode != rbt->NIL);
    assert(strcmp(foundNode->data->name, "structData") == 0);

    freeRedBlackTree(rbt);
}

void testInsertAndSearchFunction() {
    RedBlackTree* rbt = createRedBlackTree();

    Data* data = malloc(sizeof(Data));
    data->name = strdup("funcData");
    data->kind = FUNC;
    data->function.returnType = malloc(sizeof(Type));
    data->function.returnType->kind = INT;
    data->function.argc = 2;
    data->function.argvTypes = malloc(2 * sizeof(Type*));
    data->function.argvTypes[0] = malloc(sizeof(Type));
    data->function.argvTypes[0]->kind = FLOAT;
    data->function.argvTypes[1] = malloc(sizeof(Type));
    data->function.argvTypes[1]->kind = INT;

    insert(rbt, data);

    Data searchData;
    searchData.name = "funcData";
    Node* foundNode = search(rbt, &searchData);
    assert(foundNode != rbt->NIL);
    assert(strcmp(foundNode->data->name, "funcData") == 0);

    freeRedBlackTree(rbt);
}

/*
int haha(struct_a, [float, 10])
struct_a:
    struct_b:
        arrayb[int, 10]
    arrayc[struct_c, 5]
struct_c:
    int a
    int b
*/
void comprehendTest() {
    RedBlackTree* rbt = createRedBlackTree();
    Data* func_data = malloc(sizeof(Data));
    func_data->name = strdup("haha");
    func_data->kind = FUNC;
    func_data->function.returnType = malloc(sizeof(Type));
    func_data->function.returnType->kind = INT;
    func_data->function.argc = 2;
    func_data->function.argvTypes = malloc(2 * sizeof(Type*));
    // todo
}

int main() {
    testInsertAndSearchIntFloat();
    printf("Test Insert and Search INT and FLOAT passed.\n");

    testInsertAndSearchArray();
    printf("Test Insert and Search ARRAY passed.\n");

    testInsertAndSearchStructure();
    printf("Test Insert and Search STRUCTURE passed.\n");

    testInsertAndSearchFunction();
    printf("Test Insert and Search FUNCTION passed.\n");

    return 0;
}
#endif
