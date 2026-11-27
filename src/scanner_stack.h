/**
 * IFJ26 compiler
 *
 * @file    scanner_stack.h
 * @author  xjuricl01
 * @brief   scanenr_stack header file
 */

#ifndef SCANNER_STACK_H
#define SCANNER_STACK_H

#include <stdlib.h>

typedef struct {
  int *items;
  size_t size;
  size_t capacity;
} ScannerStack;

void scanner_stack_init(ScannerStack *stack);
void scanner_stack_push(ScannerStack *stack, int item);
int scanner_stack_pop(ScannerStack *stack);
int *scanner_stack_top(ScannerStack *stack);
void scanner_stack_free(ScannerStack *stack);
size_t scanner_stack_size(ScannerStack *stack);
int find_number_in_stack(ScannerStack *stack, int searchedNumber);

#endif
