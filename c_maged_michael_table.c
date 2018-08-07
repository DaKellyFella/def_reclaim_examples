#include "c_maged_michael_table.h"
#include <forkscan.h>
#include <stdbool.h>

typedef int64_t key_t;
typedef struct c_mmht_node_t c_mmht_node_t;
typedef c_mmht_node_t volatile * volatile c_mmht_node_ptr;
typedef struct c_mmht_list_view_t c_mmht_list_view_t;

struct c_mmht_node_t {
  key_t key;
  c_mmht_node_ptr next;
};

struct c_mmht_t {
  uint64_t size;
  c_mmht_node_ptr *table;
};

struct c_mmht_list_view_t {
  // Only previous needs to be "*", not a mistake.
  c_mmht_node_ptr * previous, current, next;
};

static uint64_t hash(key_t key) {
  return key;
}

static c_mmht_node_ptr mark(c_mmht_node_ptr ptr) {
  return (c_mmht_node_ptr)((uintptr_t)ptr | 0x1);
}

static c_mmht_node_ptr unmark(c_mmht_node_ptr ptr) {
  return (c_mmht_node_ptr)((uintptr_t)ptr & (~0x1));
}

static bool is_marked(c_mmht_node_ptr ptr) {
  return ((uintptr_t)ptr & 0x1) == 0x1;
}

static bool find(c_mmht_list_view_t * view, c_mmht_node_ptr *head, key_t key) {
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
    if(!is_marked(view->current)) {
      if(cur_key >= key) {
        return cur_key == key;
      }
      view->previous = &unmark(view->current)->next;
    } else {
      // Shortened down since it's leaky memory.
      if(!__sync_bool_compare_and_swap(view->previous, view->current, view->next)) {
        goto try_again;
      }
    }
    view->current = view->next;
  }
  return true;
}

c_mmht_t * c_mmht_create(uint64_t size) {
  c_mmht_t *ret = forkscan_malloc(sizeof(c_mmht_t));
  ret->size = size;
  ret->table = forkscan_malloc(size * sizeof(c_mmht_node_ptr));
  for(uint64_t i = 0; i < size; i++) {
    ret->table[i] = NULL;
  }
  return ret;
}

int c_mmht_contains(c_mmht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  c_mmht_list_view_t view;
  return find(&view, &set->table[bucket], key);
}

int c_mmht_add(c_mmht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  c_mmht_node_ptr new_node = forkscan_malloc(sizeof(c_mmht_node_t));
  new_node->key = key;
  while(true) {
    c_mmht_list_view_t view;
    if(find(&view, &set->table[bucket], key)) {
      forkscan_free((void*)new_node);
      return false;
    }
    new_node->next = unmark(view.current);
    if(__sync_bool_compare_and_swap(view.previous, view.current, new_node)) {
      return true;
    }
  }
}

int c_mmht_remove_leaky(c_mmht_t * set, key_t key) {
  uint64_t key_hash = hash(key);
  uint64_t bucket = key_hash % set->size;
  while(true) {
    c_mmht_list_view_t view;
    if(!find(&view, &set->table[bucket], key)) {
      return false;
    }
    if(!__sync_bool_compare_and_swap(&view.current->next, view.next, mark(view.next))) {
      continue;
    }
    if(!__sync_bool_compare_and_swap(view.previous, view.current, unmark(view.next))) {
      find(&view, &set->table[bucket], key);
    }
    return true;
  }
}