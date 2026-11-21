/**
 * @file    new_semantics.h
 * @author  xjuricl01
 * @brief   Reimplementation of semantic analysis
 */

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
  ERR_INTERNAL = 99
} SemanticError;

SemanticError analyze_semantics(Symtable *symtable, AstNode *root);