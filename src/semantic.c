/**
 * @file    semantic.c
 * @author  xjuricl01, xoramst00
 * @brief   Implementation of semantic analysis
 *
 * @note Main principles and architecture are inspired by "demo-cvičení" on
 * semantic analysis.
 *
 * @note Current AI use: As a second keyboard (writing repetetive code
 * continuaton)
 */

#include "semantic.h"
#include "ast.h"
#include "symtable.h"
#include <stdlib.h>
#include <string.h>

static SemanticError evaluate_function_headers(Symtable *symtable,
                                               AstNode *root);
static SemanticError register_builtin_functions(Symtable *symtable);
static SemanticError analyze_functions(Symtable *symtable, AstNode *root);
static Parameter *copy_params(const Parameter *src, uint16_t count);
static SemanticError analyze_statement(Symtable *symtable, AstNode *stmt,
                                       DataType *out);
static SemanticError analyze_expression(Symtable *symtable, AstNode *expr,
                                        DataType *out);
static AstNode *new_implicit_conversion(AstNode *operand);
static SemanticError resolve_binary_operands_type(AstNode *expr,
                                                  DataType *leftType,
                                                  DataType *rightType);
static bool always_returns(AstNode *node);
static bool is_numeric_type(DataType type);

/// @brief global uid counter
static uint32_t uidCounter;

/// @brief global return data type tracker
static DataType currentFuncRetType = T_UNIT;

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (root == NULL || root->type != AST_PROGRAM) {
    return ERR_OTHER;
  }

  uidCounter = 0;

  SemanticError error = evaluate_function_headers(symtable, root);
  if (error != ERR_OK) {
    return error;
  }

  SemanticError error2 = analyze_functions(symtable, root);
  if (error2 != ERR_OK) {
    return error2;
  }

  return ERR_OK;
}

/// @brief Analyzes all function heads and inserts them into symbol table
/// @param symtable symbol table
/// @param root AST root
/// @return Semantic error type
static SemanticError evaluate_function_headers(Symtable *symtable,
                                               AstNode *root) {

  // insertion of built-in functions into symtable
  // built-in functions and unser defined functions are all on global scope to
  // forbid redefinitions
  if (register_builtin_functions(symtable) != ERR_OK) {
    return ERR_OTHER;
  }

  bool checkedMain = false;

  // loop through all defined functions in program
  for (size_t i = 0; i < root->as.program.count; i++) {

    AstNode *functionNode = root->as.program.functions[i];

    if (functionNode->type != AST_FUNC_DEF) {
      return ERR_OTHER;
    }

    Parameter *paramCopy = NULL;

    // copy parameters so symtable has own data
    paramCopy = copy_params(functionNode->as.funcDef.params,
                            functionNode->as.funcDef.paramCount);

    if (!paramCopy && functionNode->as.funcDef.paramCount > 0) {
      return ERR_INTERNAL;
    }

    StInsertResult insertResult =
        st_insert_func(symtable, functionNode->as.funcDef.name,
                       functionNode->as.funcDef.retType, paramCopy,
                       functionNode->as.funcDef.paramCount,
                       functionNode->as.funcDef.isMain, true);

    if (insertResult == ST_INSERT_REDEFINED) {
      return ERR_REDEFINITION;
    }
    if (insertResult == ST_INSERT_ERROR) {
      return ERR_INTERNAL;
    }

    // validate main function
    if (functionNode->as.funcDef.isMain) {

      if (checkedMain) {
        return ERR_REDEFINITION;
      }

      if (functionNode->as.funcDef.paramCount > 0) {
        return ERR_INVALID_FUNCTION_PARAMS;
      }

      if (functionNode->as.funcDef.retType != T_UNIT) {
        return ERR_WRONG_TYPE;
      }

      checkedMain = true;
    }
  }

  // @main missing
  if (!checkedMain) {
    return ERR_UNDEFINED_SYMBOL;
  }
  return ERR_OK;
}

/// @brief Analyzes all function bodies
/// @param symtable symbol table
/// @param functionNode Function AST node
/// @return Semantic error type
static SemanticError analyze_functions(Symtable *symtable, AstNode *root) {

  // insert parameters to new scope for each function and analyze body
  for (size_t i = 0; i < root->as.program.count; i++) {

    AstNode *functionNode = root->as.program.functions[i];
    if (!st_push_scope(symtable)) {
      return ERR_INTERNAL;
    }

    // Add params from function
    for (uint16_t paramIdx = 0; paramIdx < functionNode->as.funcDef.paramCount;
         paramIdx++) {
      Parameter param = functionNode->as.funcDef.params[paramIdx];

      uint32_t newUid = ++uidCounter;
      functionNode->as.funcDef.params[paramIdx].uid = newUid;
      StInsertResult stResult =
          st_insert_var(symtable, param.name, param.type, false, newUid);
      if (stResult == ST_INSERT_REDEFINED) {
        st_pop_scope(symtable);
        return ERR_REDEFINITION;
      }
      if (stResult == ST_INSERT_ERROR) {
        st_pop_scope(symtable);
        return ERR_INTERNAL;
      }
    }

    AstNode *body = functionNode->as.funcDef.body;

    if (!body) {
      st_pop_scope(symtable);
      return ERR_OTHER;
    }

    DataType retType = functionNode->as.funcDef.retType;
    currentFuncRetType = retType;

    DataType bodyType = T_UNIT;
    bool hasReturn = false;

    if (body->type != AST_BLOCK) {
      // one-line function
      hasReturn = always_returns(body);
      SemanticError errCode =
          analyze_statement(symtable, body, hasReturn ? NULL : &bodyType);
      if (errCode != ERR_OK) {
        st_pop_scope(symtable);
        return errCode;
      }
    } else if (body->as.block.count > 0) {
      // analyze all statements except the last one
      for (size_t j = 0; j < body->as.block.count - 1; j++) {
        SemanticError errCode =
            analyze_statement(symtable, body->as.block.stmts[j], NULL);
        if (errCode != ERR_OK) {
          st_pop_scope(symtable);
          return errCode;
        }
      }

      // analyze the last statement (provides return value unless it always
      // returns)
      AstNode *lastStmt = body->as.block.stmts[body->as.block.count - 1];
      hasReturn = always_returns(lastStmt);
      SemanticError errCode =
          analyze_statement(symtable, lastStmt, hasReturn ? NULL : &bodyType);
      if (errCode != ERR_OK) {
        st_pop_scope(symtable);
        return errCode;
      }
    }

    // non-Unit function without guaranteed return must match bodyType
    if (retType != T_UNIT && !hasReturn && bodyType != retType &&
        !(bodyType == T_INT && retType == T_DOUBLE)) {
      st_pop_scope(symtable);
      return ERR_WRONG_TYPE;
    }

    st_pop_scope(symtable);
  }

  return ERR_OK;
}

/// @brief Analyzes one statement of a function body
/// @param symtable symbol table
/// @param stmt statement AST node
/// @return Semantic error type
static SemanticError analyze_statement(Symtable *symtable, AstNode *stmt,
                                       DataType *out) {

  SemanticError error = ERR_OK;
  if (out) {
    *out = T_UNIT;
  };

  switch (stmt->type) {
  case AST_BLOCK: {

    if (stmt->as.block.count == 0) {
      stmt->evalType = T_UNIT;
      break;
    }

    if (!st_push_scope(symtable)) {
      return ERR_INTERNAL;
    }

    for (size_t i = 0; i < stmt->as.block.count; i++) {

      error = analyze_statement(symtable, stmt->as.block.stmts[i], NULL);
      if (error != ERR_OK) {
        st_pop_scope(symtable);
        return error;
      }
    }

    st_pop_scope(symtable);

    const AstNode *last = stmt->as.block.stmts[stmt->as.block.count - 1];

    if (last->type == AST_EXPR_STATEMENT) {
      stmt->evalType = last->evalType;
    } else {
      stmt->evalType = T_UNIT;
    }

    if (out) {
      *out = stmt->evalType;
    }
    break;
  }

  case AST_EXPR_STATEMENT: {
    DataType exprType = T_UNIT;
    error =
        analyze_expression(symtable, stmt->as.exprStatement.expr, &exprType);
    if (error != ERR_OK) {
      return error;
    }

    stmt->evalType = exprType;
    if (out) {
      *out = exprType;
    }
    break;
  }

  case AST_WHILE: {

    // analyze condition
    DataType conditionType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.whileStatement.cond,
                               &conditionType);
    if (error != ERR_OK) {
      return error;
    }

    if (conditionType != T_BOOLEAN) {
      return ERR_WRONG_TYPE;
    }

    // analyze statement
    error =
        analyze_statement(symtable, stmt->as.whileStatement.bodyBlock, NULL);
    if (error != ERR_OK) {
      return error;
    }

    break;
  }

  case AST_IF: {

    // analyze condition
    DataType conditionType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.ifStatement.condition,
                               &conditionType);
    if (error != ERR_OK) {
      return error;
    }

    if (conditionType != T_BOOLEAN) {
      return ERR_WRONG_TYPE;
    }

    // analyze statement
    error = analyze_statement(symtable, stmt->as.ifStatement.thenBlock, NULL);
    if (error != ERR_OK) {
      return error;
    }

    if (stmt->as.ifStatement.elseBlock) {
      error = analyze_statement(symtable, stmt->as.ifStatement.elseBlock, NULL);
      if (error != ERR_OK) {
        return error;
      }
    }

    break;
  }

  case AST_VAR_DECL: {
    // evaluate the right side of var declaration
    DataType rightSideDatatype = T_UNIT;
    if (stmt->as.varDecl.initExpr) // null check
    {
      error = analyze_expression(symtable, stmt->as.varDecl.initExpr,
                                 &rightSideDatatype);
      if (error != ERR_OK) {
        return error;
      }
    }

    if (stmt->as.varDecl.hasDeclaredType) {
      if (rightSideDatatype == stmt->as.varDecl.declaredType) {
        stmt->evalType = rightSideDatatype;
      }

      // implicit conversion of int to double
      else if (stmt->as.varDecl.declaredType == T_DOUBLE &&
               rightSideDatatype == T_INT) {
        stmt->as.varDecl.initExpr =
            new_implicit_conversion(stmt->as.varDecl.initExpr);
        if (!stmt->as.varDecl.initExpr) {
          return ERR_INTERNAL;
        }
        stmt->evalType = T_DOUBLE;
      }

      // incompatible data types
      else {
        return ERR_WRONG_TYPE;
      }
    } else {
      stmt->evalType = rightSideDatatype;
    }

    // assign new uid and insert to symtable
    uint32_t newUid = ++uidCounter;
    stmt->uid = newUid;
    StInsertResult stResult =
        st_insert_var(symtable, stmt->as.varDecl.name, stmt->evalType,
                      stmt->as.varDecl.isModifiable, newUid);
    if (stResult == ST_INSERT_REDEFINED) {
      return ERR_REDEFINITION;
    }
    if (stResult == ST_INSERT_ERROR) {
      return ERR_INTERNAL;
    }

    break;
  }

  case AST_ASSIGN: {

    Symbol *symbol = st_get(symtable, stmt->as.assign.name);
    if (symbol == NULL || symbol->type != ST_VAR) {
      return ERR_UNDEFINED_SYMBOL;
    }

    if (!symbol->data.var.modifiable) {
      return ERR_IMMUTABLE;
    }

    DataType valueDataType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.assign.value, &valueDataType);
    if (error != ERR_OK) {
      return error;
    }

    if (valueDataType != symbol->data.var.type) {

      // convert int to double
      if (symbol->data.var.type == T_DOUBLE && valueDataType == T_INT) {
        stmt->as.assign.value = new_implicit_conversion(stmt->as.assign.value);
        if (!stmt->as.assign.value) {
          return ERR_INTERNAL;
        }
      }

      else {
        return ERR_WRONG_TYPE;
      }
    }

    stmt->uid = symbol->data.var.uid;
    stmt->evalType = symbol->data.var.type;
    break;
  }

  case AST_RETURN: {
    DataType retType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.ret.expr, &retType);
    if (error != ERR_OK) {
      return error;
    }

    // unit function
    if (currentFuncRetType == T_UNIT) {
      stmt->evalType = T_UNIT;
      break;
    }

    if (retType != currentFuncRetType) {

      // convert int to double
      if (currentFuncRetType == T_DOUBLE && retType == T_INT) {
        stmt->as.ret.expr = new_implicit_conversion(stmt->as.ret.expr);
        if (!stmt->as.ret.expr) {
          return ERR_INTERNAL;
        }
      }

      else {
        return ERR_WRONG_TYPE;
      }
    }

    // empty return is only allowed if function is T_UNIT
    if (stmt->as.ret.expr == NULL) {
      if (currentFuncRetType != T_UNIT) {
        return ERR_WRONG_TYPE;
      }
    }
    stmt->evalType = currentFuncRetType;
    break;
  }

  // CONTROL extension
  case AST_FOR: {

    // validate range
    DataType fromType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.forStatement.from, &fromType);
    if (error != ERR_OK) {
      return error;
    }

    DataType toType = T_UNIT;
    error = analyze_expression(symtable, stmt->as.forStatement.to, &toType);
    if (error != ERR_OK) {
      return error;
    }

    if (fromType != T_INT || fromType != T_INT) {
      return ERR_WRONG_TYPE;
    }

    // validate step
    if (stmt->as.forStatement.step) {
      DataType stepType = T_UNIT;
      error =
          analyze_expression(symtable, stmt->as.forStatement.step, &stepType);
      if (error != ERR_OK) {
        return error;
      }

      if (stepType != T_INT) {
        return ERR_WRONG_TYPE;
      }

      if (stepType == T_INT) {
        if (stmt->as.forStatement.step->as.intValue == 0) {
          return ERR_OTHER;
        }
      }

      // todo - error 40 or something idk
    }

    // required by assignement
    if (!st_push_scope(symtable)) {
      return ERR_INTERNAL;
    }

    // register loop variable
    uint32_t newUid = ++uidCounter;
    stmt->as.forStatement.varUid = newUid;

    StInsertResult stResult = st_insert_var(
        symtable, stmt->as.forStatement.varName, T_INT, false, newUid);

    if (error != ERR_OK) {
      st_pop_scope(symtable);
      return error;
    }

    if (stResult == ST_INSERT_ERROR) {
      st_pop_scope(symtable);
      return ERR_INTERNAL;
    }

    for (size_t i = 0; i < stmt->as.forStatement.guardCount; i++) {
      DataType guardType = T_UNIT;
      error =
          analyze_expression(symtable, stmt->as.forStatement.step, &guardType);

      if (error != ERR_OK) {
        return error;
      }

      if (error != T_BOOLEAN) {
        return error;
      }
    }

    // analyze body
    error = analyze_statement(symtable, stmt->as.forStatement.body, NULL);
    st_pop_scope(symtable);
    if (error != ERR_OK) {
      return error;
    }

    if (out) {
      *out = T_UNIT;
    }
    stmt->evalType = T_UNIT;
    break;
  }

  default: {
    // one-line functions

    DataType exprType = T_UNIT;
    error = analyze_expression(symtable, stmt, &exprType);
    if (error != ERR_OK) {
      return error;
    }

    if (out) {
      *out = exprType;
    }

    break;
  }
  }

  return ERR_OK;
}

/// @brief Analyzes expression and annotates its evaluation type
/// @param symtable symbol table
/// @param expr expression to analyze
/// @param out assigned by fucntion, data type of expression
/// @return ERR_OK or encounterd error
static SemanticError analyze_expression(Symtable *symtable, AstNode *expr,
                                        DataType *out) {
  if (expr == NULL) {
    *out = T_UNIT;
    return ERR_OK;
  }

  SemanticError error = ERR_OK;

  switch (expr->type) {
  case AST_LITERAL_INT:
    expr->evalType = T_INT;
    break;
  case AST_LITERAL_DOUBLE:
    expr->evalType = T_DOUBLE;
    break;
  case AST_LITERAL_STRING:
    expr->evalType = T_STRING;
    break;
  case AST_LITERAL_BOOL:
    expr->evalType = T_BOOLEAN;
    break;
  case AST_LITERAL_UNIT:
    expr->evalType = T_UNIT;
    break;

  case AST_IDENTIFIER: {
    Symbol *symbol = st_get(symtable, expr->as.identif);
    if (symbol == NULL || symbol->type != ST_VAR) {
      return ERR_UNDEFINED_SYMBOL;
    }

    expr->evalType = symbol->data.var.type;
    expr->uid = symbol->data.var.uid;
    break;
  }

  case AST_BINARY: {
    DataType leftType = T_UNIT;
    DataType rightType = T_UNIT;
    BinaryOperation operation = expr->as.binary.operation;

    error = analyze_expression(symtable, expr->as.binary.left, &leftType);
    if (error != ERR_OK) {
      return error;
    }
    error = analyze_expression(symtable, expr->as.binary.right, &rightType);
    if (error != ERR_OK) {
      return error;
    }

    switch (operation) {
    case OP_ADD:
      // concat strings
      if (leftType == T_STRING && rightType == T_STRING) {
        expr->evalType = T_STRING;
        break;
      }

      // convert types (if numeric)
      error = resolve_binary_operands_type(expr, &leftType, &rightType);
      if (error != ERR_OK) {
        return error;
      }
      expr->evalType = leftType;
      break;

    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
      error = resolve_binary_operands_type(expr, &leftType, &rightType);
      if (error != ERR_OK) {
        return error;
      }
      expr->evalType = leftType;
      break;

    case OP_LT:
    case OP_LE:
    case OP_GT:
    case OP_GE:
      // string compare
      if (leftType == T_STRING && rightType == T_STRING) {
        expr->evalType = T_BOOLEAN;
        break;
      }
      // convert types (if numeric)
      error = resolve_binary_operands_type(expr, &leftType, &rightType);
      if (error != ERR_OK) {
        return error;
      }
      expr->evalType = T_BOOLEAN;
      break;

    case OP_EQ:
    case OP_NEQ:
      // types must be equal or convertable to same type
      if (leftType != rightType) {
        error = resolve_binary_operands_type(expr, &leftType, &rightType);
        if (error != ERR_OK) {
          return error;
        }
      }
      expr->evalType = T_BOOLEAN;
      break;

    case OP_AND:
    case OP_OR:
      if (leftType != T_BOOLEAN || rightType != T_BOOLEAN) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = T_BOOLEAN;
      break;

    default:
      return ERR_INTERNAL;
    }
    break;
  }

  case AST_UNARY: {
    DataType operandType = T_UNIT;
    error = analyze_expression(symtable, expr->as.unary.operand, &operandType);
    if (error != ERR_OK) {
      return error;
    }

    switch (expr->as.unary.op) {
    case OP_UNARY_MINUS:
      if (!is_numeric_type(operandType)) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = operandType;
      break;

    case OP_UNARY_NOT:
      if (operandType != T_BOOLEAN) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = T_BOOLEAN;
      break;

    case OP_UNARY_INT2DOUBLE:
      if (operandType != T_INT) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = T_DOUBLE;
      break;
    default:
      return ERR_INTERNAL;
    }
    break;
  }

  case AST_FUNC_CALL: {

    // Verify that function and parameters are same as in symtable
    Symbol *sym = st_get(symtable, expr->as.funcCall.funcName);
    if (!sym || sym->type != ST_FUNC) {
      return ERR_UNDEFINED_SYMBOL;
    }
    if (sym->data.func.paramCount != expr->as.funcCall.argCount) {
      return ERR_INVALID_FUNCTION_PARAMS;
    }

    // Analyze parameters
    for (size_t i = 0; i < expr->as.funcCall.argCount; i++) {
      AstNode *arg = expr->as.funcCall.args[i];
      DataType resolvedType;
      error = analyze_expression(symtable, arg, &resolvedType);
      if (error != ERR_OK) {
        return error;
      }

      // Compare type from symtable and aplly conversions
      DataType paramType = sym->data.func.parameters[i].type;
      if (paramType != T_ANY && resolvedType != paramType) {
        if (paramType == T_DOUBLE && resolvedType == T_INT) {
          AstNode *converted = new_implicit_conversion(arg);
          if (!converted) {
            return ERR_INTERNAL;
          }
          expr->as.funcCall.args[i] = converted;
        } else {
          return ERR_INVALID_FUNCTION_PARAMS;
        }
      }
    }

    expr->evalType = sym->data.func.returnType;
    break;
  }

  // CONTROL extension
  case AST_IF: {
    // evaluate if condition valid
    DataType condType;
    error =
        analyze_expression(symtable, expr->as.ifStatement.condition, &condType);
    if (error != ERR_OK) {
      return error;
    }
    if (condType != T_BOOLEAN) {
      return ERR_WRONG_TYPE;
    }

    // else branch is mandatory
    if (!expr->as.ifStatement.elseBlock) {
      return ERR_OTHER;
    }

    DataType thenType = T_UNIT;
    DataType elseType = T_UNIT;
    // most of the time it will probably be an expression, but it can be block
    // also, so we need to call analyze statement
    error =
        analyze_statement(symtable, expr->as.ifStatement.thenBlock, &thenType);
    if (error != ERR_OK) {
      return error;
    }
    error =
        analyze_statement(symtable, expr->as.ifStatement.elseBlock, &elseType);
    if (error != ERR_OK) {
      return error;
    }

    // check if the types are valid
    if (thenType == elseType) {
      expr->evalType = thenType;
    } else if (is_numeric_type(thenType) && is_numeric_type(elseType)) {
      // codegen hadnles the conversion, because we cannot wrap a block node
      expr->evalType = T_DOUBLE;
    } else {
      return ERR_WRONG_TYPE;
    }

    break;
  }

  default:
    return ERR_OTHER;
  }

  *out = expr->evalType;
  return ERR_OK;
}

/// @brief Registeres all IFJcode26 built-in functions into symbol table
/// @param symtable symbol table
/// @return semantic error type
static SemanticError register_builtin_functions(Symtable *symtable) {

  Parameter *paramCopy;

  // print(val: any)
  const Parameter paramPrint = {"val", T_ANY, 0};
  paramCopy = copy_params(&paramPrint, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "print", T_UNIT, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // readLine(): String
  if (st_insert_func(symtable, "readLine", T_STRING, NULL, 0, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // readInt(): Int
  if (st_insert_func(symtable, "readInt", T_INT, NULL, 0, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // readDouble(): Double
  if (st_insert_func(symtable, "readDouble", T_DOUBLE, NULL, 0, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // toStr(value: Any): String
  const Parameter paramToStr = {"value", T_ANY, 0};
  paramCopy = copy_params(&paramToStr, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "toStr", T_STRING, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // toInt(value: Double): Int
  const Parameter paramToInt = {"value", T_DOUBLE, 0};
  paramCopy = copy_params(&paramToInt, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "toInt", T_INT, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // strLength(value: String): Int
  const Parameter paramStrLength = {"value", T_STRING, 0};
  paramCopy = copy_params(&paramStrLength, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "strLength", T_INT, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // subStr(value: String, start: Int, end: Int): String
  const Parameter paramSubStr[3] = {
      {"value", T_STRING, 0},
      {"start", T_INT, 0},
      {"end", T_INT, 0},
  };
  paramCopy = copy_params(paramSubStr, 3);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "subStr", T_STRING, paramCopy, 3, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // ord(value: String, index: Int): Int
  const Parameter paramOrd[2] = {
      {"value", T_STRING, 0},
      {"index", T_INT, 0},
  };
  paramCopy = copy_params(paramOrd, 2);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "ord", T_INT, paramCopy, 2, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // chr(code: Int): String
  const Parameter paramChr = {"code", T_INT, 0};
  paramCopy = copy_params(&paramChr, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "chr", T_STRING, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // readBool(): Boolean
  if (st_insert_func(symtable, "readBoolean", T_BOOLEAN, NULL, 0, false,
                     true) != ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  // floor(val: Double): Double
  const Parameter paramFloor = {"value", T_DOUBLE, 0};
  paramCopy = copy_params(&paramFloor, 1);
  if (!paramCopy) {
    return ERR_INTERNAL;
  }
  if (st_insert_func(symtable, "$floor", T_DOUBLE, paramCopy, 1, false, true) !=
      ST_INSERT_OK) {
    return ERR_INTERNAL;
  }

  return ERR_OK;
}

/// @brief Creates a deep copy of Parameter list, of n elements
/// @param src parameter array
/// @param count number of elements to copy
/// @return copy of parameter array
static Parameter *copy_params(const Parameter *src, uint16_t count) {
  if (count == 0) {
    return NULL;
  }

  Parameter *dst = calloc(count, sizeof(Parameter));
  if (!dst) {
    return NULL;
  }

  // copy names
  for (uint16_t i = 0; i < count; i++) {
    dst[i].name = strdup(src[i].name);

    // failed copy, free all previous parameters
    if (!dst[i].name) {
      for (uint16_t j = 0; j < i; j++) {
        free(dst[j].name);
      }
      free(dst);
      return NULL;
    }

    dst[i].type = src[i].type;
    dst[i].uid = src[i].uid;
  }

  return dst;
}

/// @brief Generates a new AST node for implicit int->double conversion and
/// automatically connects it to expression
/// @param operand AST node of expression to convert
/// @return new AST node
static AstNode *new_implicit_conversion(AstNode *operand) {
  if (!operand) {
    return NULL;
  }

  AstNode *newNode = ast_new_unary(OP_UNARY_INT2DOUBLE, operand, operand->line);
  if (!newNode) {
    return NULL;
  }
  newNode->evalType = T_DOUBLE;

  return newNode;
}

/// @brief Determines if the type is of T_INT or T_DOUBLE
/// @param type data type
/// @return true if numeric, false otherwise
static bool is_numeric_type(DataType type) {
  return type == T_INT || type == T_DOUBLE;
}

/// @brief Checks type compatibility and handles implicit conversion of int ->
/// double
/// @param expr AST node of expression
/// @param leftType data type of left operand
/// @param rightType data type of right operand
/// @return ERR_OK or encountered semantic error
static SemanticError resolve_binary_operands_type(AstNode *expr,
                                                  DataType *leftType,
                                                  DataType *rightType) {
  if (!is_numeric_type(*leftType) || !is_numeric_type(*rightType)) {
    return ERR_WRONG_TYPE;
  }

  // int + int or double + double
  if (*leftType == *rightType) {
    return ERR_OK;
  }

  // int + double
  if (*leftType == T_INT) {
    AstNode *converted = new_implicit_conversion(expr->as.binary.left);
    if (!converted) {
      return ERR_INTERNAL;
    }
    expr->as.binary.left = converted;
    *leftType = T_DOUBLE;
  }

  // double + int
  else {
    AstNode *converted = new_implicit_conversion(expr->as.binary.right);
    if (!converted) {
      return ERR_INTERNAL;
    }
    expr->as.binary.right = converted;
    *rightType = T_DOUBLE;
  }

  return ERR_OK;
}

/// @brief Helper function that checks if all branches end with return
/// statement
/// @param node statement AST node
/// @return true if every path ends with return statement, else otherwise
static bool always_returns(AstNode *node) {
  if (!node) {
    return false;
  }

  switch (node->type) {

  case AST_RETURN:
    return true;

  case AST_BLOCK:
    if (node->as.block.count == 0) {
      return false;
    }
    return always_returns(node->as.block.stmts[node->as.block.count - 1]);

  case AST_IF:
    return always_returns(node->as.ifStatement.thenBlock) &&
           always_returns(node->as.ifStatement.elseBlock);

  default:
    return false;
  }
}