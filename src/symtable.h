/**
 * IFJ26 compiler
 *
 * @file    symtable.h
 * @author  xoramst00
 * @brief   Symbol table declaration
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Data types supported by the language.
 */
typedef enum {
  T_INT,    // 32b
  T_DOUBLE, // 64b
  T_STRING,
  T_UNIT,
  T_BOOLEAN,
  T_ANY, // for arguments of internal functions
} DataType;

/**
 * @brief Kind of symbol stored in the symbol table.
 */
typedef enum { ST_VAR, ST_FUNC } SymbolType;

/**
 * @brief Result of insering a function symbol.
 */
typedef enum {
  ST_INSERT_OK,
  ST_INSERT_REDEFINED,
  ST_INSERT_ERROR,
} StInsertResult;

/**
 * @brief Function parameter name and type.
 */
typedef struct {
  char *name;
  DataType type;
  uint32_t uid;
} Parameter;

/**
 * @brief Information stored for a variable.
 */
typedef struct {
  DataType type;
  bool modifiable;
  uint32_t uid;
} VarInfo;

/**
 * @brief Information stored for a function.
 */
typedef struct {
  DataType returnType;
  Parameter *parameters;
  uint16_t paramCount;
  bool isMain;
  bool isDefined; // false = declared
} FuncInfo;

/**
 * @brief Symbol data for a variable or function.
 */
typedef struct {
  SymbolType type;
  union {
    VarInfo var;
    FuncInfo func;
  } data;
} Symbol;

/**
 * @brief One lexical scope and its parent scope.
 */
typedef struct SymtableNode {
  HashTable *table;
  struct SymtableNode *parent;
} SymtableNode;

/**
 * @brief Symbol table pointing to current smallest scope.
 */
typedef struct {
  SymtableNode *top;
} Symtable;

/**
 * @brief Initialize a symbol table.
 * @param st Symbol table to initialize.
 * @return true on succes, false if initialization fails.
 */
bool st_init(Symtable *symtable);

/**
 * @brief Release all memory owned by a symbol table linked list.
 * @param st Symbol table to free.
 */
void st_free(Symtable *symtable);

/**
 * @brief Open a new scope nested inside the current scope.
 * @param st Symbol table to update.
 * @return true on succes, false if opening the scope fails.
 */
bool st_push_scope(Symtable *symtable);

/**
 * @brief Close the current scope.
 * @param st Symbol table to update.
 */
void st_pop_scope(Symtable *symtable);

/**
 * @brief Insert a variable into the current scope.
 * @param st Symbol table to update.
 * @param name Variable name.
 * @param type Variable data type.
 * @param modifiable Whether the variable can be modified.
 * @return Symtable return code.
 */
StInsertResult st_insert_var(Symtable *symtable, const char *name,
                             DataType type, bool modifiable, uint32_t uid);

/**
 * @brief Insert a function into the symbol table.
 * @param st Symbol table to update.
 * @param name Function name.
 * @param ret_type Function return type.
 * @return Symtable return code.
 */
StInsertResult st_insert_func(Symtable *symtable, const char *name,
                              DataType retType, Parameter *params,
                              uint16_t paramCount, bool isMain, bool isDefined);

/**
 * @brief Find a symbol by name, searching the current and parent scopes.
 * @param st Symbol table to search.
 * @param name Symbol name.
 * @return The symbol if found, otherwise NULL.
 */
Symbol *st_get(Symtable *symtable, const char *name);

#endif
