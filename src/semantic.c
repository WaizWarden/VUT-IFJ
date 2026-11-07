/**
 * @file    semantic.c
 * @author  xsirmat00
 * @brief   Implementace sémantického analyzátoru s ochranou proti NULL
 * pointerům.
 */

#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DataType analyze_expression(Symtable *symtable, const AstNode *node,
                            SemanticError *err) {
  if (!node || !symtable) {
    *err = ERR_SEM_OK;
    return T_UNIT;
  }

  switch (node->type) {
  case AST_LITERAL_INT:
    *err = ERR_SEM_OK;
    return T_INT;

  case AST_LITERAL_DOUBLE:
    *err = ERR_SEM_OK;
    return T_DOUBLE;

  case AST_IDENTIFIER: {
    const char *name = node->as.identif;
    if (!name) {
      *err = ERR_SEM_OTHER;
      return T_UNIT;
    }

    Symbol *sym = st_get(symtable, name);
    if (!sym) {
      *err = ERR_SEM_UNDEFINED;
      return T_UNIT;
    }

    if (sym->type == ST_VAR) {
      *err = ERR_SEM_OK;
      return sym->data.var.type;
    }

    *err = ERR_SEM_OTHER;
    return T_UNIT;
  }

  case AST_BINARY: {
    if (!node->as.binary.left || !node->as.binary.right) {
      *err = ERR_SEM_OTHER;
      return T_UNIT;
    }

    SemanticError leftErr = ERR_SEM_OK;
    SemanticError rightErr = ERR_SEM_OK;
    DataType leftType =
        analyze_expression(symtable, node->as.binary.left, &leftErr);
    DataType rightType =
        analyze_expression(symtable, node->as.binary.right, &rightErr);

    if (leftErr != ERR_SEM_OK) {
      *err = leftErr;
      return T_UNIT;
    }
    if (rightErr != ERR_SEM_OK) {
      *err = rightErr;
      return T_UNIT;
    }

    BinaryOperation operation = node->as.binary.operation;

    if (operation == OP_ADD || operation == OP_SUB || operation == OP_MUL ||
        operation == OP_DIV) {
      if (leftType == T_INT && rightType == T_INT) {
        *err = ERR_SEM_OK;
        return T_INT;
      }
      if ((leftType == T_INT || leftType == T_DOUBLE) &&
          (rightType == T_INT || rightType == T_DOUBLE)) {
        *err = ERR_SEM_OK;
        return T_DOUBLE;
      }
      *err = ERR_SEM_TYPE_MISMATCH;
      return T_UNIT;
    }

    *err = ERR_SEM_OTHER;
    return T_UNIT;
  }

  case AST_UNARY: {
    if (!node->as.unary.operand) {
      *err = ERR_SEM_OTHER;
      return T_UNIT;
    }

    SemanticError opErr = ERR_SEM_OK;
    DataType opType =
        analyze_expression(symtable, node->as.unary.operand, &opErr);
    if (opErr != ERR_SEM_OK) {
      *err = opErr;
      return T_UNIT;
    }

    if (node->as.unary.op == OP_UNARY_MINUS) {
      if (opType == T_INT || opType == T_DOUBLE) {
        *err = ERR_SEM_OK;
        return opType;
      }
      *err = ERR_SEM_TYPE_MISMATCH;
      return T_UNIT;
    }
    break;
  }

  default:
    break;
  }

  *err = ERR_SEM_OTHER;
  return T_UNIT;
}

SemanticError analyze_semantics(Symtable *symtable, const AstNode *node) {
  if (!node || !symtable || !symtable->top || !symtable->top->table) {
    return ERR_SEM_OK;
  }

  switch (node->type) {
  case AST_ASSIGN: {
    const char *var_name = node->as.assign.name;
    if (!var_name || !node->as.assign.value) {
      return ERR_SEM_OTHER;
    }

    Symbol *sym = st_get(symtable, var_name);
    if (!sym) {
      return ERR_SEM_UNDEFINED;
    }

    if (sym->type != ST_VAR) {
      return ERR_SEM_OTHER;
    }

    if (!sym->data.var.modifiable) {
      return ERR_SEM_OTHER;
    }

    SemanticError exprErr = ERR_SEM_OK;
    DataType exprType =
        analyze_expression(symtable, node->as.assign.value, &exprErr);
    if (exprErr != ERR_SEM_OK) {
      return exprErr;
    }

    if (sym->data.var.type != exprType &&
        !(sym->data.var.type == T_DOUBLE && exprType == T_INT)) {
      return ERR_SEM_TYPE_MISMATCH;
    }

    return ERR_SEM_OK;
  }

  case AST_BINARY:
  case AST_UNARY:
  case AST_LITERAL_INT:
  case AST_LITERAL_DOUBLE:
  case AST_IDENTIFIER: {
    SemanticError err = ERR_SEM_OK;
    analyze_expression(symtable, node, &err);
    return err;
  }

  default:
    break;
  }

  return ERR_SEM_OK;
}