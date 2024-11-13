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
#ifdef LOCAL
#include <utils.h>
#else
#include "../utils/utils.h"
#endif


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
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
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
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
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
            } else {
                if (k == k->parent->right) {
                    k = k->parent;
                    leftRotate(tree, k);
                }
                strcpy(k->parent->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                rightRotate(tree, k->parent->parent);
            }
        } else {
            Node* u = k->parent->parent->left; // uncle
            if (strcmp(u->color, "RED") == 0) {
                strcpy(k->parent->color, "BLACK");
                strcpy(u->color, "BLACK");
                strcpy(k->parent->parent->color, "RED");
                k = k->parent->parent;
            } else {
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

/**
 * @brief insert data to the red black tree
 * @note data is not deep copied. Instead, simply pointing to it.
*/
void insert(RedBlackTree* tree, Data* data) {
    Node* new_node = createNode(data, tree->NIL);

    Node* parent = NULL;
    Node* current = tree->root;

    // BST insert
    while (current != tree->NIL) {
        parent = current;
        if (nodeCompareWithData(current, data) > 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    new_node->parent = parent;

    if (parent == NULL) {
        tree->root = new_node;
    } else if (nodeCompareWithData(parent, data) > 0) {
        parent->left = new_node;
    } else {
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

const Data* searchWithName(const RedBlackTree* tree, const char* name) {
    assert(tree != NULL);
    Data* data = malloc(sizeof(Data));
    data->name = my_strdup(name);
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

void freeType(Type* t) {
    if (t == NULL) {
        perror("Can't free a null pointer. {RBTree.c freeType}\n");
        exit(EXIT_FAILURE);
    }
    switch (t->kind) {
    case INT:
    case FLOAT:
    case ERROR:
        break;
    case ARRAY:
        freeType(t->array.elemType);
        break;
    case STRUCT:
        free(t->structure.struct_name);
        freeStructFieldElement(t->structure.fields);
        break;
    default:
        perror("false type to free in {RBTree.c freeType}.\n");
        exit(EXIT_FAILURE);
    }
    free(t);
}

void freeData(Data* d) {
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
/**
 * @brief turn Type into string
 * @return a copy of string
*/
const char* typeToString(const Type* type) {
    assert(type != NULL);
    switch (type->kind) {
    case ERROR: return my_strdup("T: Error ");
    case INT: return my_strdup("T: int ");
    case FLOAT: return my_strdup("T: float ");
    case ARRAY: return my_strdup("T: array ");
    case STRUCT:
        char buffer[50];
        sprintf(buffer, "T: struct, name: %s ", type->structure.struct_name);
        return my_strdup(buffer);
    default: return my_strdup("**UNKNOWN TYPE**");
    }
}


/**
 * @brief turn Data into string
 * @return a copy of string
*/
const char* dataToString(const Data* data) {
    if (data == NULL) {
        perror("Can't print out null data. {RBTree.c dataToString}\n");
        exit(EXIT_FAILURE);
    }
    const char buffer[80];
    if (data->kind == VAR) {
        const char* strType = typeToString(data->variable.type);
        sprintf(buffer, "<V: %s (%s)>", data->name, strType);
        free((char*)strType);
    } else { // data->kind == FUNC
        char tmp[30];
        sprintf(tmp, "<F: %s (", data->name);
        strcat(buffer, tmp);
        for (int i = 0; i < data->function.argc; ++i) {
            const char* strType = typeToString(data->function.argvTypes[i]);
            strcat(buffer, strType);
            free((char*)strType);
        }
        strcat(buffer,")>");
    }
    return my_strdup(buffer);
}

// utility functions
/**
 * @brief check two types are the same or not
 * @return 1 if same; 0 distinct.
*/
int typeEqual(const Type* t1, const Type* t2) {
    assert(t1 != NULL && t2 != NULL);
    assert(t1->kind != ERROR && t2->kind != ERROR);
    if (t1->kind != t2->kind) {
        return 0;
    } // t1->kind == t2->kind
    if (t1->kind == INT || t1->kind == FLOAT) {
        return 1;
    }
    if (t1->kind == ARRAY) {
        Type* base1 = NULL;
        Type* base2 = NULL;
        const int d1 = getArrayDimension(t1, &base1);
        const int d2 = getArrayDimension(t2, &base2);
        if (d1 != d2) {
            return 0;
        }
        const int rst = typeEqual(base1, base2);
        freeType(base1);
        freeType(base2);
        return rst;
    } // t1->kind == STRUCT
    return strcmp(t1->structure.struct_name, t2->structure.struct_name) == 0;
}

static structFieldElement* deepCopyFieldElement(const structFieldElement* src) {
    if (src == NULL) {
        return NULL;
    }
    structFieldElement* dst = malloc(sizeof(structFieldElement));
    dst->name = my_strdup(src->name);
    dst->elemType = deepCopyType(src->elemType);
    dst->next = deepCopyFieldElement(src->next);
    return dst;
}

const Type* deepCopyType(const Type* src) {
    assert(src != NULL);
    Type* dst = malloc(sizeof(Type));
    dst->kind = src->kind;
    if (src->kind == ARRAY) {
        dst->array.size = src->array.size;
        dst->array.elemType = deepCopyType(src->array.elemType);
    } else if (src->kind == STRUCT) {
        dst->structure.struct_name = my_strdup(src->structure.struct_name);
        dst->structure.fields = deepCopyFieldElement(src->structure.fields);
    }
    return dst;
}

const Data* deepCopyData(const Data* src) {
    assert(src != NULL);
    Data* dst = malloc(sizeof(Data));
    dst->name = my_strdup(src->name);
    dst->kind = src->kind;
    if (src->kind == VAR) {
        dst->variable.type = deepCopyType(src->variable.type);
    } else if (src->kind == FUNC) {
        dst->function.returnType = deepCopyType(src->function.returnType);
        const int argc = src->function.argc;
        dst->function.argc = argc;
        dst->function.argvTypes = malloc(sizeof(Type*) * argc);
        for (int i = 0; i < argc; ++i) {
            dst->function.argvTypes[i] = deepCopyType(src->function.argvTypes[i]);
        }
    }
    return dst;
}

/**
 * @brief get array dimension and may get base_type as well
 * @param arrayType
 * @param base_type if set to NULL, nothing will happen;
 *                  if not null while it's pointing to NULL, then tie a new copy of base type.
 * @return the dimension of array type
*/
int getArrayDimension(const Type* arrayType, Type** base_type) {
    assert(arrayType != NULL);
    int dimension = 0;
    while (arrayType->kind == ARRAY) {
        dimension++;
        arrayType = arrayType->array.elemType;
    }
    if (base_type != NULL) {
        assert(*base_type == NULL);                  // *base_type should be initialized to NULL
        *base_type = (Type*)deepCopyType(arrayType); // suppress warning
    }
    return dimension;
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
