/**
 * @file    semantic.h
 * @author  xsirmat00
 * @brief   Header file for semantic analysis
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

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
  ERR_SEM_FUNCTION_PARAMS = 5,  /**< Invalid param or return type count/type */
  ERR_SEM_WRONG_TYPE = 6,       /**< Type incompatibility */
  ERR_SEM_IMMUTABLE = 7,        /**< Write to val or parameter */
  ERR_SEM_OTHER = 10,           /**< Other semantic error */
  ERR_INTERNAL = 99             /**< Internal error such as malloc failure */
} SemanticError;

/**
 * @brief Performs complete semantic analysis over the parsed AST program
 * @param Symtable *st
 * @param AstNode *root
 * @returns SemanticError status code
 */
SemanticError analyze_semantics(Symtable *st, AstNode *root);

#endif // SEMANTIC_H