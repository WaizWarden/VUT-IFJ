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

/**
 * @brief Returns a new, unique label ID.
 * @return The allocated label ID.
 */
uint32_t cg_next_label_id(void);

/**
 * @brief Prints a 64-bit integer in IFJcode26 format.
 * @param val Integer value to print.
 */
void cg_print_int(int64_t val);

/**
 * @brief Prints a floating-point value in IFJcode26 format.
 * @param val Floating-point value to print.
 */
void cg_print_double(double val);

/**
 * @brief Prints a Boolean value in IFJcode26 format.
 * @param val Boolean value to print.
 */
void cg_print_bool(bool val);

/**
 * @brief Prints the Unit representation used by IFJcode26.
 */
void cg_print_unit(void);

/**
 * @brief Prints a string value, escaping characters that require encoding.
 * @param str Source string to print.
 */
void cg_print_string(const char *str);

/**
 * @brief Prints a local variable using its UID as a namespace.
 * @param name Source-level variable name.
 * @param uid Semantic UID assigned to the variable.
 */
void cg_print_var(const char *name, uint32_t uid);

/**
 * @brief Emits the program header and built-in function implementations.
 */
void cg_gen_head(void);

/**
 * @brief Generates IFJcode26 instructions for the complete AST program.
 * @param root Root AST node of type AST_PROGRAM.
 */
void cg_gen_program(const AstNode *root);

/**
 * @brief Generates instructions for one AST statement.
 * @param stmt Statement AST node.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_statement(const AstNode *stmt, DataType funcRetType);

/**
 * @brief Generates instructions for a block or non-block AST body.
 * @param block Block AST node, or a single statement.
 * @param isLastInFunc Whether this block supplies the function return value.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_block(const AstNode *block, bool isLastInFunc,
                  DataType funcRetType);

/**
 * @brief Generates instructions for a CONTROL for loop.
 * @param forNode AST node of type AST_FOR.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_for(const AstNode *forNode, DataType funcRetType);

/**
 * @brief Generates instructions for an expression and leaves its value on the
 * data stack.
 * @param expr Expression AST node.
 */
void cg_gen_expr(const AstNode *expr);

/**
 * @brief Generates instructions for an if statement or if expression.
 * @param ifNode AST node of type AST_IF.
 * @param isExprOrReturn True if a branch must leave a value on the stack.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_if(const AstNode *ifNode, bool isExprOrReturn,
               DataType funcRetType);

/**
 * @brief Generates instructions for a while loop.
 * @param whileNode AST node of type AST_WHILE.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_while(const AstNode *whileNode, DataType funcRetType);

/**
 * @brief Generates instructions for a built-in or user-defined function call.
 * @param callNode AST node of type AST_FUNC_CALL.
 */
void cg_gen_call(const AstNode *callNode);

#endif // CODEGEN_H
