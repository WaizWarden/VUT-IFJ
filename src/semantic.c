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
static SemanticError analyze_functions(Symtable *symtable,
                                       AstNode *functionNode);
static Parameter *copy_params(const Parameter *src, uint16_t count);
static SemanticError analyze_statement(Symtable *symtable, AstNode *stmt);
static SemanticError analyze_expression(Symtable *symtable, AstNode *expr,
                                        DataType *out);
static AstNode *new_implicit_conversion(AstNode *operand);

static uint32_t uidCounter = 0;

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (root == NULL || root->type != AST_PROGRAM) {
    return ERR_OTHER;
  }

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

    // semantically analyze all statements in function body
    for (size_t i = 0; i < body->as.block.count; i++) {
      AstNode *stmt = body->as.block.stmts[i];

      SemanticError errCode = analyze_statement(symtable, stmt);
      if (errCode != ERR_OK) {
        st_pop_scope(symtable);
        return errCode;
      }
    }

    st_pop_scope(symtable);
  }

  return ERR_OK;
}

/// @brief Analyzes one statement of a function body
/// @param symtable symbol table
/// @param stmt statement AST node
/// @return Semantic error type
static SemanticError analyze_statement(Symtable *symtable, AstNode *stmt) {

  SemanticError error = ERR_OK;

  switch (stmt->type) {
  case AST_BLOCK:

    if (!st_push_scope(symtable)) {
      return ERR_INTERNAL;
    }

    for (size_t i = 0; i < stmt->as.block.count; i++) {

      error = analyze_statement(symtable, stmt->as.block.stmts[i]);
      if (error != ERR_OK) {
        st_pop_scope(symtable);
        return error;
      }
    }

    st_pop_scope(symtable);
    break;

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

  default:
    return ERR_OTHER;
  }

  return ERR_OK;
}

static SemanticError analyze_expression(Symtable *symtable, AstNode *expr,
                                        DataType *out) {

  SemanticError error = ERR_OK;

  if (expr == NULL) {
    *out = T_UNIT;
    return ERR_OK;
  }

  switch (expr->type) {
  case AST_LITERAL_INT:
    *out = T_INT;
    expr->evalType = *out;
    break;

  case AST_LITERAL_DOUBLE:
    *out = T_DOUBLE;
    expr->evalType = *out;
    break;

  case AST_LITERAL_STRING:
    *out = T_STRING;
    expr->evalType = *out;
    break;

  case AST_LITERAL_BOOL:
    *out = T_BOOLEAN;
    expr->evalType = *out;
    break;

  case AST_LITERAL_UNIT:
    *out = T_UNIT;
    expr->evalType = *out;
    break;

  case AST_IDENTIFIER: {
    Symbol *symbol = st_get(symtable, expr->as.identif);
    if (!symbol || symbol->type != ST_VAR) {
      return ERR_UNDEFINED_SYMBOL;
    }

    expr->evalType = symbol->data.var.type;
    expr->uid = symbol->data.var.uid;
    *out = expr->evalType;
    break;
  }

  case AST_BINARY: {

    // @todo sub, mul,
    // @ extension and, or

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

    if (operation == OP_ADD) {
      if (leftType == T_STRING && rightType == T_STRING) {
        expr->evalType = T_STRING;
      }

      else if ((leftType == T_INT && rightType == T_DOUBLE) ||
               (leftType == T_DOUBLE && rightType == T_INT)) {

        if (leftType != rightType) {
          if (leftType == T_INT) {
            expr->as.binary.left =
                new_implicit_conversion(expr->as.binary.left);
            if (!expr->as.binary.left) {
              return ERR_INTERNAL;
            }
          }

          else {
            expr->as.binary.right =
                new_implicit_conversion(expr->as.binary.right);
            if (!expr->as.binary.right) {
              return ERR_INTERNAL;
            }
          }
        }
        expr->evalType = T_DOUBLE;
      }

      else if (leftType == rightType &&
               (leftType == T_INT || leftType == T_DOUBLE)) {
        expr->evalType = leftType;
      }

      else {
        return ERR_WRONG_TYPE;
      }
    }

    else if (operation == OP_LT || operation == OP_LE || operation == OP_GT ||
             operation == OP_GE) {

      if ((leftType != T_INT && leftType != T_DOUBLE && leftType != T_STRING) ||
          (rightType != T_INT && rightType != T_DOUBLE &&
           rightType != T_STRING)) {
        return ERR_WRONG_TYPE;
      }

      if (leftType != rightType) {
        if (!((leftType == T_INT && rightType == T_DOUBLE) ||
              (leftType == T_DOUBLE && rightType == T_INT))) {
          return ERR_WRONG_TYPE;
        }

        // convert int to double
        else {

          if (leftType == T_INT) {
            expr->as.binary.left =
                new_implicit_conversion(expr->as.binary.left);
            if (!expr->as.binary.left) {
              return ERR_INTERNAL;
            }
          }

          else {
            expr->as.binary.right =
                new_implicit_conversion(expr->as.binary.right);
            if (!expr->as.binary.right) {
              return ERR_INTERNAL;
            }
          }
        }
      }
      expr->evalType = T_BOOLEAN;
    }

    else if (operation == OP_EQ || operation == OP_NEQ) {

      if (leftType != rightType) {
        if (!((leftType == T_INT && rightType == T_DOUBLE) ||
              (leftType == T_DOUBLE && rightType == T_INT))) {
          return ERR_WRONG_TYPE;
        }

        // convert int to double
        else {

          if (leftType == T_INT) {
            expr->as.binary.left =
                new_implicit_conversion(expr->as.binary.left);
            if (!expr->as.binary.left) {
              return ERR_INTERNAL;
            }
          }

          else {
            expr->as.binary.right =
                new_implicit_conversion(expr->as.binary.right);
            if (!expr->as.binary.right) {
              return ERR_INTERNAL;
            }
          }
        }
      }
      expr->evalType = T_BOOLEAN;
    }

    *out = expr->evalType;
    break;
  }

  case AST_FUNC_CALL:
    break;

  case AST_UNARY: {
    DataType unaryType = T_UNIT;
    error = analyze_expression(symtable, expr->as.unary.operand, &unaryType);
    if (error != ERR_OK) {
      return error;
    }

    if (expr->as.unary.op == OP_UNARY_MINUS) {
      if (unaryType != T_DOUBLE && unaryType != T_INT) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = unaryType;
    }

    if (expr->as.unary.op == OP_UNARY_NOT) {
      if (unaryType != T_BOOLEAN) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = T_BOOLEAN;
    }

    if (expr->as.unary.op == OP_UNARY_INT2DOUBLE) {
      if (unaryType != T_INT) {
        return ERR_WRONG_TYPE;
      }
      expr->evalType = T_DOUBLE;
    }

    *out = expr->evalType;
    break;
  }

  default:
    break;
  }

  return error;
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

static AstNode *new_implicit_conversion(AstNode *operand) {
  if (!operand) {
    return NULL;
  }

  AstNode *newNode = ast_new_unary(OP_UNARY_INT2DOUBLE, operand, operand->line);
  newNode->evalType = T_DOUBLE;

  return newNode;
}