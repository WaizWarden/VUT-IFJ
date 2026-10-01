/**
 * @file    hashtable.c
 * @author  xoramst00
 * @brief   ADT Hash Table implementation
 */

#include "hashtable.h"
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR_THRESHOLD 0.7
#define DEFAULT_RESIZE_FACTOR 2

bool ht_init(HashTable *hashTable, uint32_t size) {
  // Alloc memory for cells
  HtCell *cells = malloc(size * sizeof(*cells));
  if (cells == NULL) {
    return false;
  }
  hashTable->cells = cells;

  // Initialize cells
  for (uint32_t index = 0; index < size; index++) {
    hashTable->cells[index].status = HT_CELL_EMPTY;
    hashTable->cells[index].key = NULL;
    hashTable->cells[index].value = NULL;
  }

  hashTable->size = size;
  hashTable->count = 0;
  hashTable->activeCells = 0;
  hashTable->deletedCells = 0;

  return true;
}

void ht_free(HashTable *hashTable) {
  for (uint32_t index = 0; index < hashTable->size; index++) {
    if (hashTable->cells[index].status != HT_CELL_EMPTY) {
      free(hashTable->cells[index].key);
    }
  }
  free(hashTable->cells);
}

bool ht_insert(HashTable *hashTable, const char *key, void *value) {
  // Determine if resize is neeeded
  if (ht_load_factor(hashTable) > LOAD_FACTOR_THRESHOLD) {
    if (!ht_resize(hashTable, DEFAULT_RESIZE_FACTOR)) {
      return false;
    }
  }

  uint32_t index = hash_f(key, hashTable->size) % hashTable->size;
  // cell to save key-value pair to
  HtCell *dst = NULL;

  for (uint32_t offset = 0; offset < hashTable->size; offset++) {

    HtCell *const currentCell = &hashTable->cells[(index + offset) % hashTable->size];

    // If the cell is empty and destination has not been found yet, set it as
    // dest. Otherwise program has found deleted space to save.
    if (currentCell->status == HT_CELL_EMPTY) {
      if (!dst) {
        dst = currentCell;
      }
      break;
    }
    if (currentCell->status == HT_CELL_OCCUPIED) {
      if (strcmp(currentCell->key, key) == 0) {
        // Unalowed redefinition - semantic error 4, returning false
        return false;
      }
      continue;
    }
    // If deleted, mark first cell as dest to save, but continue search for same
    // key cell
    if (currentCell->status == HT_CELL_DELETED) {
      if (!dst) {
        dst = currentCell;
      }
      continue;
    }
  }

  if (!dst) { // No avalible space, should not be possible due to load factor
    return false;
  }

  if (dst->status == HT_CELL_DELETED) {
    // Deleted count cant be less than 0
    if (hashTable->deletedCells == 0) {
      return false;
    }
    hashTable->deletedCells--;
    free(dst->key);
  }

  // Saving data
  dst->key = malloc(strlen(key) + 1);
  if (!dst->key) {
    return false;
  }
  memcpy(dst->key, key, strlen(key) + 1);
  dst->value = value;
  dst->status = HT_CELL_OCCUPIED;
  hashTable->activeCells++;

  return true;
}

float ht_load_factor(const HashTable *hashTable) {
  if (hashTable->size == 0) {
    return 0.0F;
  }
  return (float)(hashTable->activeCells + hashTable->deletedCells) /
         (float)hashTable->size;
}

uint32_t hash_f(const char *key, uint32_t size) {
  /// @todo todo
  (void)key;
  (void)size;
  return 0;
}

bool ht_resize(HashTable *hashTable, uint32_t resizeFactor) {
  /// @todo todo
  (void)hashTable;
  (void)resizeFactor;
  return true;
}
