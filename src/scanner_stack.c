/**
 * @file    scanner_stack.c
 * @author  xjuricl01
 * @brief   Stack using dynamic array used for indent and dedent
 */

#include "scanner_stack.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

/**
 * @brief Inicialization of the dynamic array
 * @param stack Target stack
 */
void scanner_stack_init(ScannerStack *stack) {
  stack->size = 0;
  stack->capacity = 8;
  stack->items = malloc(stack->capacity * sizeof(int));

  if (stack->items == NULL) {
    exit(INTERPRET_ERROR);
  }
}

/**
 * @brief Pushes item to stack
 * @param stack Target stack
 * @param Item you want to add on stack
 */
void scanner_stack_push(ScannerStack *stack, int item) {
  if (stack->size == stack->capacity) {
    stack->capacity *= 2;
    int *newItems = realloc(stack->items, stack->capacity * sizeof(int));

    if (!newItems) {
      exit(INTERPRET_ERROR);
    }
    stack->items = newItems;
  }
  stack->items[stack->size++] = item;
}

/**
 * @brief Returns item from top of stack and removes value
 * @param stack Target stack
 * @param item stored on top of stack
 */
int scanner_stack_pop(ScannerStack *stack) {
  if (stack->size == 0) {
    exit(INTERPRET_ERROR);
  }
  return stack->items[--stack->size];
}

/**
 * @brief Returns top
 * @param stack Target stack
 */
int *scanner_stack_top(ScannerStack *stack) {
  if (stack->size == 0) {
    exit(INTERPRET_ERROR);
  }
  return &stack->items[stack->size - 1];
}

/**
 * @brief Free items in stack
 * @param stack Target stack
 */
void scanner_stack_free(ScannerStack *stack) {
  free(stack->items);
  stack->items = NULL;
  stack->size = 0;
  stack->capacity = 0;
}

size_t scanner_stack_size(ScannerStack *stack) { return stack->size; }

/**
 * @brief Function used to search for number in stack
 * @param stack Target stack
 * @param searched number
 * @returns numbex index
 */
int find_number_in_stack(ScannerStack *stack, int searchedNumber) {
  int current_size = (int)scanner_stack_size(stack);
  int found = -1;
  for (int i = current_size - 1; i >= 0; i--) {
    if (stack->items[i] == searchedNumber) {
      found = i;
    }
  }
  return found;
}
