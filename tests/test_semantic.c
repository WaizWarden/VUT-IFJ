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

static SemanticError analyze_main_body(AstNode **statements, size_t count) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  AstNode **owned_statements = malloc(count * sizeof(*owned_statements));
  assert(owned_statements != NULL);
  memcpy(owned_statements, statements, count * sizeof(*owned_statements));

  AstNode *body = ast_new_block(owned_statements, count, 1);
  AstNode *main =
      ast_new_func_def(strdup("main"), true, T_UNIT, true, NULL, 0, body, 1);
  AstNode **functions = malloc(sizeof(*functions));
  assert(functions != NULL);
  functions[0] = main;
  AstNode *program = ast_new_program(functions, 1, 1);

  SemanticError err = analyze_semantics(&symtable, program);
  ast_free(program);
  st_free(&symtable);
  return err;
}

static SemanticError analyze_with_helper(AstNode *helper) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  AstNode *mainBody = ast_new_block(NULL, 0, 1);
  AstNode *main =
      ast_new_func_def(strdup("main"), true, T_UNIT, true, NULL, 0, mainBody, 1);
  AstNode **functions = malloc(2 * sizeof(*functions));
  assert(functions != NULL);
  functions[0] = helper;
  functions[1] = main;
  AstNode *program = ast_new_program(functions, 2, 1);

  SemanticError err = analyze_semantics(&symtable, program);
  ast_free(program);
  st_free(&symtable);
  return err;
}

static void test_if_without_else_in_explicit_unit_function(void) {
  AstNode *thenExpr = ast_new_expr_statement(ast_new_int(TEST_INT_VAL, 1), 1);
  AstNode *conditional = ast_new_if(ast_new_bool(true, 1), thenExpr, NULL, 1);
  AstNode *helper = ast_new_func_def(strdup("helper"), false, T_UNIT, true,
                                     NULL, 0, conditional, 1);

  assert(analyze_with_helper(helper) == ERR_OK);
  printf("Test: If without else in explicit Unit function Passed\n");
}

static void test_if_without_else_inferred_unit_function(void) {
  AstNode *thenExpr = ast_new_expr_statement(ast_new_unit(1), 1);
  AstNode *conditional = ast_new_if(ast_new_bool(true, 1), thenExpr, NULL, 1);
  AstNode *helper = ast_new_func_def(strdup("helper"), false, T_UNIT, false,
                                     NULL, 0, conditional, 1);

  assert(analyze_with_helper(helper) == ERR_OK);
  printf("Test: Unit inference for if without else Passed\n");
}

static void test_if_without_else_non_unit_inferred_function(void) {
  AstNode *thenExpr = ast_new_expr_statement(ast_new_int(TEST_INT_VAL, 1), 1);
  AstNode *conditional = ast_new_if(ast_new_bool(true, 1), thenExpr, NULL, 1);
  AstNode *helper = ast_new_func_def(strdup("helper"), false, T_UNIT, false,
                                     NULL, 0, conditional, 1);

  assert(analyze_with_helper(helper) == ERR_WRONG_TYPE);
  printf("Test: Non-Unit if without else cannot be inferred Passed\n");
}

static void test_valid_assignment(void) {
  AstNode *decl = ast_new_var_decl(strdup("x"), true, T_INT, true, NULL, 1);
  AstNode *val = ast_new_int(TEST_INT_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("x"), val, 1);
  AstNode *statements[] = {decl, assign};

  SemanticError err = analyze_main_body(statements, 2);
  assert(err == ERR_OK);

  printf("Test: Semantically valid assignment Passed\n");
}

static void test_undefined_variable(void) {
  AstNode *val = ast_new_int(TEST_INT_VAL_2, 1);
  AstNode *assign = ast_new_assign(strdup("y"), val, 1);
  AstNode *statements[] = {assign};

  SemanticError err = analyze_main_body(statements, 1);
  assert(err == ERR_UNDEFINED_SYMBOL);

  printf("Test: Undefined variable Passed\n");
}

static void test_type_mismatch(void) {
  AstNode *decl = ast_new_var_decl(strdup("num"), true, T_INT, true, NULL, 1);
  AstNode *val = ast_new_double(TEST_DOUBLE_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("num"), val, 1);
  AstNode *statements[] = {decl, assign};

  SemanticError err = analyze_main_body(statements, 2);
  assert(err == ERR_WRONG_TYPE);

  printf("Test: Type mismatch Passed\n");
}

static void test_int_to_double_implicit_conversion(void) {
  AstNode *decl = ast_new_var_decl(strdup("d"), true, T_DOUBLE, true, NULL, 1);
  AstNode *val = ast_new_int(TEST_INT_VAL_3, 1);
  AstNode *assign = ast_new_assign(strdup("d"), val, 1);
  AstNode *statements[] = {decl, assign};

  SemanticError err = analyze_main_body(statements, 2);
  assert(err == ERR_OK);

  printf("Test: Implicit conversion Int to Double Passed\n");
}

static void test_readonly_variable_assign(void) {
  AstNode *decl =
      ast_new_var_decl(strdup("constVal"), false, T_INT, true, NULL, 1);
  AstNode *val = ast_new_int(TEST_INT_VAL_4, 1);
  AstNode *assign = ast_new_assign(strdup("constVal"), val, 1);
  AstNode *statements[] = {decl, assign};

  SemanticError err = analyze_main_body(statements, 2);
  assert(err == ERR_IMMUTABLE);

  printf("Test: Assignment to constant (val) Passed\n");
}

void test_semantic(void) {
  printf("\n--- SEMANTIC TESTS ---\n");
  test_valid_assignment();
  test_undefined_variable();
  test_type_mismatch();
  test_int_to_double_implicit_conversion();
  test_readonly_variable_assign();
  test_if_without_else_in_explicit_unit_function();
  test_if_without_else_inferred_unit_function();
  test_if_without_else_non_unit_inferred_function();
  printf("Semantic tests completed\n");
}
