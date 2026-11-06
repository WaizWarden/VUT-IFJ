/**
 * @file    ast.c
 * @author  xhorakj01
 * @brief   AST implementation
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

/**
 * @brief Allocates memory for AstNode.
 * @param AstNodeType type
 * @param int line
 * @returns Pointer to allocated memory.
 */

static AstNode *ast_allocate(AstNodeType type, int line) {
  AstNode *node = malloc(sizeof(AstNode));

  if (node == NULL) {
    exit(INTERPRET_ERROR);
  }

  node->type = type;
  node->line = line;

  return node;
}

/**
 * @brief Creates a new binary operation AST node
 * @param BinaryOperation operation
 * @param AstNode *right
 * @param AstNode * left
 * @param int line
 *
 * @returns pointer to AstNode
 */

AstNode *ast_new_binary(BinaryOperation operation, AstNode *left,
                        AstNode *right, int line) {
  AstNode *node = ast_allocate(AST_BINARY, line);

  node->as.binary.operation = operation;
  node->as.binary.left = left;
  node->as.binary.right = right;

  return node;
}

/**
 * @brief Creates a new unary operation AST node
 * @param UnaryOperation operation
 * @param AstNode *operand
 * @param int line
 *
 * @returns pointer to AstNode
 */
AstNode *ast_new_unary(UnaryOperation operation, AstNode *operand, int line) {
  AstNode *node = ast_allocate(AST_UNARY, line);

  node->as.unary.op = operation;
  node->as.unary.operand = operand;

  return node;
}

/**
 * @brief Creates a new Literal Int operation AST node
 * @param int value
 * @param int line
 *
 * @returns pointer to AstNode
 */

AstNode *ast_new_int(int value, int line) {
  AstNode *node = ast_allocate(AST_LITERAL_INT, line);

  node->as.intValue = value;

  return node;
}

/**
 * @brief Creates a new Double Literal operation AST node
 * @param double value
 * @param int line
 *
 * @returns pointer to AstNode
 */

AstNode *ast_new_double(double value, int line) {
  AstNode *node = ast_allocate(AST_LITERAL_DOUBLE, line);

  node->as.doubleValue = value;

  return node;
}

/**
 * @brief Creates a new Identifier operation AST node
 * @param char *name
 * @param int line
 *
 * @returns pointer to AstNode
 */

AstNode *ast_new_identifier(char *name, int line) {
  AstNode *node = ast_allocate(AST_IDENTIFIER, line);

  node->as.identif = name;

  return node;
}

/**
 * @brief Free function of each node type
 * @param AstNode *node
 *
 * @returns null -> void
 */
void ast_free(AstNode *node) {
  if (node == NULL) {
    return;
  }

  switch (node->type) {
  case AST_BINARY:
    ast_free(node->as.binary.left);
    ast_free(node->as.binary.right);
    break;
  case AST_UNARY:
    ast_free(node->as.unary.operand);
    break;
  case AST_IDENTIFIER:
    free(node->as.identif);
    break;
  case AST_ASSIGN:
    free(node->as.assign.name);
    ast_free(node->as.assign.value);
    break;
  case AST_PROGRAM:
    for (size_t i = 0; i < node->as.program.count; i++) {
      ast_free(node->as.program.functions[i]);
    }

    free(node->as.program.functions);
    break;
  case AST_FUNC_DEF:
    free(node->as.funcDef.name);
    for (uint16_t i = 0; i < node->as.funcDef.paramCount; i++) {
      free(node->as.funcDef.params[i].name);
    }

    free(node->as.funcDef.params);
    ast_free(node->as.funcDef.body);
    break;
  case AST_BLOCK:
    for (size_t i = 0; i < node->as.block.count; i++) {
      ast_free(node->as.block.stmts[i]);
    }

    free(node->as.block.stmts);
    break;
  case AST_VAR_DECL:
    free(node->as.varDecl.name);
    ast_free(node->as.varDecl.initExpr);
    break;
  case AST_IF:
    ast_free(node->as.ifStatement.condition);
    ast_free(node->as.ifStatement.thenBlock);
    ast_free(node->as.ifStatement.elseBlock);
    break;
  case AST_WHILE:
    ast_free(node->as.whileStatement.cond);
    ast_free(node->as.whileStatement.bodyBlock);
    break;
  case AST_RETURN:
    ast_free(node->as.ret.expr); /* NULL-safe, ast_free umí NULL */
    break;
  case AST_FUNC_CALL:
    free(node->as.funcCall.funcName);
    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      ast_free(node->as.funcCall.args[i]);
    }
    free(node->as.funcCall.args);
    break;
  case AST_EXPR_STATEMENT:
    ast_free(node->as.exprStatement.expr);
    break;
  case AST_LITERAL_STRING:
    free(node->as.stringValue);
    break;
  default:
    // If literals -> nothing to free
    break;
  }

  free(node);
}

/**
 * @brief Printing the AST tree recursively
 * @param const AstNode *node
 * @param int indent
 */

void ast_print(const AstNode *node, int indent) {
  if (!node) {
    return;
  }

  for (int i = 0; i < indent; i++) {
    printf(" ");
  }

  switch (node->type) {
  case AST_BINARY:
    printf("Binary %d\n", node->as.binary.operation);
    ast_print(node->as.binary.left, indent + 1);
    ast_print(node->as.binary.right, indent + 1);
    break;
  case AST_UNARY:
    printf("Unary %d\n", node->as.unary.op);
    ast_print(node->as.unary.operand, indent + 1);
    break;
  case AST_LITERAL_INT:
    printf("Literal Int %d\n", node->as.intValue);
    break;
  case AST_LITERAL_DOUBLE:
    printf("Literal double %f\n", node->as.doubleValue);
    break;
  case AST_IDENTIFIER:
    printf("Identifier %s\n", node->as.identif);
    break;
  case AST_ASSIGN:
    printf("Assign %s\n", node->as.assign.name);
    ast_print(node->as.assign.value, indent + 1);
    break;
  }
}
/**
 * @brief node for assigning
 * @param char *name
 * @param AstNode *value
 * @paramint line
 * @return node
 */
AstNode *ast_new_assign(char *name, AstNode *value, int line) {
  AstNode *node = ast_allocate(AST_ASSIGN, line);

  node->as.assign.name = name;
  node->as.assign.value = value;

  return node;
}
/**
 * @brief Creates a new node for if block
 *
 * @param cond
 * @param thenBlock
 * @param elseBlock
 * @param line
 * @return AstNode*
 */

AstNode *ast_new_if(AstNode *cond, AstNode *thenBlock, AstNode *elseBlock,
                    int line) {

  AstNode *node = ast_allocate(AST_IF, line);
  node->as.ifStatement.condition = cond;
  node->as.ifStatement.elseBlock = elseBlock;
  node->as.ifStatement.thenBlock = thenBlock;

  return node;
}
