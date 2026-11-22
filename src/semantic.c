/**
 * @file    semantic.c
 * @author  xjuricl01, xoramst00
 * @brief   Implementation of semantic analysis
 */

#include "semantic.h"
#include "ast.h"
#include "symtable.h"

static SemanticError analyze_functions(Symtable *symtable, AstNode *root);
static SemanticError register_builtins(Symtable *symtable);

static int scope = 0;

typedef struct {
  const char *name;
  DataType retType;
  Parameter *params;
  uint16_t paramCount;
} Builtin;

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (root != AST_PROGRAM) {
    return ERR_OTHER;
  }

  analyze_functions(symtable, root);
  return ERR_OK;
}

/**
 * @brief Helper function that analyzes all functions
 * and checks valid main function
 *
 */
static SemanticError analyze_functions(Symtable *symtable, AstNode *root) {

  if (register_builtins(symtable) != ERR_OK) {
    return ERR_OTHER;
  }

  if (!st_push_scope(symtable)) {
    return ERR_OTHER;
  }
  // global functions are at scope 0
  scope++;

  // loop through functions
  int mainCount = 0;
  for (size_t i = 0; i < root->as.program.count; i++) {
    AstNode *functionNode = root->as.program.functions[i];

    if (functionNode->type != AST_FUNC_DEF) {
      return ERR_OTHER;
    }

    // validate main function
    if (functionNode->as.funcDef.isMain) {

      if (functionNode->as.funcDef.paramCount > 0) {
        return ERR_INVALID_FUNCTION_PARAMS;
      }

      if (functionNode->as.funcDef.retType != T_UNIT) {
        return ERR_WRONG_TYPE;
      }

      if (mainCount != 0) {
        return ERR_REDEFINITION;
      }

      mainCount++;
    }

    if (mainCount == 0) {
      return ERR_UNDEFINED_SYMBOL;
    }
  }

  return ERR_OK;
}

/**
 * @brief Helper function that registers all built in functions from
 * asssignement
 *
 */
static SemanticError register_builtins(Symtable *symtable) {

  static Parameter anyParam[] = {{"value", T_ANY, 0}};
  static Parameter doubleParam[] = {{"value", T_DOUBLE, 0}};
  static Parameter stringParam[] = {{"value", T_STRING, 0}};
  static Parameter intParam[] = {{"code", T_INT, 0}};

  static Parameter subStrParams[] = {
      {"value", T_STRING, 0}, {"start", T_INT, 0}, {"end", T_INT, 0}};
  static Parameter ordParams[] = {{"value", T_STRING, 0}, {"index", T_INT, 0}};

  static Builtin builtins[] = {
      {"print", T_UNIT, anyParam, 1},
      {"readLine", T_STRING, NULL, 0},
      {"readInt", T_INT, NULL, 0},
      {"readDouble", T_DOUBLE, NULL, 0},
      {"toStr", T_STRING, anyParam, 1},
      {"toInt", T_INT, doubleParam, 1},
      {"strLength", T_INT, stringParam, 1},
      {"subStr", T_STRING, subStrParams, 3},
      {"ord", T_INT, ordParams, 2},
      {"chr", T_STRING, intParam, 1},
  };

  int builtinsSize = sizeof(builtins) / sizeof(builtins[0]);

  for (int i = 0; i < builtinsSize; i++) {
    Builtin *function = &builtins[i];

    if (!st_insert_func(symtable, function->name, function->retType,
                        function->params, function->paramCount, false, true)) {
      return ERR_INTERNAL;
    }
  }

  return ERR_OK;
}