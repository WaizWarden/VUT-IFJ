/**
 * @file    symtable.h
 * @author  xoramst00
 * @brief   Symbol table declaration
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  T_INT,    // 32b
  T_DOUBLE, // 64b
  T_STRING,
  T_UNIT,
  T_BOOLEAN,
  T_ANY // for arguments of internal functions
} DataType;

typedef enum { ST_VAR, ST_FUNC } SymbolType;

typedef struct {
  char *name;
  DataType type;
} Parameter;

typedef struct {
  DataType type;
  bool modifiable;
} VarInfo;

typedef struct {
  DataType returnType;
  Parameter *parameters;
  uint16_t paramCount;
  bool isMain;
  bool isDefined; // false = declared
} FuncInfo;

typedef struct {
  SymbolType type;
  union {
    VarInfo var;
    FuncInfo func;
  } data;
} Symbol;

typedef struct {
  HashTable table;
  struct SymTableNode *parent;
} SymTableNode;

typedef struct {
  SymTableNode *top;
} SymTable;

#endif