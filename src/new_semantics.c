/**
 * @file    new_semantics.c
 * @author  xjuricl01, xoramst00
 * @brief   Reimplementation of semantic analysis
 */

#include "new_semantics.h"
#include "ast.h"
#include "symtable.h"

static SemanticError analyze_functions(Symtable *symtable, AstNode *root);
static SemanticError register_builtins(Symtable *symtable);

static int scope = 0;

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (root != AST_PROGRAM) {
    return ERR_OTHER;
  }

  analyze_functions(symtable, root);
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
  for (int i = 0; i < root->as.program.count; i++) {
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
static SemanticError register_builtins(Symtable *symtable) { return ERR_OK; }