/**
 * @file    test_ast.c
 * @author  xhorakj01
 * @brief   AST smoke test - build, print and free a tree of all node types
 */

#include "test_ast.h"
#include "../src/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_str(const char *s) {
  char *copy = malloc(strlen(s) + 1);
  strcpy(copy, s);
  return copy;
}

void test_ast(void) {
  /* factorial(n: Int): Int = while n > 1 do result = result * n; ... */
  Parameter *params = malloc(sizeof(Parameter));
  params[0].name = dup_str("n");
  params[0].type = T_INT;

  AstNode **whileStmts = malloc(2 * sizeof(AstNode *));
  whileStmts[0] = ast_new_assign(
      dup_str("result"),
      ast_new_binary(OP_MUL, ast_new_identifier(dup_str("result"), 4),
                     ast_new_identifier(dup_str("n"), 4), 4),
      4);
  whileStmts[1] = ast_new_expr_statement(
      ast_new_binary(OP_SUB, ast_new_identifier(dup_str("n"), 5),
                     ast_new_int(1, 5), 5),
      5);

  AstNode *whileNode =
      ast_new_while(ast_new_binary(OP_GT, ast_new_identifier(dup_str("n"), 3),
                                   ast_new_int(1, 3), 3),
                    ast_new_block(whileStmts, 2, 3), 3);

  AstNode *ifNode =
      ast_new_if(ast_new_binary(OP_EQ, ast_new_identifier(dup_str("n"), 6),
                                ast_new_int(0, 6), 6),
                 ast_new_block(NULL, 0, 6), ast_new_block(NULL, 0, 6), 6);

  AstNode **funcBody = malloc(4 * sizeof(AstNode *));
  funcBody[0] = ast_new_var_decl(dup_str("result"), true, T_INT, false,
                                 ast_new_int(1, 2), 2);
  funcBody[1] = whileNode;
  funcBody[2] = ifNode;
  funcBody[3] = ast_new_return(ast_new_identifier(dup_str("result"), 7), 7);

  /* run(): Unit = val s = "x"; val b = true; print(s); return */
  AstNode **callArgs = malloc(sizeof(AstNode *));
  callArgs[0] = ast_new_identifier(dup_str("s"), 11);

  AstNode **mainBody = malloc(4 * sizeof(AstNode *));
  mainBody[0] = ast_new_var_decl(dup_str("s"), false, T_STRING, false,
                                 ast_new_string(dup_str("x"), 10), 10);
  mainBody[1] = ast_new_var_decl(dup_str("b"), false, T_BOOLEAN, false,
                                 ast_new_bool(true, 11), 11);
  mainBody[2] = ast_new_func_call(dup_str("print"), callArgs, 1, 12);
  mainBody[3] = ast_new_return(NULL, 13);

  AstNode **funcs = malloc(2 * sizeof(AstNode *));
  funcs[0] = ast_new_func_def(dup_str("factorial"), false, T_INT, params, 1,
                              ast_new_block(funcBody, 4, 1), 1);
  funcs[1] = ast_new_func_def(dup_str("run"), true, T_UNIT, NULL, 0,
                              ast_new_block(mainBody, 4, 9), 9);

  AstNode *program = ast_new_program(funcs, 2, 0);

  /* exercise remaining expression nodes so every case is covered */
  AstNode **extra = malloc(2 * sizeof(AstNode *));
  extra[0] = ast_new_expr_statement(
      ast_new_unary(OP_UNARY_MINUS, ast_new_double(2.5, 14), 14), 14);
  extra[1] = ast_new_expr_statement(ast_new_unit(15), 15);
  AstNode *extraBlock = ast_new_block(extra, 2, 14);
  AstNode **funcs2 = malloc(sizeof(AstNode *));
  funcs2[0] = ast_new_func_def(dup_str("extra"), false, T_UNIT, NULL, 0,
                               extraBlock, 14);
  AstNode *program2 = ast_new_program(funcs2, 1, 0);

  printf("=== AST 1 ===\n");
  ast_print(program, 0);
  printf("=== AST 2 ===\n");
  ast_print(program2, 0);

  ast_free(program);
  ast_free(program2);
}
