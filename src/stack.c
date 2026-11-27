/**
 * IFJ26 compiler
 *
 * @file    stack.c
 * @author  xhorakj01
 * @brief   Stack using dynamic array
 */

#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

/**
 * @brief Inicialization of the dynamic array
 * @param stack Stack instance to initialize
 * @returns Pointer to allocated memory.
 */

void stack_init(Stack *stack) {
  stack->size = 0;
  stack->capacity = 8;
  stack->items = malloc(stack->capacity * sizeof(StackItem));

  if (stack->items == NULL) {
    exit(INTERPRET_ERROR);
  }
}

void stack_push(Stack *stack, StackItem item) {
  if (stack->size == stack->capacity) {
    stack->capacity *= 2;
    StackItem *newItems =
        realloc(stack->items, stack->capacity * sizeof(StackItem));

    if (!newItems) {
      exit(INTERPRET_ERROR);
    }
    stack->items = newItems;
  }
  stack->items[stack->size++] = item;
}

StackItem stack_pop(Stack *stack) {
  if (stack->size == 0) {
    exit(INTERPRET_ERROR);
  }
  return stack->items[--stack->size];
}

StackItem *stack_top(Stack *stack) {
  if (stack->size == 0) {
    exit(INTERPRET_ERROR);
  }
  return &stack->items[stack->size - 1];
}

/*
 * An Gemini LLM was used for advice on implementation of the further function
 * Citation of LLM: "Algoritmus se ptá tabulky na a  = terminál nejblíže
 * vrcholu. Na zásobníku ale může být navrchu neterminál  E  (stav třeba  $E+<E
 * ), takže musíš sestoupit od vrcholu dolů a přeskočit  SYM_NONTERM"
 */
StackItem *stack_top_terminal(Stack *stack) {
  for (size_t i = stack->size; i > 0; i--) {
    if (stack->items[i - 1].symbol != SYM_NONTERM &&
        stack->items[i - 1].symbol != SYM_HANDLE_START) {
      return &stack->items[i - 1];
    }
  }
  exit(INTERPRET_ERROR); // $ je vždy dole, nemůže nastat
}

/*
 * Devin AI pointed out that the handle-start marker must be inserted
 * immediately after the topmost terminal, not necessarily at the top
 *     * of the stack.
 */

/**
 * @brief the handle-start marker must be inserted immediately after the topmost
 * terminal
 */
void stack_insert_handle_start(Stack *stack) {
  // Going through the stack backwards
  size_t terminalIndex = stack->size;
  for (size_t i = stack->size; i > 0; i--) {
    size_t index = i - 1;

    // Deciding whenether the "items[index]" is terminal
    if (stack->items[index].symbol != SYM_NONTERM &&
        stack->items[index].symbol != SYM_HANDLE_START) {
      terminalIndex = index;
      break;
    }
  }
  // End of colaboration with devin

  if (terminalIndex == stack->size) {
    exit(INTERPRET_ERROR);
  }

  size_t insertIndex = terminalIndex + 1;

  if (stack->size == stack->capacity) {
    stack->capacity *= 2;
    StackItem *newItems =
        realloc(stack->items, stack->capacity * sizeof(StackItem));

    if (!newItems) {
      exit(INTERPRET_ERROR);
    }
    stack->items = newItems;
  }

  for (size_t i = stack->size; i > insertIndex; i--) {
    stack->items[i] = stack->items[i - 1];
  }

  stack->items[insertIndex].symbol = SYM_HANDLE_START;
  stack->items[insertIndex].token = TOKEN_EOF;
  stack->items[insertIndex].node = NULL;

  stack->size++;
}

void stack_free(Stack *stack) {
  free(stack->items);
  stack->items = NULL;
  stack->size = 0;
  stack->capacity = 0;
}

size_t stack_size(Stack *stack) { return stack->size; }
