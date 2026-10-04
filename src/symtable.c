/**
 * @file    symtable.c
 * @author  xoramst00
 * @brief   Symbol table implementation
 */

#include "symtable.h"
#include <stdlib.h>

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
    return false;
  }

  new->table = malloc(sizeof(HashTable));
  if (!new->table) {
    free(new);
    return false;
  }

  return new;
}

bool st_init(Symtable *symtable) {
  if (!symtable) {
    return false;
  }

  // allocate spece
  SymtableNode *new = st_alloc_node();

  // assign start values
  new->parent = NULL;
  ht_init(new->table, INITIAL_ST_SIZE);

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

    ht_free(toFree->table);
    free(toFree->table);
    free(toFree);

    toFree = nextToFree;
  }

  symtable->top = NULL;
}

bool st_push_scope(Symtable *symtable) {
  if (!symtable) {
    return false;
  }

  // allocate space
  SymtableNode *new = st_alloc_node();

  // assign new values
  new->parent = symtable->top;
  ht_init(new->table, INITIAL_ST_SIZE);

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

  ht_free(toFree->table);
  free(toFree->table);
  free(toFree);
}

bool st_insert_var(Symtable *symtable, const char *name, DataType type,
                   bool modifiable) {
  Symbol *new =
      calloc(1, sizeof(Symbol)); // MUST BE FREED before freeing Hash table
  if (!new) {
    return false;
  }

  new->type = ST_VAR;
  new->data.var.type = type;
  new->data.var.modifiable = modifiable;

  // insert symbol while checking fail
  if(!ht_insert(symtable->top->table, name, new)){
    free(new);
    return false;
  }

  return true;
}