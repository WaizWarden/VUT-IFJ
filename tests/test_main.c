/**
 * @file    test_main.c
 * @author  xhorakj01
 * @brief   Entry point for unit tests, runs all test suites
 */

#include "test_ast.h"
#include "test_parser.h"
#include "test_precedence.h"
#include "test_scanner.h"
#include "test_semantic.h"
#include "test_stack.h"

#include <string.h>

int main(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "--parser-valid") == 0) {
    test_parser_valid();
    return 0;
  }
  first_iteration();
  indentation_flow_tests();
  test_stack();
  test_ast();
  test_precedence();
  test_parser();
  test_semantic();
  return 0;
}
