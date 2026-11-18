/**
 * @file test_codegen.c
 * @author  xoramst00
 * @brief Checks that BOOLTHEN expressions and control flow become the
 *        intended IFJcode26 instructions.
 *
 * AI-assisted: I used AI to help create this test suite.
 */

#include "test_codegen.h"
#include "../src/ast.h"
#include "../src/codegen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *capture_expression(void (*generate)(void)) {
  // Temporarily redirect stdout so one generator call can be checked as text
  fflush(stdout);
  FILE *capture = tmpfile();
  assert(capture != NULL);

  int savedStdout = dup(STDOUT_FILENO);
  assert(savedStdout >= 0);
  assert(dup2(fileno(capture), STDOUT_FILENO) >= 0);

  generate();

  fflush(stdout);
  assert(dup2(savedStdout, STDOUT_FILENO) >= 0);
  close(savedStdout);

  rewind(capture);
  char *output = malloc(4096);
  assert(output != NULL);
  size_t length = fread(output, 1, 4095, capture);
  output[length] = '\0';
  fclose(capture);
  return output;
}

static AstNode *bool_not_node;
static AstNode *and_node;
static AstNode *or_node;
static AstNode *else_less_if;
static AstNode *else_if_chain;
static AstNode *if_expression;
static AstNode *comparison_node;
static AstNode *while_node;
static AstNode *program_node;

static void generate_not(void) { cg_gen_expr(bool_not_node); }

static void generate_and(void) { cg_gen_expr(and_node); }

static void generate_or(void) { cg_gen_expr(or_node); }

static void generate_else_less_if(void) {
  cg_gen_if(else_less_if, false, T_UNIT);
}

static void generate_else_if(void) { cg_gen_if(else_if_chain, false, T_UNIT); }

static void generate_if_expression(void) { cg_gen_expr(if_expression); }

static void generate_comparison(void) { cg_gen_expr(comparison_node); }

static void generate_while(void) { cg_gen_while(while_node, T_UNIT); }

static void generate_program(void) { cg_gen_program(program_node); }

static AstNode *bool_block(bool value) {
  AstNode **statements = malloc(sizeof(*statements));
  assert(statements != NULL);
  statements[0] = ast_new_expr_statement(ast_new_bool(value, 1), 1);
  return ast_new_block(statements, 1, 1);
}

static AstNode *int_expression_block(AstNode *expression) {
  AstNode **statements = malloc(sizeof(*statements));
  assert(statements != NULL);
  statements[0] = ast_new_expr_statement(expression, 1);
  return ast_new_block(statements, 1, 1);
}

static void test_unary_not(void) {
  // A negated Boolean should leave one Boolean result and use NOTS
  bool_not_node = ast_new_unary(OP_UNARY_NOT, ast_new_bool(false, 1), 1);
  bool_not_node->evalType = T_BOOLEAN;

  char *output = capture_expression(generate_not);
  assert(strstr(output, "PUSHS bool@false") != NULL);
  assert(strstr(output, "NOTS") != NULL);
  free(output);
  ast_free(bool_not_node);

  printf("Test: BOOLTHEN unary not codegen Passed\n");
}

static void test_short_circuit_and(void) {
  // AND should jump around the right operand; it must not emit ANDS
  and_node =
      ast_new_binary(OP_AND, ast_new_bool(false, 1), ast_new_bool(true, 1), 1);
  and_node->evalType = T_BOOLEAN;

  char *output = capture_expression(generate_and);
  assert(strstr(output, "$and_false_") != NULL);
  assert(strstr(output, "$and_end_") != NULL);
  assert(strstr(output, "PUSHS bool@false") != NULL);
  assert(strstr(output, "ANDS") == NULL);
  free(output);
  ast_free(and_node);

  printf("Test: BOOLTHEN short-circuit AND codegen Passed\n");
}

static void test_short_circuit_or(void) {
  // OR should jump around the right operand; it must not emit ORS
  or_node =
      ast_new_binary(OP_OR, ast_new_bool(true, 1), ast_new_bool(false, 1), 1);
  or_node->evalType = T_BOOLEAN;

  char *output = capture_expression(generate_or);
  assert(strstr(output, "$or_true_") != NULL);
  assert(strstr(output, "$or_end_") != NULL);
  assert(strstr(output, "PUSHS bool@true") != NULL);
  assert(strstr(output, "ORS") == NULL);
  free(output);
  ast_free(or_node);

  printf("Test: BOOLTHEN short-circuit OR codegen Passed\n");
}

static void test_else_less_if(void) {
  // A statement-style if with no else must not push a value
  else_less_if = ast_new_if(ast_new_bool(true, 1), bool_block(true), NULL, 1);

  char *output = capture_expression(generate_else_less_if);
  assert(strstr(output, "$if_else_") != NULL);
  assert(strstr(output, "$if_end_") != NULL);
  free(output);
  ast_free(else_less_if);

  printf("Test: BOOLTHEN else-less if codegen Passed\n");
}

static void test_else_if(void) {
  // The parser represents else-if as an AST_IF in the outer else branch
  AstNode *nested =
      ast_new_if(ast_new_bool(true, 1), bool_block(true), NULL, 1);
  else_if_chain =
      ast_new_if(ast_new_bool(false, 1), bool_block(false), nested, 1);

  char *output = capture_expression(generate_else_if);
  assert(strstr(output, "$if_else_") != NULL);
  assert(strstr(output, "$if_end_") != NULL);
  free(output);
  ast_free(else_if_chain);

  printf("Test: BOOLTHEN else-if codegen Passed\n");
}

static void test_if_expression(void) {
  // Each branch must leave an Int value instead of changing the result to nil
  AstNode *thenValue = ast_new_int(1, 1);
  AstNode *elseValue = ast_new_int(2, 1);
  if_expression =
      ast_new_if(ast_new_bool(true, 1), int_expression_block(thenValue),
                 int_expression_block(elseValue), 1);
  if_expression->evalType = T_INT;

  char *output = capture_expression(generate_if_expression);
  assert(strstr(output, "PUSHS int@1") != NULL);
  assert(strstr(output, "PUSHS int@2") != NULL);
  assert(strstr(output, "PUSHS nil@nil") == NULL);
  free(output);
  ast_free(if_expression);

  printf("Test: BOOLTHEN if-expression codegen Passed\n");
}

static void test_boolean_comparison(void) {
  // Relational expressions produce the Boolean value used by conditions.
  comparison_node =
      ast_new_binary(OP_LT, ast_new_int(1, 1), ast_new_int(2, 1), 1);
  comparison_node->evalType = T_BOOLEAN;

  char *output = capture_expression(generate_comparison);
  assert(strstr(output, "PUSHS int@1") != NULL);
  assert(strstr(output, "PUSHS int@2") != NULL);
  assert(strstr(output, "LTS") != NULL);
  free(output);
  ast_free(comparison_node);

  printf("Test: BOOLTHEN comparison codegen Passed\n");
}

static void test_boolean_while(void) {
  // A while condition is generated the same way as an if condition.
  while_node = ast_new_while(
      ast_new_bool(true, 1),
      ast_new_block(NULL, 0, 1),
      1);

  char *output = capture_expression(generate_while);
  assert(strstr(output, "$while_start_") != NULL);
  assert(strstr(output, "$while_end_") != NULL);
  assert(strstr(output, "PUSHS bool@false") != NULL);
  free(output);
  ast_free(while_node);

  printf("Test: BOOLTHEN while-condition codegen Passed\n");
}

static void test_normal_boolean_program(void) {
  // Exercise declaration, assignment, READ, comparison and print in one AST.
  AstNode *enabledDecl =
      ast_new_var_decl(strdup("enabled"), true, T_BOOLEAN, true,
                       ast_new_bool(true, 1), 1);
  enabledDecl->uid = 1;

  AstNode *enabledAssign =
      ast_new_assign(strdup("enabled"), ast_new_bool(false, 2), 2);
  enabledAssign->uid = 1;

  AstNode **readArgs = malloc(sizeof(*readArgs));
  assert(readArgs != NULL);
  readArgs[0] = ast_new_func_call(strdup("readBoolean"), NULL, 0, 3);
  readArgs[0]->evalType = T_BOOLEAN;

  AstNode *printCall =
      ast_new_func_call(strdup("print"), readArgs, 1, 3);
  printCall->evalType = T_UNIT;

  AstNode **mainStatements = malloc(3 * sizeof(*mainStatements));
  assert(mainStatements != NULL);
  mainStatements[0] = enabledDecl;
  mainStatements[1] = enabledAssign;
  mainStatements[2] = ast_new_expr_statement(printCall, 3);

  AstNode *mainBody =
      ast_new_block(mainStatements, 3, 1);
  AstNode *mainFunction =
      ast_new_func_def(strdup("main"), true, T_UNIT, NULL, 0, mainBody, 1);

  AstNode **functions = malloc(sizeof(*functions));
  assert(functions != NULL);
  functions[0] = mainFunction;
  program_node = ast_new_program(functions, 1, 1);

  char *output = capture_expression(generate_program);
  assert(strstr(output, ".IFJcode26") != NULL);
  assert(strstr(output, "LF@enabled$1") != NULL);
  assert(strstr(output, "READ GF@$tmp1 bool") != NULL);
  assert(strstr(output, "WRITE GF@$tmp1") != NULL);
  free(output);
  ast_free(program_node);

  printf("Test: BOOLTHEN normal program flow Passed\n");
}

void test_codegen(void) {
  test_unary_not();
  test_short_circuit_and();
  test_short_circuit_or();
  test_else_less_if();
  test_else_if();
  test_if_expression();
  test_boolean_comparison();
  test_boolean_while();
  test_normal_boolean_program();
}
