/**
 * @file    symtable.c
 * @author  xoramst00
 * @brief   Symbol table implementation
 */

#include "symtable.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_ST_SIZE 8

/**
 * @brief Allocates memory for SymtableNode and all its structures. Internal
 * only.
 * @param void Takes no parameters.
 * @returns Pointer to allocated memory.
 */
SymtableNode *st_alloc_node(void) {
  SymtableNode *new = calloc(1, sizeof(SymtableNode));
  if (!new) {
    return NULL;
  }

  new->table = malloc(sizeof(HashTable));
  if (!new->table) {
    free(new);
    return NULL;
  }

  return new;
}

void st_free_symbol(void *symbol) { free(symbol); }

bool st_init(Symtable *symtable) {
  if (!symtable) {
    return false;
  }

  // allocate spece
  SymtableNode *new = st_alloc_node();
  if (!new) {
    return false;
  }

  // assign start values
  new->parent = NULL;
  if (!ht_init(new->table, INITIAL_ST_SIZE)) {
    free(new->table);
    free(new);
    return false;
  }

  symtable->top = new;
  return true;
}

void st_free(Symtable *symtable) {
  if (!symtable) {
    return;
  }

  // go through the whole linked list
  SymtableNode *toFree = symtable->top;
  while (toFree != NULL) {
    SymtableNode *nextToFree = toFree->parent;

    ht_free(toFree->table, st_free_symbol);
    free(toFree->table);
    free(toFree);

    toFree = nextToFree;
  }

  symtable->top = NULL;
}

bool st_push_scope(Symtable *symtable) {
  if (!symtable || !symtable->top) {
    return false;
  }

  // allocate space
  SymtableNode *new = st_alloc_node();
  if (!new) {
    return false;
  }

  // assign new values
  new->parent = symtable->top;
  if (!ht_init(new->table, INITIAL_ST_SIZE)) {
    free(new->table);
    free(new);
    return false;
  }

  symtable->top = new;
  return true;
}

void st_pop_scope(Symtable *symtable) {
  if (!symtable || !symtable->top) {
    return;
  }

  // reasign top of linked list
  SymtableNode *toFree = symtable->top;
  symtable->top = toFree->parent;

  ht_free(toFree->table, st_free_symbol);
  free(toFree->table);
  free(toFree);
}

bool st_insert_var(Symtable *symtable, const char *name, DataType type,
                   bool modifiable) {
  if (!symtable || !symtable->top || !symtable->top->table || !name) {
    return false;
  }

  Symbol *new = calloc(1, sizeof(Symbol));
  if (!new) {
    return false;
  }

  new->type = ST_VAR;
  new->data.var.type = type;
  new->data.var.modifiable = modifiable;

  // insert symbol while checking fail
  if (!ht_insert(symtable->top->table, name, new)) {
    free(new);
    return false;
  }

  return true;
}

bool st_insert_func(Symtable *symtable, const char *name, DataType retType,
                    Parameter *params, uint16_t paramCount, bool isMain,
                    bool isDefined) {
  if (!symtable || !symtable->top || !symtable->top->table || !name ||
      (paramCount > 0 && !params)) {
    return false;
  }

  for (uint16_t i = 0; i < paramCount; i++) {
    if (!params[i].name) {
      return false;
    }
  }

  Symbol *new = calloc(1, sizeof(Symbol));
  if (!new) {
    return false;
  }

  new->type = ST_FUNC;
  new->data.func.returnType = retType;
  new->data.func.parameters = params;
  new->data.func.paramCount = paramCount;
  new->data.func.isMain = isMain;
  new->data.func.isDefined = isDefined;

  SymtableNode *globalScope = symtable->top;
  while (globalScope->parent != NULL) {
    globalScope = globalScope->parent;
  }

  // insert symbol while checking fail
  if (!ht_insert(globalScope->table, name, new)) {
    free(new);
    return false;
  }

  return true;
}

Symbol *st_get(Symtable *symtable, const char *name) {
  if (!symtable) {
    return NULL;
  }

  // go through all scopes
  SymtableNode *nextScope = symtable->top;
  Symbol *symbol = NULL;
  while (nextScope) {
    symbol = ht_get(nextScope->table, name);
    if (symbol) {
      return symbol;
    }

    nextScope = nextScope->parent;
  }

  return NULL;
}