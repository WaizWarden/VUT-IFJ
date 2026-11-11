/**
 * @file    semantic.c
 * @author  xsirmat00
 * @brief   Implementation of semantic analysis
 */

#include "semantic.h"
#include "symtable.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Symtable *symtable;
  DataType currentRetType;
  uint32_t varUidCounter;
  bool inMain;
} SemanticContext;

static SemanticError analyze_node(SemanticContext *ctx, AstNode *node);
static SemanticError analyze_expr(SemanticContext *ctx, AstNode *node,
                                   DataType *outType);

/**
 * @brief Helper for adding parameters to builtin function definitions.
 */
static Parameter *create_builtin_params(size_t count, ...) {
  if (count == 0) return NULL;
  Parameter *params = calloc(count, sizeof(Parameter));
  if (!params) return NULL;

  va_list args;
  va_start(args, count);
  for (size_t i = 0; i < count; i++) {
    params[i].name = strdup("param");
    params[i].type = va_arg(args, DataType);
    params[i].uid = 0;
  }
  va_end(args);
  return params;
}

/**
 * @brief Registers all built-in functions into symtable
 * @param Symtable *st
 * @returns SemanticError
 */
static SemanticError register_builtins(Symtable *st) {
  // readString(): String
  if (!st_insert_func(st, "readString", T_STRING, NULL, 0, false, true)) {
    return ERR_SEM_OTHER;
  }

  // readInt(): Int
  if (!st_insert_func(st, "readInt", T_INT, NULL, 0, false, true)) {
    return ERR_SEM_OTHER;
  }

  // readDouble(): Double
  if (!st_insert_func(st, "readDouble", T_DOUBLE, NULL, 0, false, true)) {
    return ERR_SEM_OTHER;
  }

  // write(term: Any): Unit
  Parameter *writeParams = create_builtin_params(1, T_ANY);
  if (!st_insert_func(st, "write", T_UNIT, writeParams, 1, false, true)) {
    return ERR_SEM_OTHER;
  }

  // toInt(term: Double): Int
  Parameter *toIntParams = create_builtin_params(1, T_DOUBLE);
  if (!st_insert_func(st, "toInt", T_INT, toIntParams, 1, false, true)) {
    return ERR_SEM_OTHER;
  }

  // toDouble(term: Int): Double
  Parameter *toDoubleParams = create_builtin_params(1, T_INT);
  if (!st_insert_func(st, "toDouble", T_DOUBLE, toDoubleParams, 1, false, true)) {
    return ERR_SEM_OTHER;
  }

  // length(s: String): Int
  Parameter *lenParams = create_builtin_params(1, T_STRING);
  if (!st_insert_func(st, "length", T_INT, lenParams, 1, false, true)) {
    return ERR_SEM_OTHER;
  }

  // substring(s: String, i: Int, j: Int): String
  Parameter *subParams = create_builtin_params(3, T_STRING, T_INT, T_INT);
  if (!st_insert_func(st, "substring", T_STRING, subParams, 3, false, true)) {
    return ERR_SEM_OTHER;
  }

  // ord(s: String, i: Int): Int
  Parameter *ordParams = create_builtin_params(2, T_STRING, T_INT);
  if (!st_insert_func(st, "ord", T_INT, ordParams, 2, false, true)) {
    return ERR_SEM_OTHER;
  }

  // chr(i: Int): String
  Parameter *chrParams = create_builtin_params(1, T_INT);
  if (!st_insert_func(st, "chr", T_STRING, chrParams, 1, false, true)) {
    return ERR_SEM_OTHER;
  }

  return ERR_SEM_OK;
}

/**
 * @brief Analyzes expressions and sets node evalType
 * @param SemanticContext *ctx
 * @param AstNode *node
 * @param DataType *outType
 * @returns SemanticError
 */
static SemanticError analyze_expr(SemanticContext *ctx, AstNode *node,
                                   DataType *outType) {
  if (!node) {
    *outType = T_UNIT;
    return ERR_SEM_OK;
  }

  SemanticError err = ERR_SEM_OK;

  switch (node->type) {
  case AST_LITERAL_INT:
    node->evalType = T_INT;
    break;

  case AST_LITERAL_DOUBLE:
    node->evalType = T_DOUBLE;
    break;

  case AST_LITERAL_STRING:
    node->evalType = T_STRING;
    break;

  case AST_LITERAL_BOOL:
    node->evalType = T_BOOLEAN;
    break;

  case AST_LITERAL_UNIT:
    node->evalType = T_UNIT;
    break;

  case AST_IDENTIFIER: {
    // Check if variable exists in symtable
    Symbol *sym = st_get(ctx->symtable, node->as.identif);
    if (!sym || sym->type != ST_VAR) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }
    node->evalType = sym->data.var.type;
    break;
  }

  case AST_BINARY: {
    DataType leftT, rightT;
    err = analyze_expr(ctx, node->as.binary.left, &leftT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    err = analyze_expr(ctx, node->as.binary.right, &rightT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    BinaryOperation op = node->as.binary.operation;

    // Relational operations
    if (op == OP_LT || op == OP_LE || op == OP_GT || op == OP_GE ||
        op == OP_EQ || op == OP_NEQ) {
      if (leftT != rightT) {
        // Allow implicit Int -> Double comparison
        if (!((leftT == T_INT && rightT == T_DOUBLE) ||
              (leftT == T_DOUBLE && rightT == T_INT))) {
          return ERR_SEM_WRONG_TYPE;
        }
      }
      node->evalType = T_BOOLEAN;
    } else {
      // Arithmetic operations
      if (leftT == T_STRING && rightT == T_STRING && op == OP_ADD) {
        node->evalType = T_STRING;
      } else if ((leftT == T_INT || leftT == T_DOUBLE) &&
                 (rightT == T_INT || rightT == T_DOUBLE)) {
        node->evalType =
            (leftT == T_DOUBLE || rightT == T_DOUBLE) ? T_DOUBLE : T_INT;
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
    }
    break;
  }

  case AST_UNARY: {
    DataType opT;
    err = analyze_expr(ctx, node->as.unary.operand, &opT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (node->as.unary.op == OP_UNARY_MINUS) {
      if (opT != T_INT && opT != T_DOUBLE) {
        return ERR_SEM_WRONG_TYPE;
      }
      node->evalType = opT;
    }
    break;
  }

  case AST_FUNC_CALL: {
    // Check if function is defined
    Symbol *fn = st_get(ctx->symtable, node->as.funcCall.funcName);
    if (!fn || fn->type != ST_FUNC) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }

    // Variadic check for builtin write function
    bool isWrite = (strcmp(node->as.funcCall.funcName, "write") == 0);
    if (!isWrite && node->as.funcCall.argCount != fn->data.func.paramCount) {
      return ERR_SEM_FUNCTION_PARAMS;
    }

    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      DataType argT;
      err = analyze_expr(ctx, node->as.funcCall.args[i], &argT);
      if (err != ERR_SEM_OK) {
        return err;
      }

      if (!isWrite) {
        DataType paramT = fn->data.func.parameters[i].type;
        if (argT != paramT) {
          // Allow Int -> Double implicit conversion
          if (!(paramT == T_DOUBLE && argT == T_INT)) {
            return ERR_SEM_FUNCTION_PARAMS;
          }
        }
      }
    }

    node->evalType = fn->data.func.returnType;
    break;
  }

  default:
    return ERR_SEM_OTHER;
  }

  *outType = node->evalType;
  return ERR_SEM_OK;
}

/**
 * @brief Analyzes statements in block or function body
 * @param SemanticContext *ctx
 * @param AstNode *node
 * @returns SemanticError
 */
static SemanticError analyze_node(SemanticContext *ctx, AstNode *node) {
  if (!node) {
    return ERR_SEM_OK;
  }

  SemanticError err = ERR_SEM_OK;

  switch (node->type) {
  case AST_BLOCK:
    st_push_scope(ctx->symtable);
    for (size_t i = 0; i < node->as.block.count; i++) {
      err = analyze_node(ctx, node->as.block.stmts[i]);
      if (err != ERR_SEM_OK) {
        st_pop_scope(ctx->symtable);
        return err;
      }
    }
    st_pop_scope(ctx->symtable);
    break;

  case AST_VAR_DECL: {
    DataType initT = T_UNIT;
    if (node->as.varDecl.initExpr) {
      err = analyze_expr(ctx, node->as.varDecl.initExpr, &initT);
      if (err != ERR_SEM_OK) {
        return err;
      }
    }

    DataType varT = node->as.varDecl.hasDeclaredType ? node->as.varDecl.declaredType : initT;

    if (node->as.varDecl.hasDeclaredType && initT != T_UNIT) {
      if (varT != initT && !(varT == T_DOUBLE && initT == T_INT)) {
        return ERR_SEM_WRONG_TYPE;
      }
    }

    // Generate unique UID for codegen
    node->uid = ++(ctx->varUidCounter);

    if (!st_insert_var(ctx->symtable, node->as.varDecl.name, varT, node->as.varDecl.isModifiable)) {
      return ERR_SEM_REDEFINITION;
    }

    // Assign UID to newly created symbol in current scope
    Symbol *sym = st_get(ctx->symtable, node->as.varDecl.name);
    if (sym) {
      sym->data.var.type = varT;
    }

    node->evalType = varT;
    break;
  }

  case AST_ASSIGN: {
    Symbol *sym = st_get(ctx->symtable, node->as.assign.name);
    if (!sym || sym->type != ST_VAR) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }
    if (!sym->data.var.modifiable) {
      return ERR_SEM_WRONG_TYPE;
    }

    DataType valT;
    err = analyze_expr(ctx, node->as.assign.value, &valT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (sym->data.var.type != valT && !(sym->data.var.type == T_DOUBLE && valT == T_INT)) {
      return ERR_SEM_WRONG_TYPE;
    }

    node->evalType = sym->data.var.type;
    break;
  }

  case AST_IF: {
    DataType condT;
    err = analyze_expr(ctx, node->as.ifStatement.condition, &condT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (condT != T_BOOLEAN) {
      return ERR_SEM_WRONG_TYPE;
    }

    err = analyze_node(ctx, node->as.ifStatement.thenBlock);
    if (err != ERR_SEM_OK) {
      return err;
    }

    err = analyze_node(ctx, node->as.ifStatement.elseBlock);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;
  }

  case AST_WHILE: {
    DataType condT;
    err = analyze_expr(ctx, node->as.whileStatement.cond, &condT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (condT != T_BOOLEAN) {
      return ERR_SEM_WRONG_TYPE;
    }

    err = analyze_node(ctx, node->as.whileStatement.bodyBlock);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;
  }

  case AST_RETURN: {
    DataType exprT = T_UNIT;
    if (node->as.ret.expr) {
      err = analyze_expr(ctx, node->as.ret.expr, &exprT);
      if (err != ERR_SEM_OK) {
        return err;
      }
    }

    if (ctx->currentRetType != exprT &&
        !(ctx->currentRetType == T_DOUBLE && exprT == T_INT)) {
      return ERR_SEM_FUNCTION_PARAMS;
    }
    break;
  }

  case AST_EXPR_STATEMENT: {
    DataType exprT;
    err = analyze_expr(ctx, node->as.exprStatement.expr, &exprT);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;
  }

  default:
    return analyze_expr(ctx, node, &(node->evalType));
  }

  return ERR_SEM_OK;
}

SemanticError analyze_semantics(Symtable *st, AstNode *root) {
  if (!st || !root || root->type != AST_PROGRAM) {
    return ERR_SEM_OTHER;
  }

  SemanticError err = register_builtins(st);
  if (err != ERR_SEM_OK) {
    return err;
  }

  // Pre-pass: collect function declarations into global symtable
  bool hasMain = false;
  for (size_t i = 0; i < root->as.program.count; i++) {
    AstNode *fnNode = root->as.program.functions[i];
    if (fnNode->type != AST_FUNC_DEF) {
      return ERR_SEM_OTHER;
    }

    Parameter *params = NULL;
    if (fnNode->as.funcDef.paramCount > 0) {
      params = calloc(fnNode->as.funcDef.paramCount, sizeof(Parameter));
      if (!params) return ERR_SEM_OTHER;

      for (uint16_t p = 0; p < fnNode->as.funcDef.paramCount; p++) {
        params[p].name = strdup(fnNode->as.funcDef.params[p].name);
        params[p].type = fnNode->as.funcDef.params[p].type;
        params[p].uid = 0;
      }
    }

    if (!st_insert_func(st, fnNode->as.funcDef.name, fnNode->as.funcDef.retType,
                        params, fnNode->as.funcDef.paramCount,
                        fnNode->as.funcDef.isMain, true)) {
      return ERR_SEM_REDEFINITION;
    }

    if (fnNode->as.funcDef.isMain ||
        strcmp(fnNode->as.funcDef.name, "main") == 0) {
      hasMain = true;
    }
  }

  if (!hasMain) {
    return ERR_SEM_UNDEFINED_SYMBOL;
  }

  // Main pass: analyze bodies of functions
  SemanticContext ctx = {.symtable = st,
                         .currentRetType = T_UNIT,
                         .varUidCounter = 0,
                         .inMain = false};

  for (size_t i = 0; i < root->as.program.count; i++) {
    AstNode *fnNode = root->as.program.functions[i];
    ctx.currentRetType = fnNode->as.funcDef.retType;
    ctx.inMain = fnNode->as.funcDef.isMain;

    st_push_scope(st);

    // Register function parameters in local scope
    for (uint16_t p = 0; p < fnNode->as.funcDef.paramCount; p++) {
      fnNode->as.funcDef.params[p].uid = ++(ctx.varUidCounter);

      if (!st_insert_var(st, fnNode->as.funcDef.params[p].name,
                         fnNode->as.funcDef.params[p].type, false)) {
        st_pop_scope(st);
        return ERR_SEM_REDEFINITION;
      }
    }

    err = analyze_node(&ctx, fnNode->as.funcDef.body);
    st_pop_scope(st);

    if (err != ERR_SEM_OK) {
      return err;
    }
  }

  return ERR_SEM_OK;
}