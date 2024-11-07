#include <assert.h>
#include <stdio.h>
#include "SymbolTable_private.h"

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

RedBlackTree* Map = NULL;

void resolverHelper(const ParseTNode* parent_node, const char* expr) {
    char* expr_copy = my_strdup(expr);
    const char* delim = " ";
    const char* e = strtok(expr_copy, delim);
    int i = 0;
    while (e != NULL) {
        const ParseTNode* node = parent_node->children.container[i];

        // look up function and call it
        int j = 0;
        while (j < ARRAY_LEN(exprFuncLookUpTable)) {
            const ExprFuncPair pair = exprFuncLookUpTable[j];
            if (strcmp(e, pair.expr) == 0) {
                pair.func(node);
                break;
            }
            j++;
        }

        if (j == ARRAY_LEN(exprFuncLookUpTable)) {
            fprintf(stderr, "Invalid expression name %s {SymbolTable.c resolver}.\n", e);
            exit(EXIT_FAILURE);
        }
        e = strtok(NULL, delim);
        i++;
    }
    free(expr_copy);
    assert(i==parent_node->children.num);
}

void resolver(const ParseTNode* node, const char* expressions[], const int length) {
    int i = 0;
    while (i < length) {
        if (nodeChildrenNameEqualHelper(node, expressions[i])) {
            resolverHelper(node, expressions[i]);
            break;
        }
        i++;
    }
    if (i == length) {
        perror("There must be a typo in expression list. Checking from {SymbolTable.c resolver}\n");
        for (int j = 0; j < length; ++j) {
            fprintf(stderr, "<%02d.>\t%s\n", j, expressions[j]);
        }
        exit(EXIT_FAILURE);
    }
}

void resolveArgs(const ParseTNode* node) {
    const char* expressions[] = {
        "Exp COMMA Args",
        "Exp"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveExp(const ParseTNode* node) {
    const char* expressions[] = {
        "Exp ASSIGNOP Exp",
        "Exp AND Exp",
        "Exp OR Exp",
        "Exp RELOP Exp",
        "Exp PLUS Exp",
        "Exp MINUS Exp",
        "Exp STAR Exp",
        "Exp DIV Exp",
        "LP Exp RP",
        "MINUS Exp",
        "NOT Exp",
        "ID LP Args RP",
        "ID LP RP",
        "Exp LB Exp RB",
        "Exp DOT ID",
        "ID",
        "INT",
        "FLOAT"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveDec(const ParseTNode* node) {
    const char* expressions[] = {
        "VarDec",
        "VarDec ASSIGNOP Exp"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveDecList(const ParseTNode* node) {
    const char* expressions[] = {
        "Dec",
        "Dec COMMA DecList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveDef(const ParseTNode* node) {
    const char* expressions[] = {
        "Specifier DecList SEMI"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveDefList(const ParseTNode* node) {
    const char* expressions[] = {
        "",
        "Def DefList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveStmt(const ParseTNode* node) {
    const char* expressions[] = {
        "Exp SEMI",
        "CompSt",
        "RETURN Exp SEMI",
        "IF LP Exp RP Stmt",
        "IF LP Exp RP Stmt ELSE Stmt",
        "WHILE LP Exp RP Stmt"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveStmtList(const ParseTNode* node) {
    const char* expressions[] = {
        "",
        "Stmt StmtList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveCompSt(const ParseTNode* node) {
    const char* expressions[] = {
        "LC DefList StmtList RC"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveParamDec(const ParseTNode* node) {
    const char* expressions[] = {
        "Specifier VarDec"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveVarList(const ParseTNode* node) {
    const char* expressions[] = {
        "ParamDec COMMA VarList",
        "ParamDec"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveFunDec(const ParseTNode* node) {
    const char* expressions[] = {
        "ID LP VarList RP",
        "ID LP RP"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveVarDec(const ParseTNode* node) {
    const char* expressions[] = {
        "ID",
        "VarDec LB INT RB"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveTag(const ParseTNode* node) {
    const char* expressions[] = {
        "ID"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveOptTag(const ParseTNode* node) {
    const char* expressions[] = {
        "",
        "ID"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveStructSpecifier(const ParseTNode* node) {
    const char* expressions[] = {
        "STRUCT OptTag LC DefList RC",
        "STRUCT Tag"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveSpecifier(const ParseTNode* node) {
    const char* expressions[] = {
        "TYPE",
        "StructSpecifier"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveExtDecList(const ParseTNode* node) {
    const char* expressions[] = {
        "VarDec",
        "VarDec COMMA ExtDecList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveExtDef(const ParseTNode* node) {
    const char* expressions[] = {
        "Specifier ExtDecList SEMI",
        "Specifier SEMI",
        "Specifier FunDec CompSt"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}

void resolveExtDefList(const ParseTNode* node) {
    const char* expressions[] = {
        "",
        "ExtDef ExtDefList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
}


void buildTable(const ParseTNode* root) {
    assert(strcmp(root->name,"Program") == 0);
    char* expressions[] = {
        "ExtDefList"
    };
    // Map = createRedBlackTree();
    resolver(root, expressions,ARRAY_LEN(expressions));
    // freeRedBlackTree(Map);
}

#ifdef SYMBOL_test
int main() {
    // todo header file structure for parseTNode
    /*
     *  Program
            |   ExtDefList
                    |   ExtDef
                    |   ExtDefList
                            |   ExtDef
                            |   ExtDefList
    */
    ParseTNode* root = createParseTNode("Program", 1, 0);
    ParseTNode* child1 = createParseTNode("ExtDefList", 1, 0);
    ParseTNode* child1_1 = createParseTNode("ExtDef", 1, 0);
    ParseTNode* child1_2 = createParseTNode("ExtDefList", 1, 0);
    ParseTNode* child1_2_1 = createParseTNode("ExtDef", 1, 0);
    ParseTNode* child1_2_2 = createParseTNode("ExtDefList", 1, 0);
    addChild(root, child1);
    addChild(child1, child1_1);
    addChild(child1, child1_2);
    addChild(child1_2, child1_2_1);
    addChild(child1_2, child1_2_2);
    buildTable(root);
    freeParseTNode(root);
}
#endif
