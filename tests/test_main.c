/**
 * @file    test_main.c
 * @author  xhorakj01
 * @brief   Entry point for unit tests, runs all test suites
 */

#include "test_ast.h"
#include "test_precedence.h"
#include "test_scanner.h"
#include "test_stack.h"
#include "test_parser.h"

int main(void) {
  first_iteration();
  indentation_flow_tests();
  test_stack();
  test_ast();
  test_precedence();
  test_parser();
  return 0;
}
