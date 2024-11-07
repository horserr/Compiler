#ifndef SYMBOL_TABLE_PRIVATE
#define SYMBOL_TABLE_PRIVATE

#include "SymbolTable.h"

typedef void* (*ResolvePtr)(const ParseTNode*, void*);

typedef struct {
    char* expr;
    ResolvePtr func;
} ExprFuncPair;

void* resolveArgs(const ParseTNode* node, void* opt);

void* resolveExp(const ParseTNode* node, void* opt);

void* resolveDec(const ParseTNode* node, void* opt);

void* resolveDecList(const ParseTNode* node, void* opt);

void* resolveDef(const ParseTNode* node, void* opt);

void* resolveDefList(const ParseTNode* node, void* opt);

void* resolveStmt(const ParseTNode* node, void* opt);

void* resolveStmtList(const ParseTNode* node, void* opt);

void* resolveCompSt(const ParseTNode* node, void* opt);

void* resolveParamDec(const ParseTNode* node, void* opt);

void* resolveVarList(const ParseTNode* node, void* opt);

void* resolveFunDec(const ParseTNode* node, void* opt);

void* resolveVarDec(const ParseTNode* node, void* opt);

void* resolveTag(const ParseTNode* node, void* opt);

void* resolveOptTag(const ParseTNode* node, void* opt);

void* resolveStructSpecifier(const ParseTNode* node, void* opt);

void* resolveSpecifier(const ParseTNode* node, void* opt);

void* resolveExtDecList(const ParseTNode* node, void* opt);

void* resolveExtDef(const ParseTNode* node, void* opt);

void* resolveExtDefList(const ParseTNode* node, void* opt);

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
