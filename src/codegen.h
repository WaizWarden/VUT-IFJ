/**
 * @file    codegen.h
 * @author  xoramst00
 * @brief   Generator definition of final code IFJcode26
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symtable.h"
#include <stdint.h>

/// @brief Increments global id counter
/// @return unique label id
uint32_t cg_next_label_id(void);

/// @brief print 64bit int intercode
/// @param x intiger to print
void cg_print_int(int64_t val);

void cg_print_double(double val);

void cg_print_bool(bool val);

void cg_print_unit(void);

void cg_print_string(const char *str);

/// @brief Prints the variable name in intercode format
/// @param name var name
/// @param uid unique id
void cg_print_var(const char *name, uint32_t uid);

/// @brief Generates header and builtin functions
void cg_gen_head(void);

/// @todo add function descriptions -----

/// @brief Generates the whole program to stdout
/// @param root program ast
void cg_gen_program(const AstNode *root);

/// @brief Generates a single line of original code to intercode
/// @param stmt statement
/// @param funcRetType return type
void cg_gen_statement(const AstNode *stmt, DataType funcRetType);

/// @brief Generates a block of code, like functions
/// @param block ast tree of block
/// @param isLastInFunc true if the block should leave a return value
/// @param funcRetType return type
void cg_gen_block(const AstNode *block, bool isLastInFunc,
                  DataType funcRetType);

/// @author xsirmat00 from here

/**
 * @brief Generates IFJcode26 instructions for an expression and pushes the
 * result onto the data stack
 *
 * @param expr AST node representing the expression
 */
void cg_gen_expr(const AstNode *expr);

/**
 * @brief Generates IFJcode26 instructions for an if-else statement or
 * expression
 *
 * @param ifNode AST node of type AST_IF
 * @param isExprOrReturn True if the evaluated branch value must remain on the
 * stack
 * @param funcRetType Return type of the enclosing function
 */
void cg_gen_if(const AstNode *ifNode, bool isExprOrReturn,
               DataType funcRetType);

/**
 * @brief Generates IFJcode26 instructions for a while loop
 *
 * @param whileNode AST node of type AST_WHILE
 * @param funcRetType Return type of the enclosing function
 */
void cg_gen_while(const AstNode *whileNode, DataType funcRetType);

/**
 * @brief Generates IFJcode26 instructions for built-in and user-defined
 * function calls
 *
 * @param callNode AST node of type AST_FUNC_CALL
 */
void cg_gen_call(const AstNode *callNode);

#endif // CODEGEN_H
