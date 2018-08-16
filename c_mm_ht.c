#include "c_mm_ht.h"
#include <forkscan.h>
#include <stdbool.h>

typedef int64_t key_t;
typedef struct node_t node_t;
typedef node_t volatile * volatile node_ptr;
typedef struct list_view_t list_view_t;

struct node_t {
  key_t key;
  node_ptr next;
};

struct c_mm_ht_t {
  uint64_t size;
  node_ptr *table;
};

struct list_view_t {
  // Only previous needs to be "*", not a mistake.
  node_ptr * previous, current, next;
};

static uint64_t hash(key_t key) {
  return key;
}

static node_ptr mark(node_ptr ptr) {
  return (node_ptr)((uintptr_t)ptr | 0x1);
}

static node_ptr unmark(node_ptr ptr) {
  return (node_ptr)((uintptr_t)ptr & (~0x1));
}

static bool is_marked(node_ptr ptr) {
  return ((uintptr_t)ptr & 0x1) == 0x1;
}

static bool find(list_view_t * view, node_ptr *head, key_t key) {
try_again:
  view->previous = head;
  view->current = *head;
  while(true) {
    if(unmark(view->current) == NULL) return false;
    view->next = unmark(view->current)->next;
    key_t cur_key = unmark(view->current)->key;
    if(*view->previous != unmark(view->current)) {
      goto try_again;
    }
    if(!is_marked(view->next)) {
      if(cur_key >= key) {
        return cur_key == key;
      }
      view->previous = &unmark(view->current)->next;
    } else {
      // Shortened down since it's leaky memory.
      if(!__sync_bool_compare_and_swap(view->previous, unmark(view->current), unmark(view->next))) {
        goto try_again;
      }
    }
    view->current = view->next;
  }
}

c_mm_ht_t * c_mm_ht_create(uint64_t size, uint64_t list_length) {
  c_mm_ht_t *ret = forkscan_malloc(sizeof(c_mm_ht_t));
  ret->size = size / list_length;
  ret->table = forkscan_malloc(ret->size  * sizeof(node_ptr));
  for(uint64_t i = 0; i < ret->size; i++) {
    ret->table[i] = NULL;
  }
  return ret;
}

int c_mm_ht_contains(c_mm_ht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  list_view_t view;
  return find(&view, &set->table[bucket], key);
}

int c_mm_ht_add(c_mm_ht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  node_ptr new_node = NULL;
  while(true) {
    list_view_t view;
    if(find(&view, &set->table[bucket], key)) {
      forkscan_free((void*)new_node);
      return false;
    }
    if(new_node == NULL) {
      new_node = forkscan_malloc(sizeof(node_t));
      new_node->key = key;
    }
    new_node->next = unmark(view.current);
    if(__sync_bool_compare_and_swap(view.previous, unmark(view.current), new_node)) {
      return true;
    }
  }
}

int c_mm_ht_remove_leaky(c_mm_ht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  while(true) {
    list_view_t view;
    if(!find(&view, &set->table[bucket], key)) {
      return false;
    }
    if(!__sync_bool_compare_and_swap(&view.current->next, unmark(view.next), mark(view.next))) {
      continue;
    }
    if(!__sync_bool_compare_and_swap(view.previous, unmark(view.current), unmark(view.next))) {
      find(&view, &set->table[bucket], key);
    }
    return true;
  }
}