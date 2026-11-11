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
  ERR_SEM_WRONG_TYPE = 4,       /**< Type incompatibility */
  ERR_SEM_REDEFINITION = 5,     /**< Redefinition error */
  ERR_SEM_FUNCTION_PARAMS = 6,  /**< Invalid param or return type count/type */
  ERR_SEM_TYPE_INFERENCE =
      7, /**< Type incompatibility in expression/statement context */
  ERR_SEM_OTHER = 10, /**< Other semantic error */
} SemanticError;

/**
 * @brief Performs complete semantic analysis over the parsed AST program
 * @param symtable Global symbol table
 * @param ast Root node of AST or subtree
 * @return SemanticError status code
 */
SemanticError analyze_semantics(Symtable *symtable, AstNode *ast);

#endif // SEMANTIC_H