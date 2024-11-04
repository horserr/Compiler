#ifndef RBTREE
#define RBTREE

// type declaration
typedef struct Type Type;
typedef struct structFieldElement structFieldElement;
typedef struct Data Data;
typedef struct RedBlackTree RedBlackTree;

// function declaration
RedBlackTree* createRedBlackTree();
void insert(RedBlackTree* tree, Data* data);
const Data* searchWithName(const RedBlackTree* tree, char* name);
void freeRedBlackTree(RedBlackTree* tree);
// debug utilities
void typeToString(Type* t);
void dataToString(Data* d);
void inorder(const RedBlackTree* tree);

// type definition
struct structFieldElement {
    char* name;
    Type* elemType;
    structFieldElement* next;
};

struct Type {
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
};

struct Data {
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
};
#endif
