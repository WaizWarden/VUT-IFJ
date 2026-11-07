/**
 * @file    semantic.h
 * @author  xsirmat00
 * @brief   Rozhraní pro sémantickou analýzu a typovou kontrolu.
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symtable.h"
#include <stdbool.h>

typedef enum {
  ERR_SEM_OK = 0,
  ERR_SEM_UNDEFINED = 3,
  ERR_SEM_REDEFINITION = 4,
  ERR_SEM_TYPE_MISMATCH = 5,
  ERR_SEM_FUNC_ARGS = 6,
  ERR_SEM_OTHER = 7,
} SemanticError;

/**
 * @brief Vyhodnotí datový typ výrazu reprezentovaného uzlem AST.
 */
DataType analyze_expression(Symtable *symtable, const AstNode *node,
                            SemanticError *err);

/**
 * @brief Prochází a provádí sémantické kontroly na podstromu AST.
 */
SemanticError analyze_semantics(Symtable *symtable, const AstNode *node);

#endif