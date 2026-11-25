/**
 * @file    ast.c
 * @author  xhorakj01
 * @brief   AST implementation
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  node->uid = 0;
  node->evalType = T_UNIT;

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

    free((void *)node->as.program.functions);
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

    free((void *)node->as.block.stmts);
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
    ast_free(node->as.ret.expr);
    break;
  case AST_FUNC_CALL:
    free(node->as.funcCall.funcName);
    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      ast_free(node->as.funcCall.args[i]);
    }
    free((void *)node->as.funcCall.args);
    break;
  case AST_EXPR_STATEMENT:
    ast_free(node->as.exprStatement.expr);
    break;
  case AST_LITERAL_STRING:
    free(node->as.stringValue);
    break;
  case AST_FOR:
    free(node->as.forStatement.varName);
    ast_free(node->as.forStatement.from);
    ast_free(node->as.forStatement.to);
    ast_free(node->as.forStatement.step);

    for (size_t i = 0; i < node->as.forStatement.guardCount; i++) {
      ast_free(node->as.forStatement.guards[i]);
    }

    free((void *)node->as.forStatement.guards);
    ast_free(node->as.forStatement.body);
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
  case AST_PROGRAM:
    printf("Program (%zu functions)\n", node->as.program.count);
    for (size_t i = 0; i < node->as.program.count; i++) {
      ast_print(node->as.program.functions[i], indent + 1);
    }
    break;
  case AST_FUNC_DEF:
    printf("FuncDef %s retType=%d isMain=%d params=%u\n", node->as.funcDef.name,
           node->as.funcDef.retType, (int)node->as.funcDef.isMain,
           node->as.funcDef.paramCount);
    ast_print(node->as.funcDef.body, indent + 1);
    break;
  case AST_BLOCK:
    printf("Block (%zu stmts)\n", node->as.block.count);
    for (size_t i = 0; i < node->as.block.count; i++) {
      ast_print(node->as.block.stmts[i], indent + 1);
    }
    break;
  case AST_VAR_DECL:
    printf("VarDecl %s %s\n", node->as.varDecl.name,
           (int)node->as.varDecl.isModifiable ? "var" : "val");
    ast_print(node->as.varDecl.initExpr, indent + 1);
    break;
  case AST_IF:
    printf("If\n");
    ast_print(node->as.ifStatement.condition, indent + 1);
    ast_print(node->as.ifStatement.thenBlock, indent + 1);
    ast_print(node->as.ifStatement.elseBlock, indent + 1);
    break;
  case AST_WHILE:
    printf("While\n");
    ast_print(node->as.whileStatement.cond, indent + 1);
    ast_print(node->as.whileStatement.bodyBlock, indent + 1);
    break;
  case AST_RETURN:
    printf("Return\n");
    ast_print(node->as.ret.expr, indent + 1);
    break;
  case AST_FUNC_CALL:
    printf("Call %s (%zu args)\n", node->as.funcCall.funcName,
           node->as.funcCall.argCount);
    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      ast_print(node->as.funcCall.args[i], indent + 1);
    }
    break;
  case AST_EXPR_STATEMENT:
    printf("ExprStmt\n");
    ast_print(node->as.exprStatement.expr, indent + 1);
    break;
  case AST_LITERAL_STRING:
    printf("Literal String \"%s\"\n", node->as.stringValue);
    break;
  case AST_LITERAL_BOOL:
    printf("Literal Bool %s\n", (int)node->as.boolValue ? "true" : "false");
    break;
  case AST_LITERAL_UNIT:
    printf("Literal Unit ()\n");
    break;
  case AST_FOR:
    printf("For %s inclusive=%d guards=%zu\n", node->as.forStatement.varName,
           (int)node->as.forStatement.inclusive,
           node->as.forStatement.guardCount);
    ast_print(node->as.forStatement.from, indent + 1);
    ast_print(node->as.forStatement.to, indent + 1);
    ast_print(node->as.forStatement.step, indent + 1);
    for (size_t i = 0; i < node->as.forStatement.guardCount; i++) {
      ast_print(node->as.forStatement.guards[i], indent + 1);
    }

    ast_print(node->as.forStatement.body, indent + 1);
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
/**
 * @brief Creates a new node for program
 *
 * @param functions
 * @param count
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_program(AstNode **functions, size_t count, int line) {
  AstNode *node = ast_allocate(AST_PROGRAM, line);
  node->as.program.count = count;
  node->as.program.functions = functions;

  return node;
}

/**
 * @brief Creates a new node for function definition
 *
 * @param name
 * @param isMain
 * @param retType
 * @param params
 * @param paramCount
 * @param body
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_func_def(char *name, bool isMain, DataType retType,
                          Parameter *params, uint16_t paramCount, AstNode *body,
                          int line) {

  AstNode *node = ast_allocate(AST_FUNC_DEF, line);
  node->as.funcDef.name = name;
  node->as.funcDef.isMain = isMain;
  node->as.funcDef.paramCount = paramCount;
  node->as.funcDef.params = params;
  node->as.funcDef.retType = retType;
  node->as.funcDef.body = body;

  return node;
}

/**
 * @brief Creates a new node for block
 *
 * @param stmts
 * @param count
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_block(AstNode **stmts, size_t count, int line) {
  AstNode *node = ast_allocate(AST_BLOCK, line);

  node->as.block.count = count;
  node->as.block.stmts = stmts;

  return node;
}

/**
 * @brief Creates a new node for variable declaration
 *
 * @param name
 * @param isModifiable
 * @param declaredType
 * @param hasDeclaredType
 * @param initExpr
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_var_decl(char *name, bool isModifiable, DataType declaredType,
                          bool hasDeclaredType, AstNode *initExpr, int line) {

  AstNode *node = ast_allocate(AST_VAR_DECL, line);
  node->as.varDecl.declaredType = declaredType;
  node->as.varDecl.name = name;
  node->as.varDecl.isModifiable = isModifiable;
  node->as.varDecl.hasDeclaredType = hasDeclaredType;
  node->as.varDecl.initExpr = initExpr;

  return node;
}

/**
 * @brief Creates a new node for while loop
 *
 * @param cond
 * @param bodyBlock
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_while(AstNode *cond, AstNode *bodyBlock, int line) {
  AstNode *node = ast_allocate(AST_WHILE, line);
  node->as.whileStatement.bodyBlock = bodyBlock;
  node->as.whileStatement.cond = cond;

  return node;
}

/**
 * @brief Creates a new node for return statement
 *
 * @param expr
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_return(AstNode *expr, int line) {
  AstNode *node = ast_allocate(AST_RETURN, line);
  node->as.ret.expr = expr;

  return node;
}

/**
 * @brief Creates a new node for function call
 *
 * @param funcName
 * @param args
 * @param argCount
 * @param line
 * @return AstNode*
 */

/**
 * @brief Creates a new node for for loop
 *
 * @param varName
 * @param from
 * @param toExpr
 * @param step NULL = default step 1
 * @param inclusive true = to (right bound included), false = until (excluded)
 * @param guards
 * @param guardCount
 * @param body
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_for(char *varName, AstNode *from, AstNode *toExpr,
                     AstNode *step, bool inclusive, AstNode **guards,
                     size_t guardCount, AstNode *body, int line) {
  AstNode *node = ast_allocate(AST_FOR, line);
  node->as.forStatement.varName = varName;
  node->as.forStatement.varUid = 0;
  node->as.forStatement.from = from;
  node->as.forStatement.to = toExpr;
  node->as.forStatement.step = step;
  node->as.forStatement.inclusive = inclusive;
  node->as.forStatement.guards = guards;
  node->as.forStatement.guardCount = guardCount;
  node->as.forStatement.body = body;

  return node;
}

AstNode *ast_new_func_call(char *funcName, AstNode **args, size_t argCount,
                           int line) {
  AstNode *node = ast_allocate(AST_FUNC_CALL, line);
  node->as.funcCall.funcName = funcName;
  node->as.funcCall.args = args;
  node->as.funcCall.argCount = argCount;

  return node;
}

/**
 * @brief Desugars member access `receiver.member` into a builtin call
 *
 * @param receiver
 * @param memberName
 * @param line
 * @return AstNode* AST_FUNC_CALL node
 */
AstNode *ast_new_member_call(AstNode *receiver, char *memberName, int line) {
  static const struct {
    const char *member;
    const char *builtin;
  } memberMap[] = {{"toString", "toStr"},
                   {"toInt", "toInt"},
                   {"floor", "$floor"},
                   {"length", "strLength"}};

  char *callName = memberName;

  for (size_t i = 0; i < sizeof(memberMap) / sizeof(memberMap[0]); i++) {
    if (strcmp(memberName, memberMap[i].member) == 0) {
      callName = strdup(memberMap[i].builtin);
      free(memberName);
      if (callName == NULL) {
        exit(INTERPRET_ERROR);
      }
      break;
    }
  }

  // Unknown members must not become calls to user defined functions.
  if (callName == memberName) {
    free(memberName);
    callName = strdup("$valmem_unknown");
    if (callName == NULL) {
      exit(INTERPRET_ERROR);
    }
  }

  AstNode **args = (AstNode **)malloc(sizeof(*args));
  if (args == NULL) {
    exit(INTERPRET_ERROR);
  }
  args[0] = receiver;

  return ast_new_func_call(callName, args, 1, line);
}

/**
 * @brief Creates a new node for expresion statement
 *
 * @param expr
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_expr_statement(AstNode *expr, int line) {
  AstNode *node = ast_allocate(AST_EXPR_STATEMENT, line);
  node->as.exprStatement.expr = expr;

  return node;
}

/**
 * @brief Creates a new node for string
 *
 * @param value
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_string(char *value, int line) {
  AstNode *node = ast_allocate(AST_LITERAL_STRING, line);
  node->as.stringValue = value;

  return node;
}

/**
 * @brief Creates a new node for boolean
 *
 * @param value
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_bool(bool value, int line) {
  AstNode *node = ast_allocate(AST_LITERAL_BOOL, line);
  node->as.boolValue = value;

  return node;
}

/**
 * @brief Creates a new node for unit
 *
 * @param line
 * @return AstNode*
 */
AstNode *ast_new_unit(int line) {
  AstNode *node = ast_allocate(AST_LITERAL_UNIT, line);

  return node;
}
