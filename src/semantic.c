/**
 * @file    semantic.c
 * @author  xsirmat00
<<<<<<< HEAD
 * @brief   Semantic analyzer implementation
=======
 * @brief   Implementation of semantic analysis
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
 */

#include "semantic.h"
#include "symtable.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

<<<<<<< HEAD
/// Helper function to register built-in functions into symtable
static bool semantic_register_builtins(Symtable *symtable) {
  // Built-in function signatures: st_insert_func(symtable, name, retType,
  // params, paramCount, isMain, isDefined)
  if (!st_insert_func(symtable, "readInt", T_INT, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "readDouble", T_DOUBLE, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "readLine", T_STRING, NULL, 0, false, true))
    return false;

  // BOOLTHEN extension
  if (!st_insert_func(symtable, "readBoolean", T_BOOLEAN, NULL, 0, false, true))
    return false;

  if (!st_insert_func(symtable, "print", T_UNIT, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "toStr", T_STRING, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "toInt", T_INT, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "chr", T_STRING, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "strLength", T_INT, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "ord", T_INT, NULL, 0, false, true))
    return false;
  if (!st_insert_func(symtable, "substr", T_STRING, NULL, 0, false, true))
    return false;
=======
typedef struct {
  Symtable *symtable;
  DataType currentRetType;
  uint32_t varUidCounter;
  bool inMain;
} SemanticContext;

static SemanticError analyze_node(SemanticContext *ctx, AstNode *node);
static SemanticError analyze_expr(SemanticContext *ctx, AstNode *node,
                                   DataType *outType);
>>>>>>> 393efe1936566121658fc1c91348025f48feb141

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

<<<<<<< HEAD
static SemanticError check_expression(Symtable *symtable, AstNode *node) {
  if (!node)
    return ERR_SEM_OK;
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141

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

<<<<<<< HEAD
  case AST_UNARY: {
    SemanticError err = check_expression(symtable, node->as.unary.operand);
    if (err != ERR_SEM_OK)
      return err;

    if (node->as.unary.op == OP_UNARY_MINUS) {
      if (node->as.unary.operand->evalType != T_INT &&
          node->as.unary.operand->evalType != T_DOUBLE) {
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
        return ERR_SEM_WRONG_TYPE;
      }
    }
    break;
  }

<<<<<<< HEAD
  case AST_BINARY: {
    SemanticError err_l = check_expression(symtable, node->as.binary.left);
    if (err_l != ERR_SEM_OK)
      return err_l;

    SemanticError err_r = check_expression(symtable, node->as.binary.right);
    if (err_r != ERR_SEM_OK)
      return err_r;

    DataType leftT = node->as.binary.left->evalType;
    DataType rightT = node->as.binary.right->evalType;

    switch (node->as.binary.operation) {
    case OP_ADD:
      if (leftT == T_STRING || rightT == T_STRING) {
        node->evalType = T_STRING;
      } else if (leftT == T_DOUBLE || rightT == T_DOUBLE) {
        node->evalType = T_DOUBLE;
      } else if (leftT == T_INT && rightT == T_INT) {
        node->evalType = T_INT;
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
      break;

    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
      if ((leftT == T_INT || leftT == T_DOUBLE) &&
          (rightT == T_INT || rightT == T_DOUBLE)) {
        if (leftT == T_DOUBLE || rightT == T_DOUBLE) {
          node->evalType = T_DOUBLE;
        } else {
          node->evalType = T_INT;
        }
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
      break;

    case OP_EQ:
    case OP_NEQ:
    case OP_LT:
    case OP_LE:
    case OP_GT:
    case OP_GE:
      if (leftT == rightT || ((leftT == T_INT || leftT == T_DOUBLE) &&
                              (rightT == T_INT || rightT == T_DOUBLE))) {
        node->evalType = T_BOOLEAN;
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
      break;

    default:
      break;
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
    }
    break;
  }

  case AST_FUNC_CALL: {
    // Check if function is defined
    Symbol *fn = st_get(ctx->symtable, node->as.funcCall.funcName);
    if (!fn || fn->type != ST_FUNC) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }

<<<<<<< HEAD
    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      SemanticError err = check_expression(symtable, node->as.funcCall.args[i]);
      if (err != ERR_SEM_OK)
        return err;
=======
    // Variadic check for builtin write function
    bool isWrite = (strcmp(node->as.funcCall.funcName, "write") == 0);
    if (!isWrite && node->as.funcCall.argCount != fn->data.func.paramCount) {
      return ERR_SEM_FUNCTION_PARAMS;
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
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
<<<<<<< HEAD
    // CONTROL extension: evaluate 'if' as an expression
    SemanticError err_cond =
        check_expression(symtable, node->as.ifStatement.condition);
    if (err_cond != ERR_SEM_OK)
      return err_cond;
=======
    DataType condT;
    err = analyze_expr(ctx, node->as.ifStatement.condition, &condT);
    if (err != ERR_SEM_OK) {
      return err;
    }
>>>>>>> 393efe1936566121658fc1c91348025f48feb141

    if (condT != T_BOOLEAN) {
      return ERR_SEM_WRONG_TYPE;
    }

<<<<<<< HEAD
    SemanticError err_then =
        check_expression(symtable, node->as.ifStatement.thenBlock);
    if (err_then != ERR_SEM_OK)
      return err_then;

    // BOOLTHEN extension: elseBlock is optional
    if (node->as.ifStatement.elseBlock) {
      SemanticError err_else =
          check_expression(symtable, node->as.ifStatement.elseBlock);
      if (err_else != ERR_SEM_OK)
        return err_else;
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141

  case AST_WHILE: {
    DataType condT;
    err = analyze_expr(ctx, node->as.whileStatement.cond, &condT);
    if (err != ERR_SEM_OK) {
      return err;
    }

<<<<<<< HEAD
      if (thenT == elseT) {
        node->evalType = thenT;
      } else if ((thenT == T_INT && elseT == T_DOUBLE) ||
                 (thenT == T_DOUBLE && elseT == T_INT)) {
        node->evalType = T_DOUBLE;
      } else {
        node->evalType = T_UNIT;
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
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

<<<<<<< HEAD
SemanticError analyze_semantics(Symtable *symtable, AstNode *ast) {
  if (!ast)
    return ERR_SEM_OK;

  // Register built-in functions
  semantic_register_builtins(symtable);

  switch (ast->type) {
  case AST_ASSIGN: {
    Symbol *sym = st_get(symtable, ast->as.assign.name);
    if (!sym || sym->type != ST_VAR) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }

    if (!sym->data.var.modifiable) {
      return ERR_SEM_OTHER; // Assignment to read-only variable (val)
    }

    SemanticError err = check_expression(symtable, ast->as.assign.value);
    if (err != ERR_SEM_OK)
      return err;

    DataType valType = ast->as.assign.value->evalType;

    if (sym->data.var.type != valType) {
      // Allow implicit Int -> Double conversion
      if (sym->data.var.type == T_DOUBLE && valType == T_INT) {
        ast->evalType = T_DOUBLE;
        return ERR_SEM_OK;
      }
      return ERR_SEM_WRONG_TYPE;
    }

    ast->evalType = sym->data.var.type;
    break;
  }

  case AST_VAR_DECL: {
    SemanticError err = check_expression(symtable, ast->as.varDecl.initExpr);
    if (err != ERR_SEM_OK)
      return err;

    DataType initType = ast->as.varDecl.initExpr->evalType;
    DataType declType = ast->as.varDecl.hasDeclaredType
                            ? ast->as.varDecl.declaredType
                            : initType;
=======
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
>>>>>>> 393efe1936566121658fc1c91348025f48feb141

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

<<<<<<< HEAD
    if (!st_insert_var(symtable, ast->as.varDecl.name, declType,
                       ast->as.varDecl.isModifiable)) {
=======
    if (!st_insert_func(st, fnNode->as.funcDef.name, fnNode->as.funcDef.retType,
                        params, fnNode->as.funcDef.paramCount,
                        fnNode->as.funcDef.isMain, true)) {
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
      return ERR_SEM_REDEFINITION;
    }

<<<<<<< HEAD
  case AST_BLOCK: {
    for (size_t i = 0; i < ast->as.block.count; i++) {
      SemanticError err = analyze_semantics(symtable, ast->as.block.stmts[i]);
      if (err != ERR_SEM_OK)
        return err;
=======
    if (fnNode->as.funcDef.isMain ||
        strcmp(fnNode->as.funcDef.name, "main") == 0) {
      hasMain = true;
>>>>>>> 393efe1936566121658fc1c91348025f48feb141
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