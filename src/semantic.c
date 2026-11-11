/**
 * @file    semantic.c
 * @author  xsirmat00
 * @brief   Semantic analyzer implementation
 */

#include "semantic.h"
#include "ast.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  return true;
}

static SemanticError check_expression(Symtable *symtable, AstNode *node) {
  if (!node)
    return ERR_SEM_OK;

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
    Symbol *sym = st_get(symtable, node->as.identif);
    if (!sym || sym->type != ST_VAR) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }
    node->evalType = sym->data.var.type;
    break;
  }

  case AST_UNARY: {
    SemanticError err = check_expression(symtable, node->as.unary.operand);
    if (err != ERR_SEM_OK)
      return err;

    if (node->as.unary.op == OP_UNARY_MINUS) {
      if (node->as.unary.operand->evalType != T_INT &&
          node->as.unary.operand->evalType != T_DOUBLE) {
        return ERR_SEM_WRONG_TYPE;
      }
      node->evalType = node->as.unary.operand->evalType;
    }
    break;
  }

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
    }
    break;
  }

  case AST_FUNC_CALL: {
    Symbol *sym = st_get(symtable, node->as.funcCall.funcName);
    if (!sym || sym->type != ST_FUNC) {
      return ERR_SEM_UNDEFINED_SYMBOL;
    }

    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      SemanticError err = check_expression(symtable, node->as.funcCall.args[i]);
      if (err != ERR_SEM_OK)
        return err;
    }

    node->evalType = sym->data.func.returnType;
    break;
  }

  case AST_IF: {
    // CONTROL extension: evaluate 'if' as an expression
    SemanticError err_cond =
        check_expression(symtable, node->as.ifStatement.condition);
    if (err_cond != ERR_SEM_OK)
      return err_cond;

    if (node->as.ifStatement.condition->evalType != T_BOOLEAN) {
      return ERR_SEM_WRONG_TYPE;
    }

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

  default:
    break;
  }

  return ERR_SEM_OK;
}

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

    if (ast->as.varDecl.hasDeclaredType && declType != initType) {
      if (declType == T_DOUBLE && initType == T_INT) {
        // Ok, implicit conversion
      } else {
        return ERR_SEM_WRONG_TYPE;
      }
    }

    if (!st_insert_var(symtable, ast->as.varDecl.name, declType,
                       ast->as.varDecl.isModifiable)) {
      return ERR_SEM_REDEFINITION;
    }
    break;
  }

  case AST_BLOCK: {
    for (size_t i = 0; i < ast->as.block.count; i++) {
      SemanticError err = analyze_semantics(symtable, ast->as.block.stmts[i]);
      if (err != ERR_SEM_OK)
        return err;
    }
    break;
  }

  default:
    return check_expression(symtable, ast);
  }

  return ERR_SEM_OK;
}