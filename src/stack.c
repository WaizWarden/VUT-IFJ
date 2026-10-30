/**
 * @file    stack.c
 * @author  xhorakj01
 * @brief   Stack using dynamic array
 */

#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

static StackItem *items = NULL; // pole
static size_t size = 0;
static size_t capacity = 0;

/**
 * @brief Inicialization of the dynamic array
 * @param AstNodeType type
 * @returns Pointer to allocated memory.
 */

void stack_init() {
  size = 0;
  capacity = 8;
  items = malloc(capacity * sizeof(StackItem));

  if (items == NULL) {
    exit(INTERPRET_ERROR);
  }
}

void stack_push(StackItem item) {
  if (size == capacity) {
    capacity *= 2;
    StackItem *newItems = realloc(items, capacity * sizeof(StackItem));

    if (!newItems) {
      exit(INTERPRET_ERROR);
    }
    items = newItems;
  }
  items[size++] = item;
}

StackItem stack_pop(void) {
  if (size == 0) {
    exit(INTERPRET_ERROR);
  }
  return items[--size];
}

StackItem *stack_top(void) {
  if (size == 0) {
    exit(INTERPRET_ERROR);
  }
  return &items[size - 1];
}

/*  Bylo použito Gemini LLM pro radu s implementací následující funkce.
 *  Citace LLM: "Algoritmus se ptá tabulky na a  = terminál nejblíže vrcholu. Na
 zásobníku ale může být navrchu neterminál  E  (stav třeba  $E+<E ), takže musíš
 sestoupit od vrcholu dolů a přeskočit  SYM_NONTERM"
 */
StackItem *stack_top_terminal(void) {
  for (size_t i = size; i > 0; i--) {
    if (items[i - 1].symbol != SYM_NONTERM &&
        items[i - 1].symbol != SYM_HANDLE_START) {
      return &items[i - 1];
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
void stack_insert_handle_start(void) {
  // Going through the stack backwards
  size_t terminalIndex = size;
  for (size_t i = size; i > 0; i--) {
    size_t index = i - 1;

    // Deciding whenether the "items[index]" is terminal
    if (items[index].symbol != SYM_NONTERM &&
        items[index].symbol != SYM_HANDLE_START) {
      terminalIndex = index;
      break;
    }
  }
  // End of colaboration with devin

  if (terminalIndex == size) {
    exit(INTERPRET_ERROR);
  }

  size_t insertIndex = terminalIndex + 1;

  if (size == capacity) {
    capacity *= 2;
    StackItem *newItems = realloc(items, capacity * sizeof(StackItem));

    if (!newItems) {
      exit(INTERPRET_ERROR);
    }
    items = newItems;
  }

  for (size_t i = size; i > insertIndex; i--) {
    items[i] = items[i - 1];
  }

  items[insertIndex].symbol = SYM_HANDLE_START;
  items[insertIndex].token = TOKEN_EOF;
  items[insertIndex].node = NULL;

  size++;
}

void stack_free(void) {
  free(items);
  items = NULL;
  size = 0;
  capacity = 0;
}

size_t stack_size(void) { return size; }
