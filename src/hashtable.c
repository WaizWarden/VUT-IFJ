/**
 * @file    hashtable.c
 * @author  xoramst00
 * @brief   ADT Hash Table implementation
 */

#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR_THRESHOLD 0.7
#define DEFAULT_RESIZE_FACTOR 2

bool ht_init(hashtable *ht, uint32_t size) {
  // Alloc memory for cells
  ht_cell *temp = (ht_cell *)malloc(size * sizeof(ht_cell));
  if (temp == NULL) {
    return false;
  }
  ht->cells = temp;

  // Initialize cells
  for (uint32_t i = 0; i < size; i++) {
    ht->cells[i].status = HT_CELL_EMPTY;
    ht->cells[i].key = NULL;
    ht->cells[i].value = NULL;
  }

  ht->size = size;
  ht->count = 0;
  ht->active_cells = 0;
  ht->deleted_cells = 0;

  return true;
}

void ht_free(hashtable *ht) {
  for (uint32_t i = 0; i < ht->size; i++) {
    if (ht->cells[i].status != HT_CELL_EMPTY) {
      free(ht->cells[i].key);
    }
  }
  free(ht->cells);
}

bool ht_insert(hashtable *ht, const char *key, void *value) {
  // Determine if resize is neeeded
  if (ht_load_factor(ht) > LOAD_FACTOR_THRESHOLD) {
    if (!ht_resize(ht, DEFAULT_RESIZE_FACTOR)) {
      return false;
    }
  }

  uint32_t index = hash_f(key, ht->size) % ht->size;
  // cell to save key-value pair to
  ht_cell *dest = NULL;

  for (uint32_t i = 0; i < ht->size; i++) {

    ht_cell *const current_cell = &(ht->cells[(index + i) % ht->size]);

    // If the cell is empty and destination has not been found yet, set it as
    // dest. Otherwise program has found deleted space to save.
    if (current_cell->status == HT_CELL_EMPTY) {
      if (!dest) {
        dest = current_cell;
      }
      break;
    }
    if (current_cell->status == HT_CELL_OCCUPIED) {
      if (strcmp(current_cell->key, key) == 0) {
        // Unalowed redefinition - semantic error 4, returning false
        return false;
      }
      continue;
    }
    // If deleted, mark first cell as dest to save, but continue search for same
    // key cell
    if (current_cell->status == HT_CELL_DELETED) {
      if (!dest) {
        dest = current_cell;
      }
      continue;
    }
  }

  if (!dest) // No avalible space, should not be possible due to load factor
    return false;

  if (dest->status == HT_CELL_DELETED) {
    // Deleted count cant be less than 0
    if (ht->deleted_cells == 0) {
      return false;
    }
    ht->deleted_cells--;
    free(dest->key);
  }

  // Saving data
  dest->key = (char *)malloc(strlen(key) + 1);
  if (!dest->key) {
    return false;
  }
  memcpy(dest->key, key, strlen(key) + 1);
  dest->value = value;
  dest->status = HT_CELL_OCCUPIED;
  ht->active_cells++;

  return true;
}

float ht_load_factor(const hashtable *ht) {
  if (ht->size == 0) {
    return 0.0f;
  }
  return (float)(ht->active_cells + ht->deleted_cells) / ht->size;
}

uint32_t hash_f(const char *key, uint32_t size) {
  return 0;

  /// @todo todo
}

bool ht_resize(hashtable *table, uint32_t resize_factor) {
  /// @todo todo
  return true;
}
