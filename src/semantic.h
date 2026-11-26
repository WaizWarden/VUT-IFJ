/**
 * @file    semantic.h
 * @author  xjuricl01
 * @brief   Implementation of semantic analysis
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"

typedef enum {
  ERR_OK = 0,
  ERR_UNDEFINED_SYMBOL = 3,
  ERR_REDEFINITION = 4,
  ERR_INVALID_FUNCTION_PARAMS = 5,
  ERR_WRONG_TYPE = 6,
  ERR_IMMUTABLE = 7,
  ERR_OTHER = 10,
  ERR_INTERNAL = 99,
} SemanticError;

/// @brief Analyzes all nodes of program AST tree and annotates every node with
/// necesary types and uid if needed.
/// @param symtable Symbol table
/// @param root AST root node of program
/// @return ERR_OK if there is no error encountered or apropriate error code
SemanticError analyze_semantics(Symtable *symtable, AstNode *root);

#endif 
