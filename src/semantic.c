/**
 * @file    semantic.c
 * @author  xsirmat00 xjuricl01
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
static SemanticError analyze_block_body(SemanticContext *ctx,
                                        AstNode **bodySlot, bool isFuncBody,
                                        DataType expectedRet);
static SemanticError analyze_if(SemanticContext *ctx, AstNode *node,
                                bool asValue);
static SemanticError analyze_for(SemanticContext *ctx, AstNode *node);
static bool always_returns(AstNode *node);

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
 * @brief Returns the type a statement yields when used as the last value of a
 * block or branch (declarations, assignments and loops yield Unit)
 *
 * @param node Statement or expression AST node
 * @return Value type of the node
 */
static DataType stmt_value_type(const AstNode *node) {
  switch (node->type) {
  case AST_VAR_DECL:
  case AST_ASSIGN:
  case AST_WHILE:
  case AST_FOR:
  case AST_RETURN:
    return T_UNIT;
  default:
    return node->evalType;
  }
}

/**
 * @brief Converts the Int value produced by a statement/branch to Double in
 * place (recurses through blocks and if branches) and updates evalType
 *
 * @param node Statement, block or if node whose value type is Int
 */
static void coerce_to_double(AstNode *node) {
  if (!node || stmt_value_type(node) != T_INT) {
    return;
  }

  switch (node->type) {
  case AST_EXPR_STATEMENT:
    node->as.exprStatement.expr =
        promote_int_to_double(node->as.exprStatement.expr);
    break;
  case AST_BLOCK:
    if (node->as.block.count > 0) {
      coerce_to_double(node->as.block.stmts[node->as.block.count - 1]);
    }
    break;
  case AST_IF:
    coerce_to_double(node->as.ifStatement.thenBlock);
    coerce_to_double(node->as.ifStatement.elseBlock);
    break;
  default:
    return;
  }
  node->evalType = T_DOUBLE;
}

/**
 * @brief Checks whether an expression is a literal zero (also `-0`, `0.0`)
 *
 * @param node Expression AST node (may be NULL)
 * @return True if the node is a zero literal
 */
static bool is_zero_literal(const AstNode *node) {
  if (!node) {
    return false;
  }
  if (node->type == AST_LITERAL_INT) {
    return node->as.intValue == 0;
  }
  if (node->type == AST_LITERAL_DOUBLE) {
    return node->as.doubleValue == 0.0;
  }
  if (node->type == AST_UNARY && node->as.unary.op == OP_UNARY_MINUS) {
    return is_zero_literal(node->as.unary.operand);
  }
  return false;
}

/**
 * @brief Checks that the last value of a function body matches the return type
 * (Int is promoted to Double where the node can be rewritten in place)
 *
 * @param lastSlot Pointer to the last statement of the body (or to the
 * one-line body itself), so a raw expression can be replaced by its promotion
 * @param expectedRet Declared return type of the function
 * @return SemanticError status code
 */
static SemanticError check_tail_value(AstNode **lastSlot, DataType expectedRet) {
  AstNode *last = *lastSlot;
  if (last->type == AST_RETURN || always_returns(last)) {
    return ERR_SEM_OK;
  }

  DataType actualRet = stmt_value_type(last);
  if (actualRet == expectedRet) {
    return ERR_SEM_OK;
  }

  if (expectedRet == T_DOUBLE && actualRet == T_INT) {
    if (last->type == AST_EXPR_STATEMENT || last->type == AST_IF ||
        last->type == AST_BLOCK) {
      coerce_to_double(last);
    } else {
      // raw expression body, e.g. `def f(x: Int): Double = x`
      *lastSlot = promote_int_to_double(last);
    }
    return ERR_SEM_OK;
  }
  return ERR_SEM_WRONG_TYPE;
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

  // floor(value: Double): Double (target of the `.floor` member desugar)
  Parameter *floorParams = create_builtin_params(1, T_DOUBLE);
  if (!st_insert_func(symtable, "floor", T_DOUBLE, floorParams, 1, false,
                      true)) {
    return ERR_INTERNAL;
  }

  // Extensions

  // readBoolean(): Boolean
  if (!st_insert_func(symtable, "readBoolean", T_BOOLEAN, NULL, 0, false,
                      true)) {
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
    if (operation == OP_AND || operation == OP_OR) {
      // BOOTHEN: logical operators take and produce Boolean
      if (leftT != T_BOOLEAN || rightT != T_BOOLEAN) {
        return ERR_SEM_WRONG_TYPE;
      }
      node->evalType = T_BOOLEAN;
    } else if (operation == OP_LT || operation == OP_LE || operation == OP_GT ||
               operation == OP_GE || operation == OP_EQ ||
               operation == OP_NEQ) {
      // Booleans can only be compared for (in)equality
      if ((leftT == T_BOOLEAN || rightT == T_BOOLEAN) && operation != OP_EQ &&
          operation != OP_NEQ) {
        return ERR_SEM_WRONG_TYPE;
      }
      if (leftT != rightT && !((leftT == T_INT && rightT == T_DOUBLE) ||
                               (leftT == T_DOUBLE && rightT == T_INT))) {
        return ERR_SEM_WRONG_TYPE;
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
    } else {
      // BOOTHEN: logical negation needs a Boolean operand
      if (operandT != T_BOOLEAN) {
        return ERR_SEM_WRONG_TYPE;
      }
      node->evalType = T_BOOLEAN;
    }
    break;
  }

  case AST_IF:
    // BOOTHEN: if used as an expression, branches must unify
    err = analyze_if(ctx, node, true);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;

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
 * @param bodySlot Pointer to the block or statement AST node (a raw
 * expression body may be replaced by its Int->Double promotion)
 * @param isFuncBody True if body belongs directly to a function definition
 * @param expectedRet Expected return type of the enclosing function
 * @return SemanticError status code
 */
static SemanticError analyze_block_body(SemanticContext *ctx,
                                        AstNode **bodySlot, bool isFuncBody,
                                        DataType expectedRet) {
  AstNode *body = *bodySlot;

  // empty function
  if (!body) {
    // def f(): T_UNIT
    if (isFuncBody && expectedRet != T_UNIT) {
      return ERR_SEM_WRONG_TYPE;
    }
    return ERR_SEM_OK;
  }

  // one line function
  if (body->type != AST_BLOCK) {
    SemanticError err = analyze_node(ctx, body);
    if (err != ERR_SEM_OK) {
      return err;
    }

    // body ends with value, check its type
    if (isFuncBody && expectedRet != T_UNIT) {
      return check_tail_value(bodySlot, expectedRet);
    }
    return ERR_SEM_OK;
  }

  // inside if, else ...
  if (!isFuncBody) {
    if (!st_push_scope(ctx->symtable)) {
      return ERR_INTERNAL;
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

  // block value: type of the last statement, Unit if empty
  body->evalType = (body->as.block.count > 0)
                       ? stmt_value_type(
                             body->as.block.stmts[body->as.block.count - 1])
                       : T_UNIT;

  if (isFuncBody && expectedRet != T_UNIT) {
    if (body->as.block.count == 0) {
      return ERR_SEM_WRONG_TYPE;
    }

    // body ends with value, check its type
    err = check_tail_value(&body->as.block.stmts[body->as.block.count - 1],
                           expectedRet);
    if (err != ERR_SEM_OK) {
      return err;
    }
    if (body->evalType != expectedRet) {
      body->evalType = expectedRet;
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
 * @brief Analyzes one branch of an if, keeping else-if chains in the same mode
 *
 * @param ctx Semantic context
 * @param branch Branch AST node (may be NULL)
 * @param asValue True if the whole if is used as an expression
 * @return SemanticError status code
 */
static SemanticError analyze_branch(SemanticContext *ctx, AstNode *branch,
                                    bool asValue) {
  if (branch && branch->type == AST_IF) {
    return analyze_if(ctx, branch, asValue);
  }
  return analyze_node(ctx, branch);
}

/**
 * @brief Analyzes an if (statement or expression), checks its guard is Boolean
 * and unifies the branch types into node->evalType
 *
 * @param ctx Semantic context
 * @param node AST_IF node
 * @param asValue True if the if is used as an expression (types must unify)
 * @return SemanticError status code
 */
static SemanticError analyze_if(SemanticContext *ctx, AstNode *node,
                                bool asValue) {
  DataType condT = T_UNIT;
  SemanticError err =
      analyze_expr(ctx, node->as.ifStatement.condition, &condT);
  if (err != ERR_SEM_OK) {
    return err;
  }

  if (condT != T_BOOLEAN) {
    return ERR_SEM_WRONG_TYPE;
  }

  AstNode *thenB = node->as.ifStatement.thenBlock;
  AstNode *elseB = node->as.ifStatement.elseBlock;

  err = analyze_branch(ctx, thenB, asValue);
  if (err != ERR_SEM_OK) {
    return err;
  }

  err = analyze_branch(ctx, elseB, asValue);
  if (err != ERR_SEM_OK) {
    return err;
  }

  node->evalType = T_UNIT;
  if (!thenB || !elseB) {
    return ERR_SEM_OK;
  }

  bool thenRet = always_returns(thenB);
  bool elseRet = always_returns(elseB);
  DataType thenT = stmt_value_type(thenB);
  DataType elseT = stmt_value_type(elseB);

  if (thenRet && elseRet) {
    node->evalType = T_UNIT;
  } else if (thenRet) {
    node->evalType = elseT;
  } else if (elseRet || thenT == elseT) {
    node->evalType = thenT;
  } else if ((thenT == T_INT && elseT == T_DOUBLE) ||
             (thenT == T_DOUBLE && elseT == T_INT)) {
    coerce_to_double(thenB);
    coerce_to_double(elseB);
    node->evalType = T_DOUBLE;
  } else if (asValue) {
    return ERR_SEM_WRONG_TYPE;
  }
  return ERR_SEM_OK;
}

/**
 * @brief Analyzes guards and body of a for loop inside its own scope
 *
 * @param ctx Semantic context
 * @param node AST_FOR node
 * @return SemanticError status code
 */
static SemanticError analyze_for_scoped(SemanticContext *ctx, AstNode *node) {
  uint32_t loopUid = ++ctx->varUidCounter;
  node->as.forStatement.varUid = loopUid;

  // CONTROL: loop variable is immutable, so writing to it is error 7
  if (!st_insert_var(ctx->symtable, node->as.forStatement.varName, T_INT,
                     false)) {
    return ERR_SEM_REDEFINITION;
  }
  if (!ctx_push_var_uid(ctx, node->as.forStatement.varName, loopUid)) {
    return ERR_INTERNAL;
  }

  for (size_t idx = 0; idx < node->as.forStatement.guardCount; idx++) {
    DataType guardT = T_UNIT;
    SemanticError err =
        analyze_expr(ctx, node->as.forStatement.guards[idx], &guardT);
    if (err != ERR_SEM_OK) {
      return err;
    }
    if (guardT != T_BOOLEAN) {
      return ERR_SEM_WRONG_TYPE;
    }
  }

  return analyze_node(ctx, node->as.forStatement.body);
}

/**
 * @brief Analyzes a for loop (bounds and step are Int, a literal zero step is
 * error 10, guards must be Boolean, the control variable is immutable)
 *
 * @param ctx Semantic context
 * @param node AST_FOR node
 * @return SemanticError status code
 */
static SemanticError analyze_for(SemanticContext *ctx, AstNode *node) {
  // bounds and step are evaluated outside the scope of the loop variable
  DataType fromT = T_UNIT;
  DataType toT = T_UNIT;
  DataType stepT = T_INT;

  SemanticError err = analyze_expr(ctx, node->as.forStatement.from, &fromT);
  if (err != ERR_SEM_OK) {
    return err;
  }
  err = analyze_expr(ctx, node->as.forStatement.to, &toT);
  if (err != ERR_SEM_OK) {
    return err;
  }
  if (node->as.forStatement.step) {
    err = analyze_expr(ctx, node->as.forStatement.step, &stepT);
    if (err != ERR_SEM_OK) {
      return err;
    }
  }

  if (fromT != T_INT || toT != T_INT || stepT != T_INT) {
    return ERR_SEM_WRONG_TYPE;
  }
  if (is_zero_literal(node->as.forStatement.step)) {
    return ERR_SEM_OTHER;
  }

  if (!st_push_scope(ctx->symtable)) {
    return ERR_INTERNAL;
  }
  ctx->scopeDepth++;

  err = analyze_for_scoped(ctx, node);

  st_pop_scope(ctx->symtable);
  ctx->scopeDepth--;
  ctx_pop_scope_uids(ctx);

  node->evalType = T_UNIT;
  return err;
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
    return analyze_block_body(ctx, &node, false, T_UNIT);

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

    if (node->as.varDecl.hasDeclaredType && node->as.varDecl.initExpr &&
        varT != initT) {
      if (varT == T_DOUBLE && initT == T_INT) {
        node->as.varDecl.initExpr =
            promote_int_to_double(node->as.varDecl.initExpr);
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
    }

    uint32_t newUid = ++ctx->varUidCounter;
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

  case AST_IF:
    err = analyze_if(ctx, node, false);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;

  case AST_FOR:
    err = analyze_for(ctx, node);
    if (err != ERR_SEM_OK) {
      return err;
    }
    break;

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

    if (ctx->currentRetType != T_UNIT) {
      if (!node->as.ret.expr) {
        return ERR_SEM_WRONG_TYPE;
      }

      if (ctx->currentRetType != exprT) {
        if (ctx->currentRetType == T_DOUBLE && exprT == T_INT) {
          node->as.ret.expr = promote_int_to_double(node->as.ret.expr);
        } else {
          return ERR_SEM_WRONG_TYPE;
        }
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
    return analyze_expr(ctx, node, &node->evalType);
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

  int mainCount = 0;
  // loop through functions
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
        params[parIdx].uid = 0; // temporary
      }
    }

    if (!st_insert_func(symtable, fnNode->as.funcDef.name,
                        fnNode->as.funcDef.retType, params,
                        fnNode->as.funcDef.paramCount,
                        fnNode->as.funcDef.isMain, true)) {
      return ERR_SEM_REDEFINITION;
    }

    if (fnNode->as.funcDef.isMain) {
      mainCount++;

      if (mainCount > 1) {
        return ERR_SEM_REDEFINITION;
      }

      if (fnNode->as.funcDef.paramCount > 0) {
        return ERR_SEM_FUNCTION_PARAMS;
      }

      if (fnNode->as.funcDef.retType != T_UNIT) {
        return ERR_SEM_WRONG_TYPE;
      }
    }
  }

  if (mainCount == 0) {
    return ERR_SEM_UNDEFINED_SYMBOL;
  }

  SemanticContext ctx = {.symtable = symtable,
                         .currentRetType = T_UNIT,
                         .varUidCounter = 0,
                         .scopeDepth = 0,
                         .uidStack = NULL,
                         .uidCount = 0,
                         .uidCapacity = 0,
                         .inMain = false,
  };

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
      uint32_t paramUid = ++ctx.varUidCounter;
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

    err = analyze_block_body(&ctx, &fnNode->as.funcDef.body, true,
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

/**
 * @brief Helper function that checks if all branches end with return statement
 */
static bool always_returns(AstNode *node) {
  if (!node) {
    return false;
  }

  switch (node->type) {

  case AST_RETURN:
    return true;

  case AST_BLOCK:
    if (node->as.block.count == 0) {
      return true;
    }
    return always_returns(node->as.block.stmts[node->as.block.count - 1]);

  case AST_IF:
    if (!always_returns(node->as.ifStatement.thenBlock)) {
      return false;
    }
    return always_returns(node->as.ifStatement.elseBlock);

  default:
    return false;
  }
}