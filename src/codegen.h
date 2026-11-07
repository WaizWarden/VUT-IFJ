/**
 * @file    codegen.h
 * @author  xoramst00
 * @brief   Generator definition of final code IFJcode26
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stdint.h>
#include "ast.h"
#include "symtable.h"

/// @brief Increments global id counter
/// @return unique label id
uint32_t cg_next_label_id();

/// @brief print 64bit int to stdout
/// @param x intiger to print
void cg_print_int(int64_t val);

void cg_print_double(double val);

void cg_print_bool(bool val);

void cg_print_unit(void);

void cg_print_string(char *str);

void cg_print_var(const char *name, uint32_t uid);

/// @brief Generates header and builtin functions 
void cg_gen_head();

/// @todo add function descriptions -----

void cg_gen_program(const AstNode *root);

void cg_gen_statement(const AstNode *stmt, DataType funcRetType);

void cg_gen_block(const AstNode *block, bool isLastInFunc, DataType funcRetType);


/// @author xsirmat00 from here
/// @todo add function descriptions -----

void cg_gen_expr(const AstNode *expr);

void cg_gen_if(const AstNode *ifNode, bool isExprOrReturn, DataType funcRetType);

void cg_gen_while(const AstNode *whileNode, DataType funcRetType);

void cg_gen_call(const AstNode *callNode);

#endif