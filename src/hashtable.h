/**
 * @file    hashtable.h
 * @author  xoramst00
 * @brief   ADT Hash Table
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { HT_CELL_EMPTY, HT_CELL_OCCUPIED, HT_CELL_DELETED } HtCellStatus;

typedef struct {
  char *key;
  void *value;
  HtCellStatus status;
} HtCell;

typedef struct {
  HtCell *cells;
  uint32_t size;
  uint32_t count;
  uint32_t activeCells;
  uint32_t deletedCells;
} HashTable;

/**
 * @brief Creates a new hash table
 * @returns true if creation was successful, false otherwise
 * @param ht Pointer to the hash table to initialize
 * @param size Initial size of the main array of the hash table, should be a
 * power of 2
 */
bool ht_init(HashTable *hashTable, uint32_t size);

/**
 * @brief Deletes hash table and frees memory
 * @param hashTable Pointer to hash table
 */
void ht_free(HashTable *hashTable);

/**
 * @brief Inserts a new key-value pair
 * @returns true if insertion was successful, false otherwise
 * @param hashTable Pointer to the hash table
 * @param key Identifier
 * @param value Pointer to data
 */
bool ht_insert(HashTable *hashTable, const char *key, void *value);

/**
 * @brief Retrieves a value from hashtable by key
 * @returns Pointer to the value if found, NULL otherwise
 * @param hashTable Pointer to the hash table
 * @param key Identifier
 */
void *ht_get(const HashTable *hashTable, const char *key);

/**
 * @brief Removes a key-value pair from the hash table
 * @returns true if removal was successful, false otherwise
 * @param hashTable Pointer to the hash table
 * @param key Identifier
 */
bool ht_remove(HashTable *hashTable, const char *key);

/**
 * @brief Resizes the hash table
 * @returns true if resizing was successful, false otherwise
 * @param hashTable Pointer to the hash table
 * @param resizeFactor Size gets multiplied by this factor, only powers of 2
 * are allowed
 */
bool ht_resize(HashTable *hashTable, uint32_t resizeFactor);

/**
 * @brief Hash function for the hash table
 * @returns Hash value
 * @param key Identifier
 * @param size Main array size from hash table
 */
uint32_t hash_f(const char *key, uint32_t size);

/**
 * @brief Calculates the load factor of the hash table
 * @returns Load factor
 * @param hashTable Pointer to the hash table
 */
float ht_load_factor(const HashTable *hashTable);

#endif