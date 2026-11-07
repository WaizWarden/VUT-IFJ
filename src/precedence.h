/**
 * @file    precedence.h
 * @author  xhorakj01
 * @brief   Precedence header file.
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "stack.h"

typedef enum {
  PRECEDENCE_LESS,
  PRECEDENCE_EQUAL,
  PRECEDENCE_GREATER,
  PRECEDENCE_EMPTY
} PrecedenceRelation;
/**
 * @param stackSymbol highest terminal on stack
 * @param inputSymbol actual input terminal
 * @returns precedent relation
 */
PrecedenceRelation precedence_get_relation(PrecSymbol stackSymbol,
                                           PrecSymbol inputSymbol);
/**
 * @brief Parses an expression using precedence analysis.
 *
 * @param input        Input stream for the scanner
 * @param currentToken In: first token of the expression (or the token
 *                     following firstOperand). Out: the token that
 *                     terminated the expression.
 * @param firstOperand Already parsed leading operand (identifier read by
 *                     the parser during lookahead), or NULL. Ownership
 *                     is transferred to this function.
 * @returns AST of the expression, or NULL on syntax error.
 */
AstNode *precedence_parse(FILE *input, Token *currentToken,
                          AstNode *firstOperand);
#endif
