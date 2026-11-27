/**
 * IFJ26 compiler
 *
 * @file    codegen.h
 * @author  xoramst00
 * @brief   Generator definition of final code IFJcode26
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/**
 * @brief Generates IFJcode26 instructions for the complete AST program.
 * @param root Root AST node of type AST_PROGRAM.
 */
void cg_gen_program(const AstNode *root);

#endif // CODEGEN_H
