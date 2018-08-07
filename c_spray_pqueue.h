#pragma once

/* A spray-list priority queue written in C.
 * Like the original description in the paper this queue has a skiplist
 * as the underlying data-structure for ordering elements.
 * The data-structure is lock-free and has relaxed correctness semantics.
 */

#include <stdint.h>

#define N 20

typedef struct c_spray_pqueue_t c_spray_pqueue_t;

c_spray_pqueue_t *c_spray_pqueue_create(int64_t threads);

int c_spray_pqueue_contains(c_spray_pqueue_t *set, int64_t key);
int c_spray_pqueue_add(uint64_t *seed, c_spray_pqueue_t *set, int64_t key);
int c_spray_pqueue_remove_leaky(c_spray_pqueue_t *set, int64_t key);
int c_spray_pqueue_leaky_pop_min(uint64_t *seed, c_spray_pqueue_t *set);
void c_spray_pqueue_print (c_spray_pqueue_t *set);