#include "c_split_order_table.h"
#include <forkscan.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef int64_t key_t;
typedef key_t so_key_t;
typedef struct c_soht_node_t c_soht_node_t;
typedef c_soht_node_t volatile * volatile c_soht_node_ptr;
typedef struct c_soht_list_view_t c_soht_list_view_t;

struct c_soht_node_t {
  uint64_t key;
  c_soht_node_ptr next;
};

struct c_soht_t {
  uint64_t max_load;
  volatile size_t size, count;
  c_soht_node_ptr *table;
};

struct c_soht_list_view_t {
  // Only previous needs to be "*", not a mistake.
  c_soht_node_ptr * previous, current, next;
};

static c_soht_node_ptr mark(c_soht_node_ptr ptr) {
  return (c_soht_node_ptr)((uintptr_t)ptr | 0x1);
}

static c_soht_node_ptr unmark(c_soht_node_ptr ptr) {
  return (c_soht_node_ptr)((uintptr_t)ptr & ~0x1);
}

static bool is_marked(c_soht_node_ptr ptr) {
  return ((uintptr_t)ptr & 0x1) == 0x1;
}


// Ref: https://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
static uint64_t reverse_bits(uint64_t key) {
  size_t shift_amount = (sizeof(uint64_t) * 8) - 1;
  uint64_t result = key;
  for(key >>= 1; key; key >>= 1) {
    result <<= 1;
    result |= key & 1;
    shift_amount--;
  }
  return result <<= shift_amount;
}

static uint64_t so_regular_key(uint64_t key) {
  return reverse_bits(key) | 0x1;
}

static uint64_t so_dummy_key(uint64_t key) {
  return reverse_bits(key);
}

static bool is_dummy(uint64_t key) {
  return (key & 0x1) == 0x0;
}

void c_soht_print(c_soht_t *set) {
  for(size_t i = 0; i < set->size; i++) {
    c_soht_node_ptr list = set->table[i];
    c_soht_node_ptr unmarked_list = unmark(list);
    if(list == NULL) continue;
    printf("Dummy node[%lu] with dummy key[%lu] is marked %d\n", so_dummy_key(unmarked_list->key), unmarked_list->key, is_marked(unmarked_list->next));
    assert(is_dummy(unmarked_list->key));
    list = list->next;
    while(list != NULL && !is_dummy(unmark(list)->key)) {
      unmarked_list = unmark(list);
      printf("Node node[%lu] with split-order key[%lu] is marked %d\n", so_dummy_key(unmarked_list->key & ~0x1), unmarked_list->key, is_marked(unmarked_list->next));
      list = unmarked_list->next;
    }
  }
}

static bool find(c_soht_list_view_t * view, c_soht_node_ptr *head, uint64_t key) {
try_again:
  view->previous = head;
  view->current = *head;
  while(true) {
    if(unmark(view->current) == NULL) return false;
    view->next = unmark(view->current)->next;
    uint64_t cur_key = unmark(view->current)->key;
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

static int c_list_add(c_soht_list_view_t *view, c_soht_node_ptr *head, c_soht_node_ptr new_node) {
  uint64_t key = new_node->key;
  while(true) {
    if(find(view, head, key)) {
      return false;
    }
    new_node->next = unmark(view->current);
    if(__sync_bool_compare_and_swap(view->previous, view->current, new_node)) {
      return true;
    }
  }
}

static int c_list_remove_leaky(c_soht_node_ptr *head, uint64_t key) {
  while(true) {
    c_soht_list_view_t view;
    if(!find(&view, head, key)) {
      return false;
    }
    if(!__sync_bool_compare_and_swap(&view.current->next, view.next, mark(view.next))) {
      continue;
    }
    // unmark(view.next) -> the bane of my life
    if(!__sync_bool_compare_and_swap(view.previous, view.current, unmark(view.next))) {
      bool _ = find(&view, head, key);
    }
    return true;
  }
}

static key_t get_parent(size_t bucket) {
  size_t copy_bucket = reverse_bits(bucket);
  for(size_t mask = 1; mask <= copy_bucket; mask = mask << 1) {
    if((copy_bucket & mask) == mask) {
      copy_bucket = copy_bucket & ~mask;
      break;
    }
  }
  return reverse_bits(copy_bucket);
}

static void initialise_bucket(c_soht_t *set, size_t bucket) {
  size_t parent = get_parent(bucket);
  if(set->table[parent] == NULL) {
    initialise_bucket(set, parent);
  }
  c_soht_node_ptr dummy_node = forkscan_malloc(sizeof(c_soht_node_t));
  dummy_node->key = so_dummy_key(bucket);
  c_soht_list_view_t view;
  if(!c_list_add(&view, &set->table[parent], dummy_node)) {
    forkscan_free((void*)dummy_node);
    dummy_node = unmark(view.current);
  }
  set->table[bucket] = dummy_node;
}

c_soht_t * c_soht_create(size_t size, uint64_t max_load) {
  c_soht_t *ret = forkscan_malloc(sizeof(c_soht_t));
  ret->size = size;
  ret->max_load = max_load;
  ret->count = 0;
  ret->table = forkscan_malloc(size * sizeof(c_soht_node_ptr));
  for(uint64_t i = 0; i < size; i++) {
    ret->table[i] = NULL;
  }
  ret->table[0] = forkscan_malloc(sizeof(c_soht_node_t));
  ret->table[0]->key = so_dummy_key(0);
  return ret;
}

int c_soht_contains(c_soht_t *set, key_t key) {
  size_t bucket = key % set->size;
  if(set->table[bucket] == NULL) {
    initialise_bucket(set, bucket);
  }
  c_soht_list_view_t view;
  return find(&view, &set->table[bucket], so_regular_key(key));
}

int c_soht_add(c_soht_t *set, key_t key) {
  c_soht_node_ptr node = forkscan_malloc(sizeof(c_soht_node_t));
  node->key = so_regular_key(key);
  size_t bucket = key % set->size;
  if(set->table[bucket] == NULL) {
    initialise_bucket(set, bucket);
  }
  c_soht_list_view_t view;
  if(!c_list_add(&view, &set->table[bucket], node)) {
    forkscan_free((void*)node);
    return false;
  }
  size_t size = set->size;
  if((__sync_fetch_and_add(&set->count, 1) / size) > set->max_load) {
    // bool _ = __sync_bool_compare_and_swap(&set->size, size, size * 2);
  }
  return true;
}

int c_soht_remove_leaky(c_soht_t *set, key_t key) {
  size_t bucket = key % set->size;
  if(set->table[bucket] == NULL) {
    initialise_bucket(set, bucket);
  }
  if(!c_list_remove_leaky(&set->table[bucket], so_regular_key(key))) {
    return false; 
  }
  size_t _ = __sync_fetch_and_sub(&set->count, 1);
  return true;
}
