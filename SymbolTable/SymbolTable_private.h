#ifndef SYMBOL_TABLE_PRIVATE
#define SYMBOL_TABLE_PRIVATE

#include "RBTree.h"
#include "SymbolTable.h"

typedef struct {
    char* expr;
    void (*func)(const ParseTNode*);
} ExprFuncPair;

void resolveArgs(const ParseTNode* node);

void resolveExp(const ParseTNode* node);

void resolveDec(const ParseTNode* node);

void resolveDecList(const ParseTNode* node);

void resolveDef(const ParseTNode* node);

void resolveDefList(const ParseTNode* node);

void resolveStmt(const ParseTNode* node);

void resolveStmtList(const ParseTNode* node);

void resolveCompSt(const ParseTNode* node);

void resolveParamDec(const ParseTNode* node);

void resolveVarList(const ParseTNode* node);

void resolveFunDec(const ParseTNode* node);

void resolveVarDec(const ParseTNode* node);

void resolveTag(const ParseTNode* node);

void resolveOptTag(const ParseTNode* node);

void resolveStructSpecifier(const ParseTNode* node);

void resolveSpecifier(const ParseTNode* node);

void resolveExtDecList(const ParseTNode* node);

void resolveExtDef(const ParseTNode* node);

void resolveExtDefList(const ParseTNode* node);

const ExprFuncPair exprFuncLookUpTable[] = {
    {.expr = "Args", .func = resolveArgs},
    {.expr = "Exp", .func = resolveExp},
    {.expr = "Dec", .func = resolveDec},
    {.expr = "DecList", .func = resolveDecList},
    {.expr = "Def", .func = resolveDef},
    {.expr = "DefList", .func = resolveDefList},
    {.expr = "Stmt", .func = resolveStmt},
    {.expr = "StmtList", .func = resolveStmtList},
    {.expr = "CompSt", .func = resolveCompSt},
    {.expr = "ParamDec", .func = resolveParamDec},
    {.expr = "VarList", .func = resolveVarList},
    {.expr = "FunDec", .func = resolveFunDec},
    {.expr = "VarDec", .func = resolveVarDec},
    {.expr = "Tag", .func = resolveTag},
    {.expr = "OptTag", .func = resolveOptTag},
    {.expr = "StructSpecifier", .func = resolveStructSpecifier},
    {.expr = "Specifier", .func = resolveSpecifier},
    {.expr = "ExtDecList", .func = resolveExtDecList},
    {.expr = "ExtDef", .func = resolveExtDef},
    {.expr = "ExtDefList", .func = resolveExtDefList}
};

#endif
