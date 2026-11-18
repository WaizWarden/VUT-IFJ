/**
 * @file    test_precedence.c
 * @author  xhorakj01
 * @brief   Tests for precedence analysis of expressions
 *
 * Each expression is fed to the scanner from a string via fmemopen() and the
 * shape of the resulting AST is checked with asserts.
 */

#include "test_precedence.h"
#include "../src/precedence.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Parses an expression from a string
 *
 * @param src  Source text of the expression
 * @param stop Out (may be NULL): token that terminated the expression
 * @return AST of the expression or NULL on syntax error
 */
static AstNode *parse_str(const char *src, Token *stop) {
  FILE *input = fmemopen((void *)src, strlen(src), "r");
  assert(input != NULL);

  Token token = get_token(input);
  AstNode *expr = precedence_parse(input, &token, NULL, NULL);

  if (stop != NULL) {
    *stop = token;
  }

  fclose(input);
  // scanner keeps its state in static variables, reset it between tests
  scanner_destroy();
  return expr;
}

/**
 * @brief Checks that the node is a binary operation of the given kind
 */
static bool is_binary(const AstNode *node, BinaryOperation operation) {
  return (bool)(node != NULL && node->type == AST_BINARY &&
                node->as.binary.operation == operation);
}

/**
 * @brief Checks that the node is a unary minus
 */
static bool is_unary_minus(const AstNode *node) {
  return (bool)(node != NULL && node->type == AST_UNARY &&
                node->as.unary.op == OP_UNARY_MINUS);
}

/**
 * @brief Checks that the node is an identifier with the given name
 */
static bool is_identifier(const AstNode *node, const char *name) {
  return (bool)(node != NULL && node->type == AST_IDENTIFIER &&
                strcmp(node->as.identif, name) == 0);
}

/**
 * @brief Checks that the node is an Int literal with the given value
 */
static bool is_int(const AstNode *node, int value) {
  return (bool)(node != NULL && node->type == AST_LITERAL_INT &&
                node->as.intValue == value);
}

/**
 * @brief Checks that the expression is rejected as a syntax error
 */
static void assert_syntax_error(const char *src) {
  AstNode *expr = parse_str(src, NULL);
  assert(expr == NULL);
}

/**
 * @brief a + b * c  ->  ADD(a, MUL(b, c))
 */
static void test_priority_mul_over_add(void) {
  AstNode *expr = parse_str("a + b * c\n", NULL);

  assert(is_binary(expr, OP_ADD));
  assert(is_identifier(expr->as.binary.left, "a"));
  assert(is_binary(expr->as.binary.right, OP_MUL));
  assert(is_identifier(expr->as.binary.right->as.binary.left, "b"));
  assert(is_identifier(expr->as.binary.right->as.binary.right, "c"));

  ast_free(expr);
}

/**
 * @brief a - b - c  ->  SUB(SUB(a, b), c)  (left associativity)
 */
static void test_left_associativity(void) {
  AstNode *expr = parse_str("a - b - c\n", NULL);

  assert(is_binary(expr, OP_SUB));
  assert(is_binary(expr->as.binary.left, OP_SUB));
  assert(is_identifier(expr->as.binary.left->as.binary.left, "a"));
  assert(is_identifier(expr->as.binary.left->as.binary.right, "b"));
  assert(is_identifier(expr->as.binary.right, "c"));

  ast_free(expr);
}

/**
 * @brief a / b * c  ->  MUL(DIV(a, b), c)  (same priority, left assoc.)
 */
static void test_mul_div_same_priority(void) {
  AstNode *expr = parse_str("a / b * c\n", NULL);

  assert(is_binary(expr, OP_MUL));
  assert(is_binary(expr->as.binary.left, OP_DIV));
  assert(is_identifier(expr->as.binary.right, "c"));

  ast_free(expr);
}

/**
 * @brief (a + b) * c  ->  MUL(ADD(a, b), c)
 */
static void test_parentheses(void) {
  AstNode *expr = parse_str("(a + b) * c\n", NULL);

  assert(is_binary(expr, OP_MUL));
  assert(is_binary(expr->as.binary.left, OP_ADD));
  assert(is_identifier(expr->as.binary.right, "c"));

  ast_free(expr);
}

/**
 * @brief 3 * (2 - 1) without trailing newline, terminated by EOF
 */
static void test_terminated_by_eof(void) {
  Token stop;
  AstNode *expr = parse_str("3 * (2 - 1)", &stop);

  assert(stop.type == TOKEN_EOF);
  assert(is_binary(expr, OP_MUL));
  assert(is_int(expr->as.binary.left, 3));
  assert(is_binary(expr->as.binary.right, OP_SUB));
  assert(is_int(expr->as.binary.right->as.binary.left, 2));
  assert(is_int(expr->as.binary.right->as.binary.right, 1));

  ast_free(expr);
}

/**
 * @brief -a * b  ->  MUL(UNARY(a), b)  (unary minus has highest priority)
 */
static void test_unary_minus_priority(void) {
  AstNode *expr = parse_str("-a * b\n", NULL);

  assert(is_binary(expr, OP_MUL));
  assert(is_unary_minus(expr->as.binary.left));
  assert(is_identifier(expr->as.binary.left->as.unary.operand, "a"));
  assert(is_identifier(expr->as.binary.right, "b"));

  ast_free(expr);
}

/**
 * @brief --a  ->  UNARY(UNARY(a))  (right associativity)
 */
static void test_double_unary_minus(void) {
  AstNode *expr = parse_str("--a\n", NULL);

  assert(is_unary_minus(expr));
  assert(is_unary_minus(expr->as.unary.operand));
  assert(is_identifier(expr->as.unary.operand->as.unary.operand, "a"));

  ast_free(expr);
}

/**
 * @brief a - -b  ->  SUB(a, UNARY(b))  (binary minus followed by unary)
 */
static void test_binary_then_unary_minus(void) {
  AstNode *expr = parse_str("a - -b\n", NULL);

  assert(is_binary(expr, OP_SUB));
  assert(is_identifier(expr->as.binary.left, "a"));
  assert(is_unary_minus(expr->as.binary.right));

  ast_free(expr);
}

/**
 * @brief (-a)  ->  UNARY(a)  (minus right after '(' is unary)
 */
static void test_unary_minus_after_paren(void) {
  AstNode *expr = parse_str("(-a)\n", NULL);

  assert(is_unary_minus(expr));
  assert(is_identifier(expr->as.unary.operand, "a"));

  ast_free(expr);
}

/**
 * @brief a + 1 <= b * 2  ->  LE(ADD(a, 1), MUL(b, 2))
 */
static void test_relational_priority(void) {
  AstNode *expr = parse_str("a + 1 <= b * 2\n", NULL);

  assert(is_binary(expr, OP_LE));
  assert(is_binary(expr->as.binary.left, OP_ADD));
  assert(is_binary(expr->as.binary.right, OP_MUL));

  ast_free(expr);
}

/**
 * @brief Every relational and equality operator maps to its AST operation
 */
static void test_all_comparison_operators(void) {
  const struct {
    const char *src;
    BinaryOperation operation;
  } cases[] = {
      {"a < b\n", OP_LT},  {"a <= b\n", OP_LE}, {"a > b\n", OP_GT},
      {"a >= b\n", OP_GE}, {"a == b\n", OP_EQ}, {"a != b\n", OP_NEQ},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    AstNode *expr = parse_str(cases[i].src, NULL);

    assert(is_binary(expr, cases[i].operation));
    assert(is_identifier(expr->as.binary.left, "a"));
    assert(is_identifier(expr->as.binary.right, "b"));

    ast_free(expr);
  }
}

/**
 * @brief Parenthesised comparison is syntactically valid: (a < b) == c
 */
static void test_parenthesised_comparison(void) {
  AstNode *expr = parse_str("(a < b) == c\n", NULL);

  assert(is_binary(expr, OP_EQ));
  assert(is_binary(expr->as.binary.left, OP_LT));

  ast_free(expr);
}

/**
 * @brief Single terms: identifier, Int, Double, String, Unit
 */
static void test_terms(void) {
  AstNode *expr = parse_str("x\n", NULL);
  assert(is_identifier(expr, "x"));
  ast_free(expr);

  expr = parse_str("42\n", NULL);
  assert(is_int(expr, 42));
  ast_free(expr);

  expr = parse_str("2.5\n", NULL);
  assert(expr != NULL && expr->type == AST_LITERAL_DOUBLE);
  assert(expr->as.doubleValue == 2.5);
  ast_free(expr);

  expr = parse_str("\"abc\"\n", NULL);
  assert(expr != NULL && expr->type == AST_LITERAL_STRING);
  assert(strcmp(expr->as.stringValue, "abc") == 0);
  ast_free(expr);

  expr = parse_str("()\n", NULL);
  assert(expr != NULL && expr->type == AST_LITERAL_UNIT);
  ast_free(expr);
}

/**
 * @brief Expression ends at 'then' / 'do' / ';' and leaves it unconsumed
 */
static void test_terminators(void) {
  Token stop;

  AstNode *expr = parse_str("x > 1 then\n", &stop);
  assert(is_binary(expr, OP_GT));
  assert(stop.type == TOKEN_KEYWORD && stop.value.keyword == KW_THEN);
  ast_free(expr);

  expr = parse_str("i < n do\n", &stop);
  assert(is_binary(expr, OP_LT));
  assert(stop.type == TOKEN_KEYWORD && stop.value.keyword == KW_DO);
  ast_free(expr);

  expr = parse_str("a + 1; b\n", &stop);
  assert(is_binary(expr, OP_ADD));
  assert(stop.type == TOKEN_SEMICOLON);
  ast_free(expr);
}

/**
 * @brief Parser hands over an already read identifier: "x" + "- 1"
 */
static void test_first_operand(void) {
  const char *src = "x - 1\n";
  FILE *input = fmemopen((void *)src, strlen(src), "r");
  assert(input != NULL);

  Token token = get_token(input); // x
  assert(token.type == TOKEN_IDENTIFIER);
  AstNode *ident = ast_new_identifier(token.value.identifier, 0);

  token = get_token(input); // -
  AstNode *expr = precedence_parse(input, &token, ident, NULL);

  // the minus must be binary and the handed over node must be reused
  assert(is_binary(expr, OP_SUB));
  assert(expr->as.binary.left == ident);
  assert(is_int(expr->as.binary.right, 1));

  ast_free(expr);
  fclose(input);
  scanner_destroy();
}

/**
 * @brief First operand alone: statement consisting only of "x"
 */
static void test_first_operand_alone(void) {
  const char *src = "x\n";
  FILE *input = fmemopen((void *)src, strlen(src), "r");
  assert(input != NULL);

  Token token = get_token(input); // x
  AstNode *ident = ast_new_identifier(token.value.identifier, 0);

  token = get_token(input); // NEWLINE
  AstNode *expr = precedence_parse(input, &token, ident, NULL);

  assert(expr == ident);
  assert(token.type == TOKEN_NEWLINE);

  ast_free(expr);
  fclose(input);
  scanner_destroy();
}

/**
 * @brief Syntax errors (empty cells in the table / no reduction rule)
 *
 * Only inputs that stop on an operator or at the end of the line are used,
 * so no allocated token (identifier / string) is left unconsumed.
 */
static void test_syntax_errors(void) {
  // chaining of relational operators is not allowed
  assert_syntax_error("a < b < c\n");
  assert_syntax_error("a == b == c\n");
  assert_syntax_error("a < b == c\n");
  assert_syntax_error("a != b >= c\n");

  // missing operand
  assert_syntax_error("a +\n");
  assert_syntax_error("a * \n");
  assert_syntax_error("-\n");

  // unbalanced parentheses
  assert_syntax_error("(a + b\n");

  // empty expression
  assert_syntax_error("\n");
}

/**
 * @brief Runs all precedence analysis tests
 */
void test_precedence(void) {
  test_priority_mul_over_add();
  test_left_associativity();
  test_mul_div_same_priority();
  test_parentheses();
  test_terminated_by_eof();
  test_unary_minus_priority();
  test_double_unary_minus();
  test_binary_then_unary_minus();
  test_unary_minus_after_paren();
  test_relational_priority();
  test_all_comparison_operators();
  test_parenthesised_comparison();
  test_terms();
  test_terminators();
  test_first_operand();
  test_first_operand_alone();
  test_syntax_errors();

  printf("Precedence tests done\n");
}
