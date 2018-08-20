/* Fixed height skiplist: A skiplist implementation with an array of "next"
 * nodes of fixed height.
 */

#include "c_lj_pq.h"

#include <stdbool.h>
#include <forkscan.h>
#include <stdio.h>
#include <assert.h>

typedef struct node_t node_t;
typedef node_t volatile * volatile node_ptr;
typedef struct unpacked_t unpacked_t; 

enum LJ_STATE { INSERT_PENDING, INSERTED };
typedef enum LJ_STATE lj_state_t;

struct node_t {
  int64_t key;
  int32_t toplevel;
  volatile lj_state_t insert_state;
  node_ptr next[N];
};

struct c_lj_pq_t {
  uint32_t boundoffset;
  node_t head, tail;
};

static node_ptr node_create(int64_t key, int32_t toplevel){
  node_ptr node = forkscan_malloc(sizeof(node_t));
  node->key = key;
  node->toplevel = toplevel;
  node->insert_state = INSERT_PENDING;
  return node;
}

node_ptr unmark(node_ptr node){
  return (node_ptr)(((size_t)node) & (~0x1));
}

node_ptr mark(node_ptr node){
  return (node_ptr)((size_t)node | 0x1);
}

bool is_marked(node_ptr node){
  return unmark(node) != node;
}


struct unpacked_t {
  bool marked;
  node_ptr address;
};

unpacked_t unpack(node_ptr node) {
  return (unpacked_t){
    .marked = is_marked(node),
    .address = unmark(node)
    };
}

/** Print out the contents of the skip list along with node heights.
 */
void c_lj_pq_print (c_lj_pq_t *set){
  node_ptr node = set->head.next[0];
  while(unmark(node) != &set->tail) {
    node_ptr unmarked_node = unmark(node);
    printf("node[%d]: %ld deleted: %d\n", unmarked_node->toplevel, unmarked_node->key, is_marked(node));
    node = unmarked_node->next[0];
  }
}

/** Return a new fixed-height skip list.
 */
c_lj_pq_t * c_lj_pq_create(uint32_t boundoffset) {
  c_lj_pq_t* lj_pqueue = forkscan_malloc(sizeof(c_lj_pq_t));
  lj_pqueue->boundoffset = boundoffset;
  lj_pqueue->head.key = INT64_MIN;
  lj_pqueue->head.insert_state = INSERTED;
  lj_pqueue->tail.key = INT64_MAX;
  lj_pqueue->tail.insert_state = INSERTED;
  for(int64_t i = 0; i < N; i++) {
    lj_pqueue->head.next[i] = &lj_pqueue->tail;
    lj_pqueue->tail.next[i] = NULL;
  }
  return lj_pqueue;
}

static uint64_t fast_rand (uint64_t *seed){
  uint64_t val = *seed;
  if(val == 0) {
    val = 1;
  }
  val ^= val << 6;
  val ^= val >> 21;
  val ^= val << 7;
  *seed = val;
  return val;
}

static int32_t random_level (uint64_t *seed, int32_t max) {
  int32_t level = 1;
  while(fast_rand(seed) % 2 == 0 && level < max) {
    level++;
  }
  return level - 1;
}


static node_ptr locate_preds(
  c_lj_pq_t *set, 
  int64_t key,
  node_ptr preds[N],
  node_ptr succs[N]) {
  node_ptr cur = &set->head, next = NULL, del = NULL;
  int32_t level = N - 1;
  bool deleted = false;
  while(level >= 0) {
    next = cur->next[level];
    deleted = is_marked(next);
    next = unmark(next);

    while(next->key < key || 
      is_marked(next->next[0]) || 
      ((level == 0) && deleted)) {
      if(level == 0 && deleted) {
        del = next;
      }
      cur = next;
      next = next->next[level];
      deleted = is_marked(next);
      next = unmark(next);
    }
    preds[level] = cur;
    succs[level] = next;
    level--;
  }
  return del;
}

/** Add a node, lock-free, to the skiplist.
 */
int c_lj_pq_add(uint64_t *seed, c_lj_pq_t * set, int64_t key) {
  node_ptr preds[N], succs[N];
  int32_t toplevel = random_level(seed, N);
  node_ptr node = NULL;
  while(true) {
    node_ptr del = locate_preds(set, key, preds, succs);
    if(succs[0]->key == key &&
      !is_marked(preds[0]->next[0]) &&
      preds[0]->next[0] == succs[0]) {
      if(node != NULL) {
        forkscan_free((void*)node);
      }
      return false;
    }

    if(node == NULL) { node = node_create(key, toplevel); }
    for(int64_t i = 0; i <= toplevel; ++i) { node->next[i] = succs[i]; }
    node_ptr pred = preds[0], succ = succs[0];
    if(!__sync_bool_compare_and_swap(&pred->next[0], succ, node)) { continue; }

    for(int64_t i = 1; i <= toplevel; i++) {

      if(is_marked(node->next[0]) ||
        is_marked(succs[i]->next[0]) ||
        del == succs[i]) {
        node->insert_state = INSERTED;
        return true;
      }

      node->next[i] = succs[i];

      if(!__sync_bool_compare_and_swap(&preds[i]->next[i], succs[i], node)) {
        del = locate_preds(set, key, preds, succs);
        if(succs[0] != node) {
          node->insert_state = INSERTED;
          return true;
        }
      }
    }
    node->insert_state = INSERTED;
    return true;
  }
}


static void restructure(c_lj_pq_t *set) {
  node_ptr pred = NULL, cur = NULL, head = NULL;
  int32_t level = N - 1;
  pred = &set->head;
  while(level > 0) {
    head = set->head.next[level];
    cur = pred->next[level];
    if(!is_marked(head->next[0])) {
      level--;
      continue;
    }
    while(is_marked(cur->next[0])) {
      pred = cur;
      cur = pred->next[level];
    }
    if(__sync_bool_compare_and_swap(&set->head.next[level], head, cur)){
      level--;
    }
  }
}


/** Pop the front node from the list.  Return true iff there was a node to pop.
 *  Leak the memory.
 */
int c_lj_pq_leaky_pop_min(c_lj_pq_t * set) {
  node_ptr cur = NULL, next = NULL, newhead = NULL,
    obs_head = NULL;
  int32_t offset = 0;
  cur = &set->head;
  obs_head = cur->next[0];
  do {
    offset++;
    next = cur->next[0];
    if(unmark(next) == &set->tail) { return false; }
    if(newhead == NULL && cur->insert_state == INSERT_PENDING) { newhead = cur; }
    if(is_marked(next)) { continue; }
    next = (node_ptr)__sync_fetch_and_or((uintptr_t*)&cur->next[0], (uintptr_t)1);
  } while((cur = unmark(next)) && is_marked(next));
  

  if(newhead == NULL) { newhead = cur; }
  if(offset <= set->boundoffset) { return true; }
  if(set->head.next[0] != obs_head) { return true; }

  if(__sync_bool_compare_and_swap(&set->head.next[0], 
    obs_head, mark(newhead))) {
    restructure(set);
  }
  return true;
}