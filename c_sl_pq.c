/* A Shavit Lotan priority queue written in C.
 * Like the original description in the paper this queue has a skiplist
 * as the underlying data-structure for ordering elements.
 * The data-structure is lock-free and quiescently consistent.
 */

#include "c_sl_pq.h"

#include <stdbool.h>
#include <forkscan.h>
#include <stdio.h>


typedef struct node_t node_t; 
typedef node_t volatile * volatile node_ptr;
typedef struct node_unpacked_t node_unpacked_t;

struct node_t {
  int64_t key;
  int32_t toplevel;
  volatile int deleted;
  node_ptr next[N];
};

struct c_sl_pq_t {
  node_t head, tail;
};

struct node_unpacked_t {
  bool marked;
  node_ptr address;
};

static node_ptr node_create(int64_t key, int32_t toplevel){
  node_ptr node = forkscan_malloc(sizeof(node_t));
  node->key = key;
  node->toplevel = toplevel;
  node->deleted = false;
  return node;
}

static node_ptr node_unmark(node_ptr node){
  return (node_ptr)(((size_t)node) & (~0x1));
}

static node_ptr node_mark(node_ptr node){
  return (node_ptr)((size_t)node | 0x1);
}

static bool node_is_marked(node_ptr node){
  return node_unmark(node) != node;
}

static node_unpacked_t node_unpack(node_ptr node){
  return (node_unpacked_t){
    .marked = node_is_marked(node),
    .address = node_unmark(node)
    };
}

void c_sl_pq_print (c_sl_pq_t *set){
  node_ptr node = set->head.next[0];
  while(node_unmark(node) != &set->tail) {
    if(node_is_marked(node->next[0])) {
      node = node_unmark(node)->next[0];
    } else {
      node = node_unmark(node);
      printf("node[%d]: %ld\n", node->toplevel, node->key);
      node = node->next[0];
    }
  }
}

/** Return a new shavit lotan priority queue.
 */
c_sl_pq_t* c_sl_pq_create() {
  c_sl_pq_t* sl_pqueue = forkscan_malloc(sizeof(c_sl_pq_t));
  sl_pqueue->head.key = INT64_MIN;
  sl_pqueue->tail.key = INT64_MAX;
  for(int64_t i = 0; i < N; i++) {
    sl_pqueue->head.next[i] = &sl_pqueue->tail;
    sl_pqueue->tail.next[i] = NULL;
  }
  return sl_pqueue;
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

static bool find(c_sl_pq_t *set, int64_t key, 
  node_ptr preds[N], node_ptr succs[N]) {
  bool marked, snip;
  node_ptr pred = NULL, curr = NULL, succ = NULL;
retry:
  while(true) {
    pred = &set->head;
    for(int64_t level = N - 1; level >= 0; --level) {
      curr = node_unmark(pred->next[level]);
      while(true) {
        node_unpacked_t unpacked_node = node_unpack(curr->next[level]);
        succ = unpacked_node.address;
        marked = unpacked_node.marked;
        while(unpacked_node.marked) {
          snip = __sync_bool_compare_and_swap(&pred->next[level], curr, succ);
          if(!snip) {
            goto retry;
          }
          curr = node_unmark(pred->next[level]);
          unpacked_node = node_unpack(curr->next[level]);
          succ = unpacked_node.address;
          marked = unpacked_node.marked;
        }
        if(curr->key < key) {
          pred = curr;
          curr = succ;
        } else {
          break;
        }
      }
      preds[level] = pred;
      succs[level] = curr;
    }
    return curr->key == key;
  }
}

/** Add a node, lock-free, to the Shavit Lotan priority queue.
 */
int c_sl_pq_add(uint64_t *seed, c_sl_pq_t * set, int64_t key) {
  node_ptr preds[N], succs[N];
  int32_t toplevel = random_level(seed, N);
  node_ptr node = NULL;
  while(true) {
    if(find(set, key, preds, succs)) {
      if(node != NULL) {
        forkscan_free((void*)node);
      }
      return false;
    }
    if(node == NULL) { node = node_create(key, toplevel); }
    for(int64_t i = 0; i <= toplevel; ++i) {
      node->next[i] = node_unmark(succs[i]);
    }
    node_ptr pred = preds[0], succ = succs[0];
    if(!__sync_bool_compare_and_swap(&pred->next[0], node_unmark(succ), node)) {
      continue;
    }
    for(int64_t i = 1; i <= toplevel; i++) {
      while(true) {
        pred = preds[i], succ = succs[i];
        if(__sync_bool_compare_and_swap(&pred->next[i],
          node_unmark(succ), node)){
          break;
        }
        find(set, key, preds, succs);
      }
    }
    return true;
  }
}

/** Remove a node, lock-free, from the skiplist.
 */
int c_sl_pq_remove_leaky(c_sl_pq_t * set, int64_t key) {
  node_ptr preds[N], succs[N];
  node_ptr succ = NULL;
  while(true) {
    if(!find(set, key, preds, succs)) {
      return false;
    }
    node_ptr node_to_remove = succs[0];
    bool marked;
    for(int64_t level = node_to_remove->toplevel; level >= 1; --level) {
      succ = node_to_remove->next[level];
      marked = node_is_marked(succ);
      while(!marked) {
        bool _ = __sync_bool_compare_and_swap(&node_to_remove->next[level],
          node_unmark(succ), node_mark(succ));
        succ = node_to_remove->next[level];
        marked = node_is_marked(succ);
      }
    }
    succ = node_to_remove->next[0];
    marked = node_is_marked(succ);
    while(true) {
      bool i_marked_it = __sync_bool_compare_and_swap(&node_to_remove->next[0],
        node_unmark(succ), node_mark(succ));
      succ = succs[0]->next[0];
      marked = node_is_marked(succ);
      if(i_marked_it) {
        find(set, key, preds, succs);
        return true;
      } else if(marked) {
        return false;
      }
    }
  }
}

/** Remove the minimum element in the Shavit Lotan priority queue.
 */
int c_sl_pq_leaky_pop_min(c_sl_pq_t * set) {
  while(true) {
    node_ptr curr = node_unmark(set->head.next[0]);
    if(curr == &set->tail) {
      return false;
    }
    for(; curr != &set->tail; curr = node_unmark(curr->next[0])) {
      if(curr->deleted) {
        continue;
      }
      if(__sync_bool_compare_and_swap(&curr->deleted, false, true)){
        return c_sl_pq_remove_leaky(set, curr->key);
      }
    }
  }
}
