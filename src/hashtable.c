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
#define HASH_INITIAL_VALUE 5381

bool ht_init(HashTable *hashTable, uint32_t size) {
  if (hashTable == NULL || size == 0) {
    return false;
  }

  // Alloc memory for cells
  HtCell *cells = calloc(size, sizeof(*cells));
  if (cells == NULL) {
    return false;
  }
  hashTable->cells = cells;

  // Initialize cells
  for (uint32_t i = 0; i < size; i++) {
    hashTable->cells[i].status = HT_CELL_EMPTY;
    hashTable->cells[i].key = NULL;
    hashTable->cells[i].value = NULL;
  }

  hashTable->size = size;
  hashTable->activeCells = 0;
  hashTable->deletedCells = 0;

  return true;
}

void ht_free(HashTable *hashTable, void (*data_free)(void *)) {
  if (hashTable == NULL) {
    return;
  }

  for (uint32_t i = 0; i < hashTable->size; i++) {
    if (hashTable->cells != NULL &&
        hashTable->cells[i].status != HT_CELL_EMPTY) {
      free(hashTable->cells[i].key);

      // free data if function is provided
      if (data_free != NULL && hashTable->cells[i].value != NULL) {
        data_free(hashTable->cells[i].value);
      }
    }
  }
  free(hashTable->cells);
  hashTable->cells = NULL;
  hashTable->size = 0;
  hashTable->activeCells = 0;
  hashTable->deletedCells = 0;
}

bool ht_insert(HashTable *hashTable, const char *key, void *value) {
  if (hashTable == NULL || key == NULL || hashTable->cells == NULL ||
      hashTable->size == 0) {
    return false;
  }

  // Determine if resize is neeeded
  if (ht_load_factor(hashTable) > LOAD_FACTOR_THRESHOLD) {
    if (!ht_resize(hashTable, DEFAULT_RESIZE_FACTOR)) {
      return false;
    }
  }

  uint32_t index = hash_f(key, hashTable->size);
  HtCell *dst = NULL;

  for (uint32_t offset = 0; offset < hashTable->size; offset++) {

    HtCell *const currentCell =
        &hashTable->cells[(index + offset) % hashTable->size];

    // If the cell is empty and destination has not been found yet, set it as
    // dest. Otherwise program has found deleted space to save.
    if (currentCell->status == HT_CELL_EMPTY) {
      if (!dst) {
        dst = currentCell;
      }
      break;
    }
    if (currentCell->status == HT_CELL_OCCUPIED &&
        strcmp(currentCell->key, key) == 0) {
      // Unalowed redefinition - semantic error 4, returning false
      return false;
    }
    // If deleted, mark first cell as dest to save, but continue search for same
    // key cell
    if (currentCell->status == HT_CELL_DELETED && !dst) {
      dst = currentCell;
    }
  }

  if (!dst) { // No avalible space, should not be possible due to load factor
    return false;
  }

  char *newKey = malloc(strlen(key) + 1);
  if (newKey == NULL) {
    return false;
  }
  memcpy(newKey, key, strlen(key) + 1); // NOLINT

  if (dst->status == HT_CELL_DELETED) {
    // Deleted count cant be less than 0
    if (hashTable->deletedCells == 0) {
      free(newKey);
      return false;
    }
    hashTable->deletedCells--;
    free(dst->key);
  }
  dst->key = newKey;

  // Saving data
  dst->value = value;
  dst->status = HT_CELL_OCCUPIED;
  hashTable->activeCells++;

  return true;
}

float ht_load_factor(const HashTable *hashTable) {
  if (hashTable == NULL || hashTable->size == 0) {
    return 0.0F;
  }
  return (float)(hashTable->activeCells + hashTable->deletedCells) /
         (float)hashTable->size;
}

// DJB2 Hashing function implementation
// source: https://theartincode.stanis.me/008-djb2/
uint32_t hash_f(const char *key, uint32_t size) {
  if (key == NULL || size == 0) {
    return 0;
  }

  uint32_t hash = HASH_INITIAL_VALUE; // Initial hashing magic number
  unsigned char current;

  while ((current = *key++)) {
    hash = ((hash << 5) + hash) + current; // NOLINT
  }

  return hash % size;
}

bool ht_resize(HashTable *hashTable, uint32_t resizeFactor) {
  if (hashTable == NULL || hashTable->cells == NULL || resizeFactor == 0 ||
      hashTable->size == 0 || UINT32_MAX / hashTable->size < resizeFactor) {
    return false;
  }

  // Memory alocation
  uint32_t newSize = hashTable->size * resizeFactor;
  HtCell *newCellArray = calloc(newSize, sizeof(HtCell));
  if (!newCellArray) {
    return false; // memory alocation fault
  }

  // Cell inicializatin
  for (uint32_t i = 0; i < newSize; i++) {
    newCellArray[i].status = HT_CELL_EMPTY;
    newCellArray[i].key = NULL;
    newCellArray[i].value = NULL;
  }

  // Data copying (only active cells)
  for (uint32_t i = 0; i < hashTable->size; i++) {
    HtCell *currentOld = &hashTable->cells[i];
    if (currentOld->status != HT_CELL_OCCUPIED || !currentOld->key) {
      continue; // skip empty and faulty cells
    }

    uint32_t newHash = hash_f(currentOld->key, newSize);
    HtCell *newDst = NULL;

    // Find empty space
    for (uint32_t j = 0; j < newSize; j++) {
      HtCell *currentNew = &newCellArray[(newHash + j) % newSize];

      if (currentNew->status == HT_CELL_EMPTY) {
        newDst = currentNew;
        break;
      }
    }

    if (!newDst) {
      free(newCellArray);
      return false;
    }

    newDst->key = currentOld->key; // only change the pointer, key stays
    newDst->value = currentOld->value;
    newDst->status = HT_CELL_OCCUPIED;
  }

  // Put new cell array into hashtable
  free(hashTable->cells);
  hashTable->cells = newCellArray;

  hashTable->deletedCells = 0;
  hashTable->size = newSize;

  return true;
}

void *ht_get(const HashTable *hashTable, const char *key) {
  if (hashTable == NULL || key == NULL || hashTable->cells == NULL ||
      hashTable->size == 0) {
    return NULL;
  }
  uint32_t hash = hash_f(key, hashTable->size);

  // Cycle through hash table from hash index
  for (uint32_t i = 0; i < hashTable->size; i++) {
    HtCell *current = &hashTable->cells[(hash + i) % hashTable->size];
    if (current->status == HT_CELL_OCCUPIED && strcmp(current->key, key) == 0) {
      return current->value; // Key found, return value
    }

    if (current->status == HT_CELL_EMPTY) {
      break; // element is not in table
    }
  }

  // Element not found
  return NULL;
}

bool ht_remove(HashTable *hashTable, const char *key) {
  if (hashTable == NULL || key == NULL || hashTable->cells == NULL ||
      hashTable->size == 0) {
    return false;
  }

  uint32_t hash = hash_f(key, hashTable->size);

  // Cycle through hash table from hash index
  for (uint32_t i = 0; i < hashTable->size; i++) {
    HtCell *current = &hashTable->cells[(hash + i) % hashTable->size];
    if (current->status == HT_CELL_OCCUPIED && strcmp(current->key, key) == 0) {

      free(current->key); // element found, delete it
      current->key = NULL;
      current->value = NULL; // hashtable doesnt own the value
      current->status = HT_CELL_DELETED;
      hashTable->deletedCells++;
      hashTable->activeCells--;
      return true;
    }

    if (current->status == HT_CELL_EMPTY) {
      break; // element is not in table
    }
  }

  // Element not found
  return false;
}
