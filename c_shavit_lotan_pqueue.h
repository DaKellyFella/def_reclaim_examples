#pragma once

/* A Shavit Lotan priority queue written in C.
 * Like the original description in the paper this queue has a skiplist
 * as the underlying data-structure for ordering elements.
 * The data-structure is lock-free and quiescently consistent.
 */

#include <stdint.h>

#define N 20

typedef struct c_shavit_lotan_pqueue_t c_shavit_lotan_pqueue_t;

c_shavit_lotan_pqueue_t * c_shavit_lotan_pqueue_create();

int c_shavit_lotan_pqueue_contains(c_shavit_lotan_pqueue_t *set, int64_t key);
int c_shavit_lotan_pqueue_add(uint64_t *seed, c_shavit_lotan_pqueue_t *set, int64_t key);
int c_shavit_lotan_pqueue_remove_leaky(c_shavit_lotan_pqueue_t *set, int64_t key);
int c_shavit_lotan_pqueue_leaky_pop_min(c_shavit_lotan_pqueue_t *set);
void c_shavit_lotan_pqueue_print (c_shavit_lotan_pqueue_t *set);