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

static SemanticError analyze_functions(Symtable *symtable, AstNode *root);
static SemanticError register_builtin_functions(Symtable *symtable);
static SemanticError analyze_function_body(Symtable *symtable, AstNode *functionNode);
static Parameter *copy_params(const Parameter *src, uint16_t count);
static SemanticError analyze_statement(Symtable *symtable, AstNode *statement);

SemanticError analyze_semantics(Symtable *symtable, AstNode *root) {
  if (root == NULL || root->type != AST_PROGRAM) {
    return ERR_OTHER;
  }

  SemanticError error = analyze_functions(symtable, root);
  if (error != ERR_OK) {
    return error;
  }

  SemanticError error2 = analyze_function_body(symtable, root);
  if (error2 != ERR_OK) {
    return error2;
  }

  return ERR_OK;
}

/// @brief Analyzes all function heads and inserts them into symbol table
/// @param symtable symbol table
/// @param root AST root
/// @return Semantic error type
static SemanticError analyze_functions(Symtable *symtable, AstNode *root) {

  // insertion of built-in functions into symtable
  if (register_builtin_functions(symtable) != ERR_OK) {
    return ERR_OTHER;
  }

  // ensure all builtin functions are on global scope
  // and user defined are lower
  if (!st_push_scope(symtable)) {
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
                       functionNode->as.funcDef.paramCount, false, true);

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

  // analyze function body
  for (size_t i = 0; i < root->as.program.count; i++) {

    AstNode *functionNode = root->as.program.functions[i];
    if (!st_push_scope(symtable)) {
      return ERR_INTERNAL;
    }

    // Add params from function
    for (uint16_t p = 0; p < functionNode->as.funcDef.paramCount; p++) {
      Parameter param = functionNode->as.funcDef.params[p];

      if (!st_insert_var(symtable, param.name, param.type, false)) {
        st_pop_scope(symtable);
        return ERR_REDEFINITION;
      }
    }

    SemanticError result = analyze_function_body(symtable, functionNode);
    st_pop_scope(symtable);

    if (result != ERR_OK){
      return result;
    }

  }
  return ERR_OK;
}

/// @brief Analyzes body of a function
/// @param symtable symbol table
/// @param root AST root
/// @return Semantic error type
static SemanticError analyze_function_body(Symtable *symtable, AstNode *functionNode) {
  AstNode *body = functionNode->as.funcDef.body;

  for (size_t i = 0; i < body->as.block.count; i++){
    AstNode *stmt =  body->as.block.stmts[i];
    analyze_statement(symtable, stmt);
  }
  
  return ERR_OK;
}

static SemanticError analyze_statement(Symtable *symtable, AstNode *stmt) {
  
  switch (stmt->type)
  {
  case (AST_BLOCK):
    /* code */
    break;
  
  default:
    return ERR_OTHER;
  }

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