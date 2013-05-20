/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sylvain Genevès wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>

#ifndef __SKIPLIST_H
#define __SKIPLIST_H

typedef enum {false,true} boolean;

#define VALUE_TYPE  int
#define VALUE_UNDEF  (-1)

#define INITIAL_NB_LEVELS (sizeof(VALUE_TYPE)*8)

typedef struct _node {
  struct _node* next;
  struct _node* prev;
  struct _node* up;
  struct _node* down;
  int level;
  VALUE_TYPE value;
} node_t;

typedef struct _skiplist {
  int max_lvl;
  node_t** slist;       //per-level array of lists (node_t*)
} *skiplist_t;

skiplist_t new_skiplist(int maxlvl);
void delete_skiplist(skiplist_t l);

node_t* sl_search(skiplist_t l, VALUE_TYPE v, boolean* found);
int sl_insert(skiplist_t l, VALUE_TYPE v);
boolean sl_remove(skiplist_t l, VALUE_TYPE v);
void sl_print(skiplist_t l);


#endif

