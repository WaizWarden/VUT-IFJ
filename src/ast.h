/**
 * @file    ast.h
 * @author  xhorakj01
 * @brief   AST header file.
 */

#ifndef AST_H
#define AST_H

#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  AST_PROGRAM,
  AST_FUNC_DEF,
  AST_BLOCK,
  AST_VAR_DECL,
  AST_ASSIGN,
  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_RETURN,
  AST_FUNC_CALL,
  AST_EXPR_STATEMENT,
  AST_BINARY,
  AST_UNARY,
  AST_IDENTIFIER,
  AST_LITERAL_INT,
  AST_LITERAL_DOUBLE,
  AST_LITERAL_STRING,
  AST_LITERAL_BOOL,
  AST_LITERAL_UNIT
} AstNodeType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_LE,
  OP_GT,
  OP_GE,
  OP_AND,
  OP_OR
} BinaryOperation;

typedef enum { OP_UNARY_MINUS, OP_UNARY_NOT } UnaryOperation;

typedef struct AstNode {
  AstNodeType type;
  int line;
  uint32_t uid;
  DataType evalType;
  union {
    struct {
      BinaryOperation operation;
      struct AstNode *left;
      struct AstNode *right;
    } binary;

    struct {
      UnaryOperation op;
      struct AstNode *operand;
    } unary;

    struct {
      char *name;
      struct AstNode *value;
    } assign;

    struct {
      struct AstNode **functions;
      size_t count;
    } program;

    struct {
      struct AstNode **stmts;
      size_t count;
    } block;

    struct {
      char *name;
      bool isModifiable;
      DataType declaredType;
      bool hasDeclaredType;
      struct AstNode *initExpr;
    } varDecl;

    struct {
      char *name;
      bool isMain;
      DataType retType;
      Parameter *params;
      uint16_t paramCount;
      struct AstNode *body;
    } funcDef;

    struct {
      char *funcName;
      struct AstNode **args;
      size_t argCount;
    } funcCall;

    struct {
      struct AstNode *condition;
      struct AstNode *thenBlock;
      struct AstNode *elseBlock;
    } ifStatement;

    struct {
      struct AstNode *cond;
      struct AstNode *bodyBlock;
    } whileStatement;

    struct {
      char *varName;
      uint32_t varUid;
      struct AstNode *from;
      struct AstNode *to;
      struct AstNode *step; // NULL = 1
      bool inclusive;
      struct AstNode **guards;
      size_t guardCount;
      struct AstNode *body;
    } forStatement;

    struct {
      struct AstNode *expr;
    } ret;

    struct {
      struct AstNode *expr;
    } exprStatement;

    int intValue;
    double doubleValue;
    char *identif;
    char *stringValue;
    bool boolValue;
  } as;
} AstNode;

/**
 * @brief Creates a binary operation node (takes ownership of children)
 */
AstNode *ast_new_binary(BinaryOperation op, AstNode *left, AstNode *right,
                        int line);

AstNode *ast_new_unary(UnaryOperation op, AstNode *operand, int line);
AstNode *ast_new_int(int value, int line);
AstNode *ast_new_double(double value, int line);
AstNode *ast_new_identifier(char *name, int line); // steals `name`
AstNode *ast_new_assign(char *name, AstNode *value, int line);

AstNode *ast_new_program(AstNode **functions, size_t count, int line);
AstNode *ast_new_func_def(char *name, bool isMain, DataType retType,
                          Parameter *params, uint16_t paramCount, AstNode *body,
                          int line);
AstNode *ast_new_block(AstNode **stmts, size_t count, int line);
AstNode *ast_new_var_decl(char *name, bool isModifiable, DataType declaredType,
                          bool hasDeclaredType, AstNode *initExpr, int line);
AstNode *ast_new_if(AstNode *cond, AstNode *thenBlock, AstNode *elseBlock,
                    int line);
AstNode *ast_new_while(AstNode *cond, AstNode *bodyBlock, int line);
AstNode *ast_new_for(char *varName, AstNode *from, AstNode *to, AstNode *step,
                     bool inclusive, AstNode **guards, size_t guardCount,
                     AstNode *body, int line);
AstNode *ast_new_return(AstNode *expr, int line);
AstNode *ast_new_func_call(char *funcName, AstNode **args, size_t argCount,
                           int line);
AstNode *ast_new_expr_statement(AstNode *expr, int line);
AstNode *ast_new_string(char *value, int line); // steals `value`
AstNode *ast_new_bool(bool value, int line);
AstNode *ast_new_unit(int line);

void ast_free(AstNode *node);
void ast_print(const AstNode *node, int indent);

#endif // AST_H
