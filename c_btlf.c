#include "c_btlf.h"
#include <assert.h>
#include <forkscan.h>


typedef struct node_t node_t;
typedef node_t volatile * volatile node_ptr;
typedef struct seek_record_t seek_record_t;
typedef struct node_unpacked_t node_unpacked_t;

struct node_t {
    int64_t key;
    node_ptr left, right;
};

struct c_btlf_t {
    node_ptr R, S;
};

struct seek_record_t {
    node_ptr ancestor, successor, parent, leaf;
};

enum REMOVE_STATE {INJECTION, CLEANUP};

struct node_unpacked_t {
    bool flagged, tagged;
    node_ptr address;
};

node_t* c_bt_lf_node_create(int64_t key){
    node_t* node = forkscan_malloc(sizeof(node_t));
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    return node;
}

node_ptr c_bt_lf_node_address(node_ptr node){
    return (node_ptr)(((size_t)node) & (~0x3));
}

node_ptr c_bt_lf_node_flag(node_ptr node, bool flag){
    if(flag){
        return (node_ptr)((size_t)node | 0x1);
    } else {
        return (node_ptr)((size_t)node & (~0x1));
    }
}

bool c_bt_lf_node_is_flagged(node_ptr node){
    return ((size_t)node & 0x1) == 1;
}

node_ptr c_bt_lf_node_tag(node_ptr node, bool tag){
    if(tag){
        return (node_ptr)((size_t)node | 0x2);
    } else {
        return (node_ptr)((size_t)node & (~0x2));
    }
}

bool c_bt_lf_node_is_tagged(node_ptr node){
    return ((size_t)node & 0x2) == 2;
}

node_ptr c_bt_lf_node_pack(node_ptr node, bool flag, bool tag){
    return c_bt_lf_node_tag(c_bt_lf_node_flag(c_bt_lf_node_address(node), flag), tag);
}

node_unpacked_t c_bt_lf_node_unpack(node_ptr node){
    return (node_unpacked_t){
        .flagged = c_bt_lf_node_is_flagged(node),
        .tagged = c_bt_lf_node_is_tagged(node),
        .address = c_bt_lf_node_address(node)
        };
}

c_btlf_t* c_bt_lf_create(){
    c_btlf_t * bt_lf = forkscan_malloc(sizeof(c_btlf_t));
    bt_lf->R = c_bt_lf_node_create(INT64_MAX);
    bt_lf->S = c_bt_lf_node_create(INT64_MAX - 1);
    bt_lf->R->left = bt_lf->S;
    bt_lf->S->left = c_bt_lf_node_create(INT64_MAX - 2);
    bt_lf->S->right = c_bt_lf_node_create(INT64_MAX - 1);
    return bt_lf;
}

void c_bt_lf_init_seek_record(c_btlf_t *bt_lf, seek_record_t* sr){
    sr->ancestor = bt_lf->R;
    sr->successor = bt_lf->S;
    sr->parent = bt_lf->S;
    sr->leaf = c_bt_lf_node_address(bt_lf->S->left);
}

node_ptr c_bt_lf_node_setup(int64_t key, int64_t sibbling_key, node_ptr sibbling_node){
    node_t * node = c_bt_lf_node_create(key);
    node_t * internal_node = c_bt_lf_node_create(key);
    if(key < sibbling_key){
        internal_node->left = node;
        internal_node->right = sibbling_node;
        internal_node->key = sibbling_key;
    } else {
        internal_node->left = sibbling_node;
        internal_node->right = node;
    }
    return internal_node;
}

void c_bt_lf_seek(c_btlf_t * set, seek_record_t * sr, int64_t key){
    c_bt_lf_init_seek_record(set, sr);
    volatile node_t * parent_field = sr->parent->left;
    volatile node_t * current_field = sr->leaf->left;
    volatile node_t * current = c_bt_lf_node_address(current_field);

    while(current != NULL){
        if(!c_bt_lf_node_is_tagged(parent_field)){
            sr->ancestor = sr->parent;
            sr->successor = sr->leaf;
        }
        sr->parent = sr->leaf;
        sr->leaf = current;
        parent_field = current_field;
        if(key < current->key){
            current_field = current->left;
        }else {
            current_field = current->right;
        }
        current = c_bt_lf_node_address(current_field);
    }
}

bool c_bt_lf_cleanup(c_btlf_t * set, seek_record_t *sr, int64_t key) {
    node_ptr ancestor = sr->ancestor, successor = sr->successor, parent = sr->parent, leaf = sr->leaf;

    node_ptr volatile* successor_address = NULL;
    if(key < ancestor->key) {
        successor_address = &ancestor->left;
    } else {
        successor_address = &ancestor->right;
    }
    node_ptr child_val = NULL;
    node_ptr volatile* child_address = NULL;
    node_ptr sibling_val = NULL;
    node_ptr volatile* sibling_address = NULL;
    if(key < parent->key) {
        child_val = parent->left;
        child_address = &parent->left;
        sibling_val = parent->right;
        sibling_address = &parent->right;
    } else {
        child_val = parent->right;
        child_address = &parent->right;
        sibling_val = parent->left;
        sibling_address = &parent->left;
    }
    node_unpacked_t unpacked_node = c_bt_lf_node_unpack(*child_address);
    if(!unpacked_node.flagged) {
        sibling_val = child_val;
        sibling_address = child_address;
    }

    __sync_bool_compare_and_swap(sibling_address,
        sibling_val,
        c_bt_lf_node_tag(c_bt_lf_node_address(sibling_val), true));
    node_unpacked_t unpacked_sibbling = c_bt_lf_node_unpack(*sibling_address);
    bool result = __sync_bool_compare_and_swap(successor_address,
        c_bt_lf_node_address(successor),
        c_bt_lf_node_flag(unpacked_sibbling.address, unpacked_sibbling.flagged));
    return result;
}

int c_bt_lf_contains(c_btlf_t *set, int64_t key) {
    seek_record_t sr;
    c_bt_lf_seek(set, &sr, key);
    return sr.leaf->key == key;
}


int c_bt_lf_add(c_btlf_t *set, int64_t key) {
    while(true) {
        seek_record_t sr;
        c_bt_lf_seek(set, &sr, key);
        int64_t leaf_key = sr.leaf->key;
        if(leaf_key != key) {
            node_ptr parent = sr.parent;
            node_ptr leaf = sr.leaf;
            node_ptr volatile* child_address = NULL;
            int64_t parent_key = parent->key;
            if(key < parent_key) {
                child_address = &parent->left;
            } else {
                child_address = &parent->right;
            }
            node_ptr internal_node = c_bt_lf_node_setup(key, leaf_key, leaf);
            bool result = __sync_bool_compare_and_swap(child_address,
                c_bt_lf_node_address(leaf),
                internal_node);
            if(result) {
                return true;
            } else {
                if(key < leaf_key) {
                    forkscan_free((void *)internal_node->left);
                } else {
                    forkscan_free((void *)internal_node->right);
                }
                forkscan_free((void *)internal_node);
                node_unpacked_t unpacked_node = c_bt_lf_node_unpack(*child_address);
                if(unpacked_node.address == leaf &&
                    (unpacked_node.flagged || unpacked_node.tagged)){
                    bool done = c_bt_lf_cleanup(set, &sr, key);
                }
            }
        } else {
            return false;
        }
    }
}

int c_bt_lf_remove_leaky(c_btlf_t * set, int64_t key) {
    enum REMOVE_STATE mode = INJECTION;
    node_ptr leaf = NULL;
    while(true) {
        seek_record_t sr;
        c_bt_lf_seek(set, &sr, key);
        node_ptr parent = sr.parent;
        node_ptr volatile* child_address = NULL;
        int64_t parent_key = parent->key;
        if(key < parent_key) {
            child_address = &parent->left;
        } else {
            child_address = &parent->right;
        }
        if(mode == INJECTION) {
            leaf = sr.leaf;
            if(leaf->key != key) {
                return false;
            }
            bool result = __sync_bool_compare_and_swap(child_address,
                c_bt_lf_node_address(leaf),
                c_bt_lf_node_flag(leaf, true));
            if(result) {
                mode = CLEANUP;
                bool done = c_bt_lf_cleanup(set, &sr, key);
                if(done) {
                    return true;
                }
            } else {
                node_unpacked_t unpacked_node = c_bt_lf_node_unpack(*child_address);
                if(unpacked_node.address == leaf &&
                    (unpacked_node.flagged || unpacked_node.tagged)){
                    bool done = c_bt_lf_cleanup(set, &sr, key);
                }
            }
        } else {
            if(sr.leaf != leaf) {
                return true;
            } else {
                bool done = c_bt_lf_cleanup(set, &sr, key);
                if(done) {
                    return true;
                }
            }
        }
    }
}

int c_bt_lf_remove_retire(c_btlf_t * set, int64_t key) {
    enum REMOVE_STATE mode = INJECTION;
    node_ptr leaf = NULL;
    while(true) {
        seek_record_t sr;
        c_bt_lf_seek(set, &sr, key);
        node_ptr parent = sr.parent;
        node_ptr volatile* child_address = NULL;
        int64_t parent_key = parent->key;
        if(key < parent_key) {
            child_address = &parent->left;
        } else {
            child_address = &parent->right;
        }
        if(mode == INJECTION) {
            leaf = sr.leaf;
            if(leaf->key != key) {
                return false;
            }
            bool result = __sync_bool_compare_and_swap(child_address,
                c_bt_lf_node_address(leaf),
                c_bt_lf_node_flag(leaf, true));
            if(result) {
                mode = CLEANUP;
                bool done = c_bt_lf_cleanup(set, &sr, key);
                if(done) {
                    forkscan_retire((void *)leaf);
                    return true;
                }
            } else {
                node_unpacked_t unpacked_node = c_bt_lf_node_unpack(*child_address);
                if(unpacked_node.address == leaf &&
                    (unpacked_node.flagged || unpacked_node.tagged)){
                    bool done = c_bt_lf_cleanup(set, &sr, key);
                }
            }
        } else {
            if(sr.leaf != leaf) {
                forkscan_retire((void *)leaf);
                return true;
            } else {
                bool done = c_bt_lf_cleanup(set, &sr, key);
                if(done) {
                    forkscan_retire((void *)leaf);
                    return true;
                }
            }
        }
    }
}