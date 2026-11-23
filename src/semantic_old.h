/**
 * @file    semantic.h
 * @author  xsirmat00
 * @brief   Header file for semantic analysis
 */

#ifndef SEMANTIC_H_OLD
#define SEMANTIC_H_OLD

#include "ast.h"
#include "symtable.h"
#include <stdbool.h>

/**
 * @brief Semantic error codes according to IFJcode26 specification
 */
typedef enum {
  ERR_SEM_OK = 0,               /**< OK */
  ERR_SEM_UNDEFINED_SYMBOL = 3, /**< Undefined function or variable */
  ERR_SEM_REDEFINITION = 4,     /**< Redefinition error */
  ERR_SEM_FUNCTION_PARAMS = 5,  /**< Invalid param count/type in call */
  ERR_SEM_WRONG_TYPE = 6,       /**< Type incompatibility */
  ERR_SEM_IMMUTABLE = 7,        /**< Write to val or parameter */
  ERR_SEM_OTHER = 10,           /**< Other semantic error */
  ERR_INTERNAL = 99             /**< Internal error (malloc, ...) */
} SemanticError;

/**
 * @brief Performs complete semantic analysis over the parsed AST program
 *
 * @param symtable Initialized global symbol table
 * @param root Root node of the AST (must be AST_PROGRAM)
 * @return SemanticError status code
 */
SemanticError analyze_semantics(Symtable *symtable, AstNode *root);

#endif  SEMANTIC_H_OLD
