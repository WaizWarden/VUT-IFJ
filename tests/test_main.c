/**
 * @file    test_main.c
 * @author  xhorakj01
 * @brief   Entry point for unit tests, runs all test suites
 */

#include "test_scanner.h"
#include "test_stack.h"

int main(void) {
  first_iteration();
  indentation_flow_tests();
  test_stack();
  return 0;
}
