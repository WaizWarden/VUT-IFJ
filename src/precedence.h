/**
 * @file    ast.h
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
AstNode *precedence_parse(FILE *input, Token *currentTk);
#endif
