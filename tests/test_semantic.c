/**
 * @file    test_semantic.c
 * @author  xsirmat00
 * @brief   Samostatné unit testy pro sémantický analyzátor.
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

static void test_valid_assignment(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  char *var_x = strdup("x");
  assert(st_insert_var(&symtable, var_x, T_INT, true) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("x"), val, 1);

  SemanticError err = analyze_semantics(&symtable, assign);
  assert(err == ERR_SEM_OK);

  ast_free(assign);
  st_free(&symtable);
  free(var_x);
  printf("Test: Sémanticky platné přiřazení Passed\n");
}

static void test_undefined_variable(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_2, 1);
  AstNode *assign = ast_new_assign(strdup("y"), val, 1);

  SemanticError err = analyze_semantics(&symtable, assign);
  assert(err == ERR_SEM_UNDEFINED);

  ast_free(assign);
  st_free(&symtable);
  printf("Test: Nedefinovaná proměnná Passed\n");
}

static void test_type_mismatch(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  char *var_num = strdup("num");
  assert(st_insert_var(&symtable, var_num, T_INT, true) == true);

  AstNode *val = ast_new_double(TEST_DOUBLE_VAL, 1);
  AstNode *assign = ast_new_assign(strdup("num"), val, 1);

  SemanticError err = analyze_semantics(&symtable, assign);
  assert(err == ERR_SEM_TYPE_MISMATCH);

  ast_free(assign);
  st_free(&symtable);
  free(var_num);
  printf("Test: Typová nekompatibilita Passed\n");
}

static void test_int_to_double_implicit_conversion(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  char *var_d = strdup("d");
  assert(st_insert_var(&symtable, var_d, T_DOUBLE, true) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_3, 1);
  AstNode *assign = ast_new_assign(strdup("d"), val, 1);

  SemanticError err = analyze_semantics(&symtable, assign);
  assert(err == ERR_SEM_OK);

  ast_free(assign);
  st_free(&symtable);
  free(var_d);
  printf("Test: Implicitní konverze Int na Double Passed\n");
}

static void test_readonly_variable_assign(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  char *var_const = strdup("constVal");
  assert(st_insert_var(&symtable, var_const, T_INT, false) == true);

  AstNode *val = ast_new_int(TEST_INT_VAL_4, 1);
  AstNode *assign = ast_new_assign(strdup("constVal"), val, 1);

  SemanticError err = analyze_semantics(&symtable, assign);
  assert(err == ERR_SEM_OTHER);

  ast_free(assign);
  st_free(&symtable);
  free(var_const);
  printf("Test: Přiřazení do konstanty (val) Passed\n");
}

void test_semantic(void) {
  printf("\n--- SÉMANTICKÉ TESTY ---\n");
  test_valid_assignment();
  test_undefined_variable();
  test_type_mismatch();
  test_int_to_double_implicit_conversion();
  test_readonly_variable_assign();
  printf("Sémantické testy dokončeny\n");
}