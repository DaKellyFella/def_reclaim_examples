/* Lock-free seperate chaining hash table.
 * From paper "High Performance Dynamic Lock-Free Hash Tables and List-Based Sets".
 * Lock-free updates (add/remove, contains).
*/

#pragma once

#include <stdint.h>

typedef struct c_mmht_t c_mmht_t;

c_mmht_t * c_mmht_create(uint64_t size);
int c_mmht_contains(c_mmht_t * set, int64_t key);
int c_mmht_add(c_mmht_t * set, int64_t key);
int c_mmht_remove_leaky(c_mmht_t * set, int64_t key);