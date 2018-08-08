/* Fixed height skiplist: A skiplist implementation with an array of "next"
 * nodes of fixed height.
 */

#include "c_fhsl_lf.h"

#include <stdbool.h>
#include <forkscan.h>
#include <stdio.h>


typedef struct node_t node_t;
typedef node_t volatile * volatile node_ptr;
typedef struct node_unpacked_t node_unpacked_t;

struct node_t {
  int64_t key;
  int32_t toplevel;
  node_ptr next[N];
};

struct c_fhsl_lf_t {
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

/** Print out the contents of the skip list along with node heights.
 */
void c_fhsl_lf_print (c_fhsl_lf_t *set){
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

/** Return a new fixed-height skip list.
 */
c_fhsl_lf_t * c_fhsl_lf_create() {
  c_fhsl_lf_t* fhsl_lf = forkscan_malloc(sizeof(c_fhsl_lf_t));
  fhsl_lf->head.key = INT64_MIN;
  fhsl_lf->tail.key = INT64_MAX;
  for(int64_t i = 0; i < N; i++) {
    fhsl_lf->head.next[i] = &fhsl_lf->tail;
    fhsl_lf->tail.next[i] = NULL;
  }
  return fhsl_lf;
}

/** Return whether the skip list contains the value.
 */
int c_fhsl_lf_contains(c_fhsl_lf_t *set, int64_t key) {
  node_ptr node = &set->head;
  for(int64_t i = N - 1; i >= 0; i--) {
    node_ptr next = node_unmark(node->next[i]);
    while(next->key <= key) {
      node = next;
      next = node_unmark(node->next[i]);
    }
    if(node->key == key) {
      return !node_is_marked(node->next[0]);
    }
  }
  return false;
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

static bool find(c_fhsl_lf_t *set, int64_t key, 
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

/** Add a node, lock-free, to the skiplist.
 */
int c_fhsl_lf_add(uint64_t *seed, c_fhsl_lf_t * set, int64_t key) {
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
int c_fhsl_lf_remove_leaky(c_fhsl_lf_t * set, int64_t key) {
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