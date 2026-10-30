/**
 * @file    scanner_stack.h
 * @author  xhorakj01, xjuricl01
 * @brief   scanenr_stack header file
 */

#include <stdlib.h>

void scanner_stack_init();
void scanner_stack_push(int item);
int scanner_stack_pop(void);
int *scanner_stack_top(void);
void scanner_stack_free(void);
size_t scanner_stack_size(void);