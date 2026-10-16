/**
    * @file    stack.h
    * @author  xhorakj01
    * @brief   Stack header file.
    */

#ifndef STACK_H
#define STACK_H

#include "scanner.h"
#include "ast.h"

typedef enum {
       SYM_PLUS_MINUS, SYM_MUL_DIV, SYM_LPAREN, SYM_RPAREN,
       SYM_TERM, SYM_DOLLAR, SYM_NONTERM, SYM_HANDLE_START
} PrecSymbol;

typedef struct {
    PrecSymbol symbol;
    TokenType token;
    AstNode *node;
} StackItem;

void stack_init(void);
void stack_push(StackItem item);
StackItem stack_pop(void);
StackItem *stack_top(void);
StackItem *stack_top_terminal(void);
void stack_free(void);
void stack_insert_handle_start(void);

#endif
