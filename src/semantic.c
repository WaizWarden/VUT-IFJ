/**
 * @file    semantic.c
 * @author  xsirmat00
 * @brief   Implementation of semantic analysis
 */

#include "semantic.h"
#include "symtable.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *name;
  uint32_t uid;
  uint32_t depth;
} VarUidEntry;

typedef struct {
  Symtable *symtable;
  DataType currentRetType;
  uint32_t varUidCounter;
  uint32_t scopeDepth;
  VarUidEntry *uidStack;
  size_t uidCount;
  size_t uidCapacity;
  bool inMain;
} SemanticContext;

static SemanticError analyze_node(SemanticContext *ctx, AstNode *node);
static SemanticError analyze_expr(SemanticContext *ctx, AstNode *node,
                                  DataType *outType);
static SemanticError analyze_block_body(SemanticContext *ctx, AstNode *body,
                                        bool isFuncBody, DataType expectedRet);

/**
 * @brief Wraps an Int AST expression node in `expr + 0.0` so codegen emits
 * INT2FLOATS
 *
 * @param expr Original Int expression node
 * @return Double-typed AST binary addition node
 */
static AstNode *promote_int_to_double(AstNode *expr) {
  AstNode *zeroDouble = ast_new_double(0.0, expr->line);
  zeroDouble->evalType = T_DOUBLE;
  AstNode *promoted = ast_new_binary(OP_ADD, expr, zeroDouble, expr->line);
  promoted->evalType = T_DOUBLE;
  return promoted;
}

/**
 * @brief Records variable UID in current lexical scope depth
 *
 * @param ctx Semantic context
 * @param name Variable identifier
 * @param uid Unique variable identifier
 * @return True on success, false on allocation failure
 */
static bool ctx_push_var_uid(SemanticContext *ctx, const char *name,
                             uint32_t uid) {
  if (ctx->uidCount >= ctx->uidCapacity) {
    size_t newCap = (ctx->uidCapacity == 0) ? 32 : ctx->uidCapacity * 2;
    VarUidEntry *newStack =
        realloc(ctx->uidStack, newCap * sizeof(VarUidEntry));
    if (!newStack) {
      return false;
    }
    ctx->uidStack = newStack;
    ctx->uidCapacity = newCap;
  }

  ctx->uidStack[ctx->uidCount].name = name;
  ctx->uidStack[ctx->uidCount].uid = uid;
  ctx->uidStack[ctx->uidCount].depth = ctx->scopeDepth;
  ctx->uidCount++;
  return true;
}

/**
 * @brief Looks up innermost variable UID by name
 *
 * @param ctx Semantic context
 * @param name Variable identifier
 * @return Variable UID or 0 if not found
 */
static uint32_t ctx_lookup_var_uid(const SemanticContext *ctx,
                                   const char *name) {
  for (size_t idx = ctx->uidCount; idx > 0; idx--) {
    if (strcmp(ctx->uidStack[idx - 1].name, name) == 0) {
      return ctx->uidStack[idx - 1].uid;
    }
  }
  return 0;
}

/**
 * @brief Removes variable UIDs belonging to scopes deeper than current
 *
 * @param ctx Semantic context
 */
static void ctx_pop_scope_uids(SemanticContext *ctx) {
  while (ctx->uidCount > 0 &&
         ctx->uidStack[ctx->uidCount - 1].depth > ctx->scopeDepth) {
    ctx->uidCount--;
  }
}

/**
 * @brief Helper for creating parameter arrays for built-in function definitions
 *
 * @param count Number of parameters
 * @return Dynamically allocated array of Parameter structs
 */
static Parameter *create_builtin_params(size_t count, ...) {
  if (count == 0) {
    return NULL;
  }

  Parameter *params = calloc(count, sizeof(Parameter));
  if (!params) {
    return NULL;
  }

  va_list args;
  va_start(args, count);
  for (size_t idx = 0; idx < count; idx++) {
    params[idx].name = strdup("param");
    params[idx].type = va_arg(args, DataType);
    params[idx].uid = 0;
  }
  va_end(args);
  return params;
}

/**
 * @brief Registers all built-in functions into symtable matching codegen.c
 *
 * @param symtable Global symbol table
 * @return ERR_SEM_OK on success or error code
 */
static SemanticError register_builtins(Symtable *symtable) {

  //  print(value: Any): Unit
  Parameter *printParams = create_builtin_params(1, T_ANY);
  if (!st_insert_func(symtable, "print", T_UNIT, printParams, 1, false, true)) {
    return ERR_INTERNAL;
  }

  // readLine(): String
  if (!st_insert_func(symtable, "readLine", T_STRING, NULL, 0, false, true)) {
    return ERR_INTERNAL;
  }

  // readInt(): Int
  if (!st_insert_func(symtable, "readInt", T_INT, NULL, 0, false, true)) {
    return ERR_INTERNAL;
  }

  // readDouble(): Double
  if (!st_insert_func(symtable, "readDouble", T_DOUBLE, NULL, 0, false, true)) {
    return ERR_INTERNAL;
  }

  // toStr(value: Any): String
  Parameter *toStrParams = create_builtin_params(1, T_ANY);
  if (!st_insert_func(symtable, "toStr", T_STRING, toStrParams, 1, false,
                      true)) {
    return ERR_INTERNAL;
  }

  // toInt(value: Double): Int
  Parameter *toIntParams = create_builtin_params(1, T_DOUBLE);
  if (!st_insert_func(symtable, "toInt", T_INT, toIntParams, 1, false, true)) {
    return ERR_INTERNAL;
  }

  // strLength(value: String): Int
  Parameter *lenParams = create_builtin_params(1, T_STRING);
  if (!st_insert_func(symtable, "strLength", T_INT, lenParams, 1, false,
                      true)) {
    return ERR_INTERNAL;
  }

  // subStr(value: String, start: Int, end: Int): String
  Parameter *subParams = create_builtin_params(3, T_STRING, T_INT, T_INT);
  if (!st_insert_func(symtable, "subStr", T_STRING, subParams, 3, false,
                      true)) {
    return ERR_INTERNAL;
  }

  // ord(value: String, index: Int): Int
  Parameter *ordParams = create_builtin_params(2, T_STRING, T_INT);
  if (!st_insert_func(symtable, "ord", T_INT, ordParams, 2, false, true)) {
    return ERR_INTERNAL;
  }

  // chr(code: Int): String
  Parameter *chrParams = create_builtin_params(1, T_INT);
  if (!st_insert_func(symtable, "chr", T_STRING, chrParams, 1, false, true)) {
    return ERR_INTERNAL;
  }

  return ERR_SEM_OK;
}

/**
 * @brief Analyzes expressions and sets node evalType and uid
 *
 * @param ctx Semantic context
 * @param node Expression AST node
 * @param outType Output parameter for deduced DataType
 * @return SemanticError status code
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
    Symbol *sym = st_get(ctx->symtable, node->as.identif);
    if (!sym || sym->type != ST_VAR) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }
    node->evalType = sym->data.var.type;
    node->uid = ctx_lookup_var_uid(ctx, node->as.identif);
    break;
  }

  case AST_BINARY: {
    DataType leftT = T_UNIT;
    DataType rightT = T_UNIT;

    err = analyze_expr(ctx, node->as.binary.left, &leftT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    err = analyze_expr(ctx, node->as.binary.right, &rightT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    BinaryOperation operation = node->as.binary.operation;
    if (operation == OP_LT || operation == OP_LE || operation == OP_GT ||
        operation == OP_GE || operation == OP_EQ || operation == OP_NEQ) {
      if (leftT != rightT) {
        if (!((leftT == T_INT && rightT == T_DOUBLE) ||
              (leftT == T_DOUBLE && rightT == T_INT))) {
          return ERR_SEM_WRONG_TYPE;
        }
      }
      node->evalType = T_BOOLEAN;
    } else {
      if (leftT == T_STRING && rightT == T_STRING && operation == OP_ADD) {
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
    DataType operandT = T_UNIT;
    err = analyze_expr(ctx, node->as.unary.operand, &operandT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (node->as.unary.op == OP_UNARY_MINUS) {
      if (operandT != T_INT && operandT != T_DOUBLE) {
        return ERR_SEM_WRONG_TYPE;
      }
      node->evalType = operandT;
    }
    break;
  }

  case AST_FUNC_CALL: {

    // left side of =
    const char *funcName = node->as.funcCall.funcName;
    Symbol *funcSym = st_get(ctx->symtable, funcName);
    if (!funcSym || funcSym->type != ST_FUNC) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }

    if (node->as.funcCall.argCount != funcSym->data.func.paramCount) {
      return ERR_SEM_FUNCTION_PARAMS;
    }

    for (size_t idx = 0; idx < node->as.funcCall.argCount; idx++) {
      DataType argT = T_UNIT;
      err = analyze_expr(ctx, node->as.funcCall.args[idx], &argT);
      if (err != ERR_SEM_OK) {
        return err;
      }

      DataType paramT = funcSym->data.func.parameters[idx].type;
      if (paramT != T_ANY && argT != paramT) {
        if (paramT == T_DOUBLE && argT == T_INT) {
          node->as.funcCall.args[idx] =
              promote_int_to_double(node->as.funcCall.args[idx]);
        } else {
          return ERR_SEM_FUNCTION_PARAMS;
        }
      }
    }

    node->evalType = funcSym->data.func.returnType;
    break;
  }

  default:
    return ERR_SEM_OTHER;
  }

  *outType = node->evalType;
  return ERR_SEM_OK;
}

/**
 * @brief Analyzes a block or single statement and checks return value if needed
 *
 * @param ctx Semantic context
 * @param body Block or statement AST node
 * @param isFuncBody True if body belongs directly to a function definition
 * @param expectedRet Expected return type of the enclosing function
 * @return SemanticError status code
 */
static SemanticError analyze_block_body(SemanticContext *ctx, AstNode *body,
                                        bool isFuncBody, DataType expectedRet) {
  if (!body) {
    if (isFuncBody && expectedRet != T_UNIT) {
      return ERR_SEM_FUNCTION_PARAMS;
    }
    return ERR_SEM_OK;
  }

  if (body->type != AST_BLOCK) {
    SemanticError err = analyze_node(ctx, body);
    if (err != ERR_SEM_OK) {
      return err;
    }
    if (isFuncBody && expectedRet != T_UNIT && body->type != AST_RETURN) {
      AstNode *exprNode = (body->type == AST_EXPR_STATEMENT)
                              ? body->as.exprStatement.expr
                              : body;
      DataType actualRet = exprNode->evalType;
      if (actualRet != expectedRet) {
        if (expectedRet == T_DOUBLE && actualRet == T_INT &&
            body->type == AST_EXPR_STATEMENT) {
          body->as.exprStatement.expr = promote_int_to_double(exprNode);
        } else {
          return ERR_SEM_FUNCTION_PARAMS;
        }
      }
    }
    return ERR_SEM_OK;
  }

  if (!isFuncBody) {
    if (!st_push_scope(ctx->symtable)) {
      return ERR_SEM_OTHER;
    }
    ctx->scopeDepth++;
  }

  SemanticError err = ERR_SEM_OK;
  for (size_t idx = 0; idx < body->as.block.count; idx++) {
    err = analyze_node(ctx, body->as.block.stmts[idx]);
    if (err != ERR_SEM_OK) {
      if (!isFuncBody) {
        st_pop_scope(ctx->symtable);
        ctx->scopeDepth--;
        ctx_pop_scope_uids(ctx);
      }
      return err;
    }
  }

  if (isFuncBody && expectedRet != T_UNIT) {
    if (body->as.block.count == 0) {
      return ERR_SEM_FUNCTION_PARAMS;
    }
    AstNode *last = body->as.block.stmts[body->as.block.count - 1];
    if (last->type != AST_RETURN) {
      AstNode *exprNode = (last->type == AST_EXPR_STATEMENT)
                              ? last->as.exprStatement.expr
                              : last;
      DataType lastType = exprNode->evalType;
      if (lastType != expectedRet) {
        if (expectedRet == T_DOUBLE && lastType == T_INT &&
            last->type == AST_EXPR_STATEMENT) {
          last->as.exprStatement.expr = promote_int_to_double(exprNode);
        } else {
          return ERR_SEM_FUNCTION_PARAMS;
        }
      }
    }
  }

  if (!isFuncBody) {
    st_pop_scope(ctx->symtable);
    ctx->scopeDepth--;
    ctx_pop_scope_uids(ctx);
  }

  return ERR_SEM_OK;
}

/**
 * @brief Analyzes statements in block or function body
 *
 * @param ctx Semantic context
 * @param node Statement or expression AST node
 * @return SemanticError status code
 */
static SemanticError analyze_node(SemanticContext *ctx, AstNode *node) {
  if (!node) {
    return ERR_SEM_OK;
  }

  SemanticError err = ERR_SEM_OK;

  switch (node->type) {
  case AST_BLOCK:
    return analyze_block_body(ctx, node, false, T_UNIT);

  case AST_VAR_DECL: {
    DataType initT = T_UNIT;
    if (node->as.varDecl.initExpr) {
      err = analyze_expr(ctx, node->as.varDecl.initExpr, &initT);
      if (err != ERR_SEM_OK) {
        return err;
      }
    }

    DataType varT = node->as.varDecl.hasDeclaredType
                        ? node->as.varDecl.declaredType
                        : initT;

    if (node->as.varDecl.hasDeclaredType && node->as.varDecl.initExpr) {
      if (varT != initT) {
        if (varT == T_DOUBLE && initT == T_INT) {
          node->as.varDecl.initExpr =
              promote_int_to_double(node->as.varDecl.initExpr);
        } else {
          return ERR_SEM_WRONG_TYPE;
        }
      }
    }

    uint32_t newUid = ++(ctx->varUidCounter);
    node->uid = newUid;

    if (!st_insert_var(ctx->symtable, node->as.varDecl.name, varT,
                       node->as.varDecl.isModifiable)) {
      return ERR_SEM_REDEFINITION;
    }

    if (!ctx_push_var_uid(ctx, node->as.varDecl.name, newUid)) {
      return ERR_SEM_OTHER;
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
      return ERR_SEM_IMMUTABLE;
    }

    DataType valT = T_UNIT;
    err = analyze_expr(ctx, node->as.assign.value, &valT);
    if (err != ERR_SEM_OK) {
      return err;
    }

    if (sym->data.var.type != valT) {
      if (sym->data.var.type == T_DOUBLE && valT == T_INT) {
        node->as.assign.value = promote_int_to_double(node->as.assign.value);
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
    }

    node->uid = ctx_lookup_var_uid(ctx, node->as.assign.name);
    node->evalType = sym->data.var.type;
    break;
  }

  case AST_IF: {
    DataType condT = T_UNIT;
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

    if (node->as.ifStatement.thenBlock && node->as.ifStatement.elseBlock) {
      DataType thenT = node->as.ifStatement.thenBlock->evalType;
      DataType elseT = node->as.ifStatement.elseBlock->evalType;
      if (thenT == elseT) {
        node->evalType = thenT;
      } else if ((thenT == T_INT && elseT == T_DOUBLE) ||
                 (thenT == T_DOUBLE && elseT == T_INT)) {
        node->evalType = T_DOUBLE;
      } else {
        node->evalType = T_UNIT;
      }
    } else {
      node->evalType = T_UNIT;
    }
    break;
  }

  case AST_WHILE: {
    DataType condT = T_UNIT;
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
    node->evalType = T_UNIT;
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

    if (ctx->currentRetType == T_UNIT) {
      if (node->as.ret.expr != NULL && exprT != T_UNIT) {
        return ERR_SEM_WRONG_TYPE;
      }
    } else if (ctx->currentRetType != exprT) {
      if (ctx->currentRetType == T_DOUBLE && exprT == T_INT &&
          node->as.ret.expr != NULL) {
        node->as.ret.expr = promote_int_to_double(node->as.ret.expr);
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
    }
    node->evalType = exprT;
    break;
  }

  case AST_EXPR_STATEMENT: {
    DataType exprT = T_UNIT;
    err = analyze_expr(ctx, node->as.exprStatement.expr, &exprT);
    if (err != ERR_SEM_OK) {
      return err;
    }
    node->evalType = exprT;
    break;
  }

  default:
    return analyze_expr(ctx, node, &(node->evalType));
  }

  return ERR_SEM_OK;
}

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (!symtable || !root || root->type != AST_PROGRAM) {
    return ERR_SEM_OTHER;
  }

  SemanticError err = register_builtins(symtable);
  if (err != ERR_SEM_OK) {
    return err;
  }

  bool hasMain = false;
  for (size_t idx = 0; idx < root->as.program.count; idx++) {
    AstNode *fnNode = root->as.program.functions[idx];
    if (fnNode->type != AST_FUNC_DEF) {
      return ERR_SEM_OTHER;
    }

    Parameter *params = NULL;
    if (fnNode->as.funcDef.paramCount > 0) {
      params = calloc(fnNode->as.funcDef.paramCount, sizeof(Parameter));
      if (!params) {
        return ERR_INTERNAL;
      }

      for (uint16_t parIdx = 0; parIdx < fnNode->as.funcDef.paramCount;
           parIdx++) {
        params[parIdx].name = strdup(fnNode->as.funcDef.params[parIdx].name);
        params[parIdx].type = fnNode->as.funcDef.params[parIdx].type;
        params[parIdx].uid = 0;
      }
    }

    if (!st_insert_func(symtable, fnNode->as.funcDef.name,
                        fnNode->as.funcDef.retType, params,
                        fnNode->as.funcDef.paramCount,
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

  SemanticContext ctx = {.symtable = symtable,
                         .currentRetType = T_UNIT,
                         .varUidCounter = 0,
                         .scopeDepth = 0,
                         .uidStack = NULL,
                         .uidCount = 0,
                         .uidCapacity = 0,
                         .inMain = false};

  for (size_t idx = 0; idx < root->as.program.count; idx++) {
    AstNode *fnNode = root->as.program.functions[idx];
    ctx.currentRetType = fnNode->as.funcDef.retType;
    ctx.inMain = fnNode->as.funcDef.isMain;

    if (!st_push_scope(symtable)) {
      free(ctx.uidStack);
      return ERR_SEM_OTHER;
    }
    ctx.scopeDepth = 1;

    for (uint16_t parIdx = 0; parIdx < fnNode->as.funcDef.paramCount;
         parIdx++) {
      uint32_t paramUid = ++(ctx.varUidCounter);
      fnNode->as.funcDef.params[parIdx].uid = paramUid;

      if (!st_insert_var(symtable, fnNode->as.funcDef.params[parIdx].name,
                         fnNode->as.funcDef.params[parIdx].type, false)) {
        st_pop_scope(symtable);
        free(ctx.uidStack);
        return ERR_SEM_REDEFINITION;
      }

      if (!ctx_push_var_uid(&ctx, fnNode->as.funcDef.params[parIdx].name,
                            paramUid)) {
        st_pop_scope(symtable);
        free(ctx.uidStack);
        return ERR_SEM_OTHER;
      }
    }

    err = analyze_block_body(&ctx, fnNode->as.funcDef.body, true,
                             fnNode->as.funcDef.retType);
    st_pop_scope(symtable);
    ctx.scopeDepth = 0;
    ctx_pop_scope_uids(&ctx);

    if (err != ERR_SEM_OK) {
      free(ctx.uidStack);
      return err;
    }
  }

  free(ctx.uidStack);
  return ERR_SEM_OK;
}
