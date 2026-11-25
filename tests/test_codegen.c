/**
 * @file test_codegen.c
 * @author  xoramst00
 * @brief Checks that BOOLTHEN, CONTROL, and VALMEM constructs become the
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
static AstNode *for_default_node;
static AstNode *for_negative_guard_node;
static AstNode *for_zero_step_node;
static AstNode *for_prepass_node;
static AstNode *valmem_node;

static void generate_program(void) { cg_gen_program(program_node); }

static char *capture_test_statement(AstNode *statement) {
  AstNode **statements = malloc(2 * sizeof(*statements));
  assert(statements != NULL);
  statements[0] = statement;
  statements[1] = ast_new_return(ast_new_int(0, 1), 1);
  AstNode *body = ast_new_block(statements, 2, 1);
  AstNode *mainFunction =
      ast_new_func_def(strdup("main"), true, T_INT, NULL, 0, body, 1);

  AstNode **functions = malloc(sizeof(*functions));
  assert(functions != NULL);
  functions[0] = mainFunction;
  program_node = ast_new_program(functions, 1, 1);

  char *output = capture_expression(generate_program);
  ast_free(program_node);
  program_node = NULL;
  return output;
}

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

  char *output =
      capture_test_statement(ast_new_expr_statement(bool_not_node, 1));
  assert(strstr(output, "PUSHS bool@false") != NULL);
  assert(strstr(output, "NOTS") != NULL);
  free(output);
  printf("Test: BOOLTHEN unary not codegen Passed\n");
}

static void test_short_circuit_and(void) {
  // AND should jump around the right operand; it must not emit ANDS
  and_node =
      ast_new_binary(OP_AND, ast_new_bool(false, 1), ast_new_bool(true, 1), 1);
  and_node->evalType = T_BOOLEAN;

  char *output = capture_test_statement(ast_new_expr_statement(and_node, 1));
  assert(strstr(output, "$and_false_") != NULL);
  assert(strstr(output, "$and_end_") != NULL);
  assert(strstr(output, "PUSHS bool@false") != NULL);
  assert(strstr(output, "ANDS") == NULL);
  free(output);
  printf("Test: BOOLTHEN short-circuit AND codegen Passed\n");
}

static void test_short_circuit_or(void) {
  // OR should jump around the right operand; it must not emit ORS
  or_node =
      ast_new_binary(OP_OR, ast_new_bool(true, 1), ast_new_bool(false, 1), 1);
  or_node->evalType = T_BOOLEAN;

  char *output = capture_test_statement(ast_new_expr_statement(or_node, 1));
  assert(strstr(output, "$or_true_") != NULL);
  assert(strstr(output, "$or_end_") != NULL);
  assert(strstr(output, "PUSHS bool@true") != NULL);
  assert(strstr(output, "ORS") == NULL);
  free(output);
  printf("Test: BOOLTHEN short-circuit OR codegen Passed\n");
}

static void test_else_less_if(void) {
  // A statement-style if with no else must not push a value
  else_less_if = ast_new_if(ast_new_bool(true, 1), bool_block(true), NULL, 1);

  char *output = capture_test_statement(else_less_if);
  assert(strstr(output, "$if_else_") != NULL);
  assert(strstr(output, "$if_end_") != NULL);
  free(output);
  printf("Test: BOOLTHEN else-less if codegen Passed\n");
}

static void test_else_if(void) {
  // The parser represents else-if as an AST_IF in the outer else branch
  AstNode *nested =
      ast_new_if(ast_new_bool(true, 1), bool_block(true), NULL, 1);
  else_if_chain =
      ast_new_if(ast_new_bool(false, 1), bool_block(false), nested, 1);

  char *output = capture_test_statement(else_if_chain);
  assert(strstr(output, "$if_else_") != NULL);
  assert(strstr(output, "$if_end_") != NULL);
  free(output);
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

  char *output =
      capture_test_statement(ast_new_expr_statement(if_expression, 1));
  assert(strstr(output, "PUSHS int@1") != NULL);
  assert(strstr(output, "PUSHS int@2") != NULL);
  assert(strstr(output, "PUSHS nil@nil") == NULL);
  free(output);
  printf("Test: BOOLTHEN if-expression codegen Passed\n");
}

static void test_boolean_comparison(void) {
  // Relational expressions produce the Boolean value used by conditions.
  comparison_node =
      ast_new_binary(OP_LT, ast_new_int(1, 1), ast_new_int(2, 1), 1);
  comparison_node->evalType = T_BOOLEAN;

  char *output =
      capture_test_statement(ast_new_expr_statement(comparison_node, 1));
  assert(strstr(output, "PUSHS int@1") != NULL);
  assert(strstr(output, "PUSHS int@2") != NULL);
  assert(strstr(output, "LTS") != NULL);
  free(output);
  printf("Test: BOOLTHEN comparison codegen Passed\n");
}

static void test_boolean_while(void) {
  // A while condition is generated the same way as an if condition.
  while_node =
      ast_new_while(ast_new_bool(true, 1), ast_new_block(NULL, 0, 1), 1);

  char *output = capture_test_statement(while_node);
  assert(strstr(output, "$while_start_") != NULL);
  assert(strstr(output, "$while_end_") != NULL);
  assert(strstr(output, "PUSHS bool@false") != NULL);
  free(output);
  printf("Test: BOOLTHEN while-condition codegen Passed\n");
}

static void test_control_default_step(void) {
  // An inclusive range without `by` should begin at 1 and advance once per
  // iteration. This small program exercises both setup and the loop boundary.
  AstNode *body = ast_new_block(NULL, 0, 1);
  for_default_node =
      ast_new_for(strdup("i"), ast_new_int(1, 1), ast_new_int(5, 1), NULL, true,
                  NULL, 0, body, 1);
  for_default_node->as.forStatement.varUid = 42;

  char *output = capture_test_statement(for_default_node);
  assert(strstr(output, "PUSHS int@1") != NULL);
  assert(strstr(output, "POPS LF@i$42") != NULL);
  assert(strstr(output, "POPS LF@%end$42") != NULL);
  assert(strstr(output, "LF@%step$42") != NULL);
  assert(strstr(output, "$for_start_") != NULL);
  assert(strstr(output, "GT LF@%cond$42") != NULL);
  assert(strstr(output, "JUMPIFEQ $for_end_") != NULL);
  assert(strstr(output, "ADD LF@") != NULL);
  assert(strstr(output, "LF@i$42") != NULL);
  assert(strstr(output, "LF@%step$42") != NULL);
  assert(strstr(output, "JUMP $for_start_") != NULL);
  free(output);
  printf("Test: CONTROL inclusive for with default step Passed\n");
}

static void test_control_negative_until_guards(void) {
  // A negative step makes the loop run backwards. The guard must skip the
  // body for one iteration while the loop variable is still advanced.
  AstNode *guard = ast_new_binary(OP_GT, ast_new_identifier(strdup("i"), 1),
                                  ast_new_int(0, 1), 1);
  guard->evalType = T_BOOLEAN;
  AstNode **guards = malloc(sizeof(*guards));
  assert(guards != NULL);
  guards[0] = guard;

  AstNode *body = ast_new_block(NULL, 0, 1);
  for_negative_guard_node =
      ast_new_for(strdup("i"), ast_new_int(5, 1), ast_new_int(1, 1),
                  ast_new_int(-2, 1), false, guards, 1, body, 1);
  for_negative_guard_node->as.forStatement.varUid = 43;

  char *output = capture_test_statement(for_negative_guard_node);
  assert(strstr(output, "PUSHS int@-2") != NULL);
  assert(strstr(output, "$for_negative_") != NULL);
  assert(strstr(output, "LT LF@%cond$43") != NULL);
  assert(strstr(output, "EQ LF@%cond$43") != NULL);
  assert(strstr(output, "$for_next_") != NULL);
  assert(strstr(output, "JUMPIFEQS $for_next_") != NULL);
  assert(strstr(output, "ADD LF@") != NULL);
  assert(strstr(output, "LF@i$43") != NULL);
  assert(strstr(output, "LF@%step$43") != NULL);
  free(output);
  printf("Test: CONTROL descending until loop with guard Passed\n");
}

static void test_control_zero_step(void) {
  // A zero step is invalid: it must terminate with runtime error 40 rather
  // than turn the loop into an accidental infinite loop.
  AstNode *body = ast_new_block(NULL, 0, 1);
  for_zero_step_node =
      ast_new_for(strdup("i"), ast_new_int(1, 1), ast_new_int(5, 1),
                  ast_new_int(0, 1), true, NULL, 0, body, 1);
  for_zero_step_node->as.forStatement.varUid = 44;

  char *output = capture_test_statement(for_zero_step_node);
  assert(strstr(output, "JUMPIFEQ $for_zero_") != NULL);
  assert(strstr(output, "LF@%step$44 int@0") != NULL);
  assert(strstr(output, "LABEL $for_zero_") != NULL);
  assert(strstr(output, "EXIT int@40") != NULL);
  free(output);
  printf("Test: CONTROL zero-step loop exits with error 40 Passed\n");
}

static void test_control_for_prepass(void) {
  // The prepass must declare the loop variable and internal FOR helpers,
  // including declarations found inside the loop body.
  AstNode *innerDecl = ast_new_var_decl(strdup("value"), true, T_INT, true,
                                        ast_new_int(7, 1), 1);
  innerDecl->uid = 45;

  AstNode **bodyStatements = malloc(sizeof(*bodyStatements));
  assert(bodyStatements != NULL);
  bodyStatements[0] = innerDecl;
  AstNode *body = ast_new_block(bodyStatements, 1, 1);

  AstNode *loop = ast_new_for(strdup("i"), ast_new_int(1, 1), ast_new_int(3, 1),
                              NULL, true, NULL, 0, body, 1);
  loop->as.forStatement.varUid = 46;

  AstNode **mainStatements = malloc(sizeof(*mainStatements));
  assert(mainStatements != NULL);
  mainStatements[0] = loop;
  AstNode *mainBody = ast_new_block(mainStatements, 1, 1);
  AstNode *mainFunction =
      ast_new_func_def(strdup("main"), true, T_UNIT, NULL, 0, mainBody, 1);

  AstNode **functions = malloc(sizeof(*functions));
  assert(functions != NULL);
  functions[0] = mainFunction;
  for_prepass_node = ast_new_program(functions, 1, 1);
  program_node = for_prepass_node;

  char *output = capture_expression(generate_program);
  assert(strstr(output, "DEFVAR LF@i$46") != NULL);
  assert(strstr(output, "DEFVAR LF@%end$46") != NULL);
  assert(strstr(output, "DEFVAR LF@%step$46") != NULL);
  assert(strstr(output, "DEFVAR LF@%cond$46") != NULL);
  assert(strstr(output, "DEFVAR LF@value$45") != NULL);
  free(output);
  ast_free(for_prepass_node);

  printf("Test: CONTROL for-loop prepass declarations Passed\n");
}

static void test_valmem_to_string(void) {
  // toString is desugared to toStr, including Double formatting behavior
  AstNode *receiver = ast_new_double(1.2345, 1);
  receiver->evalType = T_DOUBLE;

  valmem_node =
      ast_new_member_call(receiver, strdup("toString"), 1);
  valmem_node->evalType = T_STRING;

  char *output =
      capture_test_statement(ast_new_expr_statement(valmem_node, 1));
  assert(strstr(output, "PUSHS float@") != NULL);
  assert(strstr(output, "FLOAT2STR") != NULL);
  assert(strstr(output, "PUSHS GF@$tmp1") != NULL);
  free(output);
  printf("Test: VALMEM toString codegen Passed\n");
}

static void test_valmem_to_int(void) {
  // Double.toInt should truncate the receiver toward zero
  AstNode *receiver = ast_new_double(-3.75, 1);
  receiver->evalType = T_DOUBLE;

  valmem_node = ast_new_member_call(receiver, strdup("toInt"), 1);
  valmem_node->evalType = T_INT;

  char *output =
      capture_test_statement(ast_new_expr_statement(valmem_node, 1));
  assert(strstr(output, "PUSHS float@") != NULL);
  assert(strstr(output, "FLOAT2INTS") != NULL);
  free(output);
  printf("Test: VALMEM toInt codegen Passed\n");
}

static void test_valmem_length(void) {
  // String.length should reuse strLength and leave an Int on the stack
  AstNode *receiver = ast_new_string(strdup("IFJ26"), 1);
  receiver->evalType = T_STRING;

  valmem_node = ast_new_member_call(receiver, strdup("length"), 1);
  valmem_node->evalType = T_INT;

  char *output =
      capture_test_statement(ast_new_expr_statement(valmem_node, 1));
  assert(strstr(output, "STRLEN GF@$tmp1 GF@$tmp1") != NULL);
  assert(strstr(output, "PUSHS GF@$tmp1") != NULL);
  free(output);
  printf("Test: VALMEM length codegen Passed\n");
}

static void test_valmem_floor(void) {
  // Floor must truncate, then lower negative fractional values by one
  AstNode *negativeReceiver = ast_new_double(-3.75, 1);
  negativeReceiver->evalType = T_DOUBLE;

  valmem_node =
      ast_new_member_call(negativeReceiver, strdup("floor"), 1);
  valmem_node->evalType = T_DOUBLE;

  char *output =
      capture_test_statement(ast_new_expr_statement(valmem_node, 1));
  assert(strstr(output, "FLOAT2INT GF@$tmp2 GF@$tmp1") != NULL);
  assert(strstr(output, "INT2FLOAT GF@$tmp3 GF@$tmp2") != NULL);
  assert(strstr(output, "LT GF@$tmp4 GF@$tmp1 GF@$tmp3") != NULL);
  assert(strstr(output, "$floor_end_") != NULL);
  assert(strstr(output, "SUB GF@$tmp3 GF@$tmp3 float@0x1p+0") != NULL);
  assert(strstr(output, "PUSHS GF@$tmp3") != NULL);
  free(output);
  printf("Test: VALMEM floor codegen Passed\n");
}

static void test_valmem_chained_floor_to_int(void) {
  // Chained members remain nested calls: floor must produce Double first
  // and toInt can then consume that result
  AstNode *receiver = ast_new_double(-3.75, 1);
  receiver->evalType = T_DOUBLE;

  AstNode *floored =
      ast_new_member_call(receiver, strdup("floor"), 1);
  floored->evalType = T_DOUBLE;

  valmem_node = ast_new_member_call(floored, strdup("toInt"), 1);
  valmem_node->evalType = T_INT;

  char *output =
      capture_test_statement(ast_new_expr_statement(valmem_node, 1));
  assert(strstr(output, "FLOAT2INT GF@$tmp2 GF@$tmp1") != NULL);
  assert(strstr(output, "INT2FLOAT GF@$tmp3 GF@$tmp2") != NULL);
  assert(strstr(output, "$floor_end_") != NULL);
  assert(strstr(output, "FLOAT2INTS") != NULL);
  free(output);
  printf("Test: VALMEM chained floor.toInt codegen Passed\n");
}

static void test_normal_boolean_program(void) {
  // Exercise declaration, assignment, READ, comparison and print in one AST.
  AstNode *enabledDecl = ast_new_var_decl(strdup("enabled"), true, T_BOOLEAN,
                                          true, ast_new_bool(true, 1), 1);
  enabledDecl->uid = 1;

  AstNode *enabledAssign =
      ast_new_assign(strdup("enabled"), ast_new_bool(false, 2), 2);
  enabledAssign->uid = 1;

  AstNode **readArgs = malloc(sizeof(*readArgs));
  assert(readArgs != NULL);
  readArgs[0] = ast_new_func_call(strdup("readBoolean"), NULL, 0, 3);
  readArgs[0]->evalType = T_BOOLEAN;

  AstNode *printCall = ast_new_func_call(strdup("print"), readArgs, 1, 3);
  printCall->evalType = T_UNIT;

  AstNode **mainStatements = malloc(3 * sizeof(*mainStatements));
  assert(mainStatements != NULL);
  mainStatements[0] = enabledDecl;
  mainStatements[1] = enabledAssign;
  mainStatements[2] = ast_new_expr_statement(printCall, 3);

  AstNode *mainBody = ast_new_block(mainStatements, 3, 1);
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
  test_control_default_step();
  test_control_negative_until_guards();
  test_control_zero_step();
  test_control_for_prepass();
  test_valmem_to_string();
  test_valmem_to_int();
  test_valmem_length();
  test_valmem_floor();
  test_valmem_chained_floor_to_int();
  test_normal_boolean_program();
}
