/* Lock-free split-order hash table.
 * From paper "Split-Ordered Lists: Lock-Free Extensible Hash Tables".
 * Lock-free updates (contains/add/remove).
*/

#pragma once

#include <stdint.h>

typedef struct c_soht_t c_soht_t;

c_soht_t * c_soht_create(uint64_t size, uint64_t max_load);
int c_soht_contains(c_soht_t *set, int64_t key);
int c_soht_add(c_soht_t *set, int64_t key);
int c_soht_remove_leaky(c_soht_t *set, int64_t key);
void c_soht_print(c_soht_t *set);