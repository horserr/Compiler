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
const Data* searchWithName(const RedBlackTree* tree, const char* name);
void freeRedBlackTree(RedBlackTree* tree);
// debug utilities
const char* typeToString(const Type* type);
const char* dataToString(const Data* data);
void inorder(const RedBlackTree* tree);

// utility function
void freeType(Type* t);
void freeData(Data* d);
const Type* deepCopyType(const Type* src);
const Data* deepCopyData(const Data* src);
int typeEqual(const Type* t1, const Type* t2);
int getArrayDimension(const Type* arrayType, Type** base_type);


// type definition
struct structFieldElement {
    char* name;
    Type* elemType;
    structFieldElement* next;
};

struct Type {
    enum { INT, FLOAT, ARRAY, STRUCT, ERROR } kind;
    // error is the most generic type and only use when resolve expression 'resolveExp SymbolTable.c'
    union {
        // array needs element type and size
        struct {
            Type* elemType;
            int size;
        } array;

        struct {
            char* struct_name;
            structFieldElement* fields;
        } structure;
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
