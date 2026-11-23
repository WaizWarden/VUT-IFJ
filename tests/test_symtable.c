/**
 * @file test_symtable.c
 * @brief Tests variable/function symbols, scopes, and owned parameter storage
 *
 * AI-assisted: test structure and edge cases were prepared with AI assistance
 */

#include "test_symtable.h"
#include "../src/symtable.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_init_free(void) {
  // starting with an empty global scope is the basic symtable invariant
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(symtable.top != NULL);
  assert(symtable.top->parent == NULL);
  assert(st_get(&symtable, "missing") == NULL);

  st_free(&symtable);
  assert(symtable.top == NULL);

  printf("Test: Symtable init and free Passed\n");
}

static void test_insert_get_var(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  assert(st_insert_var(&symtable, "count", T_INT, true) == true);

  Symbol *found = st_get(&symtable, "count");
  assert(found != NULL);
  assert(found->type == ST_VAR);
  assert(found->data.var.type == T_INT);
  assert(found->data.var.modifiable == true);

  // redefining the same name in one scope must leave the original symbol intact
  assert(st_insert_var(&symtable, "count", T_DOUBLE, false) == false);
  found = st_get(&symtable, "count");
  assert(found->data.var.type == T_INT);
  assert(found->data.var.modifiable == true);

  st_free(&symtable);

  printf("Test: Symtable variable insert and lookup Passed\n");
}

static void test_nested_scope_shadowing(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_insert_var(&symtable, "value", T_INT, true) == true);

  assert(st_push_scope(&symtable) == true);
  assert(st_insert_var(&symtable, "value", T_STRING, false) == true);

  Symbol *local = st_get(&symtable, "value");
  assert(local != NULL);
  assert(local->data.var.type == T_STRING);
  assert(local->data.var.modifiable == false);

  // the outer value becomes visible again once the nested scope is removed
  st_pop_scope(&symtable);

  Symbol *outer = st_get(&symtable, "value");
  assert(outer != NULL);
  assert(outer->data.var.type == T_INT);
  assert(outer->data.var.modifiable == true);

  st_free(&symtable);

  printf("Test: Symtable nested scope shadowing Passed\n");
}

static void test_insert_get_function(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  Parameter *parameters = calloc(2, sizeof(*parameters));
  assert(parameters != NULL);
  parameters[0].name = strdup("left");
  parameters[0].type = T_INT;
  parameters[1].name = strdup("right");
  parameters[1].type = T_DOUBLE;
  assert(parameters[0].name != NULL);
  assert(parameters[1].name != NULL);

  assert(st_insert_func(&symtable, "combine", T_DOUBLE, parameters, 2, false,
                        true) == true);

  Symbol *found = st_get(&symtable, "combine");
  assert(found != NULL);
  assert(found->type == ST_FUNC);
  assert(found->data.func.returnType == T_DOUBLE);
  assert(found->data.func.paramCount == 2);
  assert(strcmp(found->data.func.parameters[0].name, "left") == 0);
  assert(found->data.func.parameters[0].type == T_INT);
  assert(strcmp(found->data.func.parameters[1].name, "right") == 0);
  assert(found->data.func.parameters[1].type == T_DOUBLE);

  st_free(&symtable);

  printf("Test: Symtable function parameters Passed\n");
}

static void test_function_is_global(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);
  assert(st_push_scope(&symtable) == true);

  // function definitions always belong to the global scope
  assert(st_insert_func(&symtable, "globalFunction", T_UNIT, NULL, 0, false,
                        true) == true);
  st_pop_scope(&symtable);

  Symbol *found = st_get(&symtable, "globalFunction");
  assert(found != NULL);
  assert(found->type == ST_FUNC);
  assert(found->data.func.parameters == NULL);
  assert(found->data.func.paramCount == 0);

  st_free(&symtable);

  printf("Test: Symtable function global scope Passed\n");
}

static void test_redefinition_cleanup(void) {
  Symtable symtable;
  assert(st_init(&symtable) == true);

  Parameter *first = calloc(1, sizeof(*first));
  assert(first != NULL);
  first[0].name = strdup("value");
  first[0].type = T_INT;
  assert(first[0].name != NULL);
  assert(st_insert_func(&symtable, "duplicate", T_INT, first, 1, false,
                        true) == true);

  Parameter *duplicate = calloc(1, sizeof(*duplicate));
  assert(duplicate != NULL);
  duplicate[0].name = strdup("rejected");
  duplicate[0].type = T_STRING;
  assert(duplicate[0].name != NULL);

  // failed insertion must not replace the existing function or leak its params
  assert(st_insert_func(&symtable, "duplicate", T_STRING, duplicate, 1, false,
                        true) == false);

  Symbol *found = st_get(&symtable, "duplicate");
  assert(found != NULL);
  assert(found->data.func.returnType == T_INT);
  assert(found->data.func.paramCount == 1);
  assert(strcmp(found->data.func.parameters[0].name, "value") == 0);

  st_free(&symtable);

  printf("Test: Symtable redefinition cleanup Passed\n");
}

void test_symtable(void) {
  test_init_free();
  test_insert_get_var();
  test_nested_scope_shadowing();
  test_insert_get_function();
  test_function_is_global();
  test_redefinition_cleanup();
}
