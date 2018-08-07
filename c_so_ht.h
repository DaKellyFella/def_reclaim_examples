/* Lock-free split-order hash table.
 * From paper "Split-Ordered Lists: Lock-Free Extensible Hash Tables".
 * Lock-free updates (contains/add/remove).
*/

#pragma once

#include <stdint.h>

typedef struct c_so_ht_t c_so_ht_t;

c_so_ht_t * c_so_ht_create(uint64_t size, uint64_t max_load);
int c_so_ht_contains(c_so_ht_t *set, int64_t key);
int c_so_ht_add(c_so_ht_t *set, int64_t key);
int c_so_ht_remove_leaky(c_so_ht_t *set, int64_t key);
void c_so_ht_print(c_so_ht_t *set);