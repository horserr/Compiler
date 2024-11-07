#include <assert.h>
#include <stdio.h>
#include "Environment.h"
#include "SymbolTable_private.h"

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

Environment* globalEnv = NULL;
Environment* currentEnv = NULL;

ResolvePtr lookUpFunction(const char* expression) {
    for (int i = 0; i < ARRAY_LEN(exprFuncLookUpTable); ++i) {
        const ExprFuncPair pair = exprFuncLookUpTable[i];
        if (strcmp(pair.expr, expression) == 0) {
            return pair.func;
        }
    }
    return NULL;
}

void resolverHelper(const ParseTNode* parent_node, const char* expr) {
    char* expr_copy = my_strdup(expr);
    const char* delim = " ";
    const char* e = strtok(expr_copy, delim);
    int i = 0;
    while (e != NULL) {
        const ParseTNode* node = parent_node->children.container[i];
        const ResolvePtr f = lookUpFunction(e);
        if (f == NULL) {
            fprintf(stderr, "Invalid expression name \"%s\" {SymbolTable.c resolver}.\n", e);
            exit(EXIT_FAILURE);
        }

        // default argument for option is null
        f(node, NULL);
        e = strtok(NULL, delim);
        i++;
    }
    free(expr_copy);
    assert(i==parent_node->children.num);
}

/**
 * @return the index of expr fitted in expressions
*/
int resolver(const ParseTNode* node, const char* expressions[], const int length) {
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
    return i;
}

void* resolveArgs(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Exp COMMA Args",
        "Exp"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveExp(const ParseTNode* node, void* opt) {
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
    return NULL;
}

void* resolveDec(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "VarDec",
        "VarDec ASSIGNOP Exp"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveDecList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Dec",
        "Dec COMMA DecList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveDef(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Specifier DecList SEMI"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    /*
    Type* type = resolveSpecifier(node);
    resolveDecList(node, type);
    */
    return NULL;
}

void* resolveDefList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "",
        "Def DefList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveStmt(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Exp SEMI",
        "CompSt",
        "RETURN Exp SEMI",
        "IF LP Exp RP Stmt",
        "IF LP Exp RP Stmt ELSE Stmt",
        "WHILE LP Exp RP Stmt"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveStmtList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "",
        "Stmt StmtList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveCompSt(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "LC DefList StmtList RC"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveParamDec(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Specifier VarDec"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveVarList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "ParamDec COMMA VarList",
        "ParamDec"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveFunDec(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "ID LP VarList RP",
        "ID LP RP"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveVarDec(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "ID",
        "VarDec LB INT RB"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveTag(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "ID"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveOptTag(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "",
        "ID"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveStructSpecifier(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "STRUCT OptTag LC DefList RC",
        "STRUCT Tag"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveSpecifier(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "TYPE",
        "StructSpecifier"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveExtDecList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "VarDec",
        "VarDec COMMA ExtDecList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveExtDef(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "Specifier ExtDecList SEMI",
        "Specifier SEMI",
        "Specifier FunDec CompSt"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}

void* resolveExtDefList(const ParseTNode* node, void* opt) {
    const char* expressions[] = {
        "",
        "ExtDef ExtDefList"
    };
    resolver(node, expressions, ARRAY_LEN(expressions));
    return NULL;
}


void buildTable(const ParseTNode* root) {
    assert(strcmp(root->name,"Program") == 0);
    char* expressions[] = {
        "ExtDefList"
    };
    globalEnv = currentEnv = newEnvironment(NULL);
    resolver(root, expressions,ARRAY_LEN(expressions));
    freeEnvironment(globalEnv);
}

#ifdef SYMBOL_test
int main() {
    // NOTE: incomplete test case
    // Create the root node
    ParseTNode* root = createParseTNode("Program", 1, 0);

    // Create ExtDefList node
    ParseTNode* extDefList = createParseTNode("ExtDefList", 1, 0);
    addChild(root, extDefList);

    // Create ExtDef node
    ParseTNode* extDef = createParseTNode("ExtDef", 1, 0);
    addChild(extDefList, extDef);

    ParseTNode* extDefList1 = createParseTNode("ExtDefList", 1, 0);
    addChild(extDefList, extDefList1);

    // Create Specifier node
    ParseTNode* specifier = createParseTNode("Specifier", 1, 0);
    addChild(extDef, specifier);

    // Create TYPE node with value
    ValueUnion typeValue;
    typeValue.str_value = "int";
    ParseTNode* type = createParseTNodeWithValue("TYPE", 1, typeValue);
    addChild(specifier, type);

    // Create FunDec node
    ParseTNode* funDec = createParseTNode("FunDec", 1, 0);
    addChild(extDef, funDec);

    // Create ID node with value
    ValueUnion idValue;
    idValue.str_value = "main";
    ParseTNode* id = createParseTNodeWithValue("ID", 1, idValue);
    addChild(funDec, id);

    // Create LP and RP nodes
    ParseTNode* lp = createParseTNode("LP", 1, 1);
    ParseTNode* rp = createParseTNode("RP", 1, 1);
    addChild(funDec, lp);
    addChild(funDec, rp);

    // Create CompSt node
    ParseTNode* compSt = createParseTNode("CompSt", 2, 0);
    addChild(extDef, compSt);

    // Create LC node
    ParseTNode* lc = createParseTNode("LC", 2, 1);
    addChild(compSt, lc);

    // Create DefList node
    ParseTNode* defList = createParseTNode("DefList", 3, 0);
    addChild(compSt, defList);

    // Create Def node
    ParseTNode* def = createParseTNode("Def", 3, 0);
    addChild(defList, def);

    // Create Specifier node for Def
    ParseTNode* defSpecifier = createParseTNode("Specifier", 3, 0);
    addChild(def, defSpecifier);

    // Create TYPE node with value for Def
    ParseTNode* defType = createParseTNodeWithValue("TYPE", 3, typeValue);
    addChild(defSpecifier, defType);

    // Create DecList node
    ParseTNode* decList = createParseTNode("DecList", 3, 0);
    addChild(def, decList);

    // Create Dec node
    ParseTNode* dec = createParseTNode("Dec", 3, 0);
    addChild(decList, dec);

    // Create VarDec node
    ParseTNode* varDec = createParseTNode("VarDec", 3, 0);
    addChild(dec, varDec);

    // Create ID node with value for VarDec
    ValueUnion varIdValue;
    varIdValue.str_value = "i";
    ParseTNode* varId = createParseTNodeWithValue("ID", 3, varIdValue);
    addChild(varDec, varId);

    // Create ASSIGNOP node
    ParseTNode* assignop = createParseTNode("ASSIGNOP", 3, 1);
    addChild(dec, assignop);

    // Create Exp node
    ParseTNode* exp = createParseTNode("Exp", 3, 0);
    addChild(dec, exp);

    // Create INT node with value
    ValueUnion intValue;
    intValue.int_value = 0;
    ParseTNode* intNode = createParseTNodeWithValue("INT", 3, intValue);
    addChild(exp, intNode);

    // Create SEMI node
    ParseTNode* semi = createParseTNode("SEMI", 3, 1);
    addChild(def, semi);

    // Create StmtList node
    ParseTNode* stmtList = createParseTNode("StmtList", 4, 0);
    addChild(compSt, stmtList);

    // Create Stmt node
    ParseTNode* stmt = createParseTNode("Stmt", 4, 0);
    addChild(stmtList, stmt);

    // Create Exp node for Stmt
    ParseTNode* stmtExp = createParseTNode("Exp", 4, 0);
    addChild(stmt, stmtExp);

    // Create Exp node for ID
    ParseTNode* idExp = createParseTNode("Exp", 4, 0);
    addChild(stmtExp, idExp);

    // Create ID node with value for Exp
    ValueUnion jValue;
    jValue.str_value = "j";
    ParseTNode* jId = createParseTNodeWithValue("ID", 4, jValue);
    addChild(idExp, jId);

    // Create ASSIGNOP node for Stmt
    ParseTNode* stmtAssignop = createParseTNode("ASSIGNOP", 4, 1);
    addChild(stmtExp, stmtAssignop);

    // Create Exp node for assignment
    ParseTNode* assignExp = createParseTNode("Exp", 4, 0);
    addChild(stmtExp, assignExp);

    // Create Exp node for ID i
    ParseTNode* iExp = createParseTNode("Exp", 4, 0);
    addChild(assignExp, iExp);

    // Create ID node with value for Exp
    ValueUnion iValue;
    iValue.str_value = "i";
    ParseTNode* iId = createParseTNodeWithValue("ID", 4, iValue);
    addChild(iExp, iId);

    // Create PLUS node
    ParseTNode* plus = createParseTNode("PLUS", 4, 1);
    addChild(assignExp, plus);

    // Create Exp node for INT 1
    ParseTNode* intExp = createParseTNode("Exp", 4, 0);
    addChild(assignExp, intExp);

    // Create INT node with value 1
    ValueUnion oneValue;
    oneValue.int_value = 1;
    ParseTNode* oneInt = createParseTNodeWithValue("INT", 4, oneValue);
    addChild(intExp, oneInt);

    // Create SEMI node for Stmt
    ParseTNode* stmtSemi = createParseTNode("SEMI", 4, 1);
    addChild(stmt, stmtSemi);

    // Create RC node
    ParseTNode* rc = createParseTNode("RC", 2, 1);
    addChild(compSt, rc);

    // Build the symbol table
    buildTable(root);

    // Free the parse tree
    freeParseTNode(root);
}
#endif
