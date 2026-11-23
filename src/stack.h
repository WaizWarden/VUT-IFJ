/**
 * @file    stack.h
 * @author  xhorakj01
 * @brief   Stack header file.
 */

#ifndef STACK_H
#define STACK_H

#include "ast.h"
#include "scanner.h"
#include <stddef.h>

typedef enum {
  SYM_PLUS_MINUS, // + -
  SYM_MUL_DIV,    // * /
  SYM_UMINUS,     // unární -
  SYM_NOT,        // unární ! - negace
  SYM_REL,        // < <= > >=
  SYM_EQ_NEQ,     // == !=
  SYM_AND,        // &&
  SYM_OR,         // ||
  SYM_DOT,        // . member access
  SYM_LPAREN,
  SYM_RPAREN,
  SYM_TERM,
  SYM_DOLLAR,
  SYM_NONTERM,
  SYM_HANDLE_START
} PrecSymbol;

typedef struct {
  PrecSymbol symbol;
  TokenType token;
  AstNode *node;
} StackItem;

typedef struct {
  StackItem *items;
  size_t size;
  size_t capacity;
} Stack;

void stack_init(Stack *stack);
void stack_push(Stack *stack, StackItem item);
StackItem stack_pop(Stack *stack);
StackItem *stack_top(Stack *stack);
StackItem *stack_top_terminal(Stack *stack);
void stack_free(Stack *stack);
void stack_insert_handle_start(Stack *stack);
size_t stack_size(Stack *stack);
#endif
