/* Lock-free seperate chaining hash table.
 * From paper "High Performance Dynamic Lock-Free Hash Tables and List-Based Sets".
 * Lock-free updates (add/remove, contains).
*/

#pragma once

#include <stdint.h>

typedef struct c_mm_ht_t c_mm_ht_t;

c_mm_ht_t * c_mm_ht_create(uint64_t size, uint64_t list_length);
int c_mm_ht_contains(c_mm_ht_t * set, int64_t key);
int c_mm_ht_add(c_mm_ht_t * set, int64_t key);
int c_mm_ht_remove_leaky(c_mm_ht_t * set, int64_t key);