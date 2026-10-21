/**
 * @file    parser.h
 * @author  xhorakj01
 * @brief   Parser header file.
 */

#ifndef PARSER_H
#define PARSER_H

#include "precedence.h"

AstNode *parser_parse_assignment(FILE *input, Token *currentToken);

#endif
