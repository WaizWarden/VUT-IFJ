/**
 * @file    test_semantic.c
 * @author  xsirmat00
 * @brief   Independent unit tests for the semantic analyzer
 */

#include "test_semantic.h"
#include "../src/ast.h"
#include "../src/semantic.h"
#include "../src/symtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_INT_VAL 42
#define TEST_INT_VAL_2 10
#define TEST_INT_VAL_3 5
#define TEST_INT_VAL_4 100
#define TEST_DOUBLE_VAL 3.14

static AstNode *wrap_in_main_program(AstNode *stmt) {
  AstNode **stmts = malloc(sizeof(AstNode *));
  assert(stmts != NULL);
  stmts[0] = stmt;

  AstNode *body = ast_new_block(stmts, 1, 1);
  AstNode *mainFunc =
      ast_new_func_def(strdup("main"), true, T_UNIT, NULL, 0, body, 1);

  AstNode **funcs = malloc(sizeof(AstNode *));
  assert(funcs != NULL);
  funcs[0] = mainFunc;

  return ast_new_program(funcs, 1, 1);
}

static void test_valid_assignment(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_insert_var(&symtable, "x", T_INT, true) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("x"), val, 1);
  AstNode *program = wrap_in_main_program(assign);

  SemanticError err = analyze_semantics(&symtable, program);
  assert(err == ERR_SEM_OK);

  ast_free(program);
  st_free(&symtable);
  printf("Test: Semantically valid assignment Passed\n");
}

static void test_undefined_variable(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_2, 1);
  AstNode *assign = ast_new_assign(strdup("y"), val, 1);
  AstNode *program = wrap_in_main_program(assign);

  SemanticError err = analyze_semantics(&symtable, program);
  assert(err == ERR_SEM_UNDEFINED_SYMBOL);

  ast_free(program);
  st_free(&symtable);
  printf("Test: Undefined variable Passed\n");
}

static void test_type_mismatch(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_insert_var(&symtable, "num", T_INT, true) == true);

  AstNode *val = ast_new_double(TEST_DOUBLE_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("num"), val, 1);
  AstNode *program = wrap_in_main_program(assign);

  SemanticError err = analyze_semantics(&symtable, program);
  assert(err == ERR_SEM_WRONG_TYPE);

  ast_free(program);
  st_free(&symtable);
  printf("Test: Type mismatch Passed\n");
}

static void test_int_to_double_implicit_conversion(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_insert_var(&symtable, "d", T_DOUBLE, true) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_3, 1);
  AstNode *assign = ast_new_assign(strdup("d"), val, 1);
  AstNode *program = wrap_in_main_program(assign);

  SemanticError err = analyze_semantics(&symtable, program);
  assert(err == ERR_SEM_OK);

  ast_free(program);
  st_free(&symtable);
  printf("Test: Implicit conversion Int to Double Passed\n");
}

static void test_readonly_variable_assign(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_insert_var(&symtable, "constVal", T_INT, false) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_4, 1);
  AstNode *assign = ast_new_assign(strdup("constVal"), val, 1);
  AstNode *program = wrap_in_main_program(assign);

  SemanticError err = analyze_semantics(&symtable, program);
  assert(err == ERR_SEM_OTHER);

  ast_free(program);
  st_free(&symtable);
  printf("Test: Assignment to constant (val) Passed\n");
}

void test_semantic(void) {
  printf("\n--- SEMANTIC TESTS ---\n");
  test_valid_assignment();
  test_undefined_variable();
  test_type_mismatch();
  test_int_to_double_implicit_conversion();
  test_readonly_variable_assign();
  printf("Semantic tests completed\n");
}
