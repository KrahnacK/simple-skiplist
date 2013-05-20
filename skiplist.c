/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sylvain Genevès wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */


#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdint.h>
#include "skiplist.h"

#define RANDOM_PRECISION  (8)
static __thread unsigned int seed = 0;
static __thread struct random_data rand_dat;
static __thread char rand_buf[RANDOM_PRECISION];

/*
 * create a new node
 */
static node_t* new_node(VALUE_TYPE value, int level) {
  node_t* res = malloc(sizeof(node_t));
  res->next = NULL;
  res->prev = NULL;
  res->up = NULL;
  res->down = NULL;
  res->value = value;
  res->level = level;
  return res;
}

/*
 * builds a stack of lists (node_t* here)
 * returns highest level node
*/
static node_t* new_node_stack(VALUE_TYPE v, int height) {
  int i;
  node_t* current, *higher;
  node_t* stack = new_node(v, height);
  current = stack;
  for(i = height-1; i >= 0; i--) {
    higher = current;
    current = new_node(v, i);
    higher->down = current;
    current->up = higher;
  }
  return stack;
}

skiplist_t new_skiplist(int max_lvl) {
  int i;
  skiplist_t l = malloc(sizeof(*l));
  l->max_lvl = max_lvl;
  l->slist = malloc( (l->max_lvl+1) * sizeof(*l->slist));
  l->slist[l->max_lvl] = new_node_stack(VALUE_UNDEF, l->max_lvl);
  for (i = l->max_lvl - 1; i>=0; i--) {
    assert( l->slist[i+1]->down != NULL );
    l->slist[i] = l->slist[i+1]->down;  //make our stack accessible through our l->slist array
  }

  if (!seed) {
    //init per-thread random generator
    seed = time(NULL);
    if (initstate_r(seed, (char*)rand_buf, (size_t) RANDOM_PRECISION, &rand_dat))
      perror("initstate_r:");
    if (srandom_r(seed, &rand_dat))
      perror("srandom_r:");
  }
  return l;
}

/*
 * frees a column (stack)
 */
static void delete_nodestack(node_t* l) {
  if (l){
    if (l->down)
      delete_nodestack(l->down);
    free(l);
    l = NULL;
  }
}

/*
 * frees a line (list)
 */
static void delete_nodelist(node_t* l) {
  if (l){
    if (l->next)
      delete_nodelist(l->next);
    free(l);
    l = NULL;
  }
}

/*
 * frees a whole skiplist
 */
void delete_skiplist(skiplist_t l) {
  if (l) {
    int i;
    for (i=0; i< l->max_lvl; i++) { //foreach list in the highest stack (first is assumed to be the highest)
      delete_nodelist(l->slist[i]);
    }
    free(l);
    l = NULL;
  }
}

/*
 * lookup a value
 * found is updated, highest-level node containing the value is returned when found=true
 * when found=false, we return the highest-level node just before where the value should be 
 */
node_t* sl_search(skiplist_t l, VALUE_TYPE v, boolean* found) {
  int lvl = l->max_lvl;
  node_t *current;
  *found = false;
  current = l->slist[lvl];
  while(current && lvl >= 0) {
    do {
      //did we hit it?
      if (current->value == v) {
          *found = true;
          //printf("search: just found it at %p (prev=%p)\n", current, prev);
          return current;
      }

      if (current->next == NULL) break;
      current = current->next;

    } while( current && (current->value == VALUE_UNDEF || current->value <= v));  //TODO: add type-agnostic comparison function
    if (current->prev != NULL)//we got one step too far, go back if we can
      current = current->prev;
    lvl--;
    current = current->down;
  }
  return current; //(v should be inserted just after current)
  //can be NULL if skiplist is empty
}

/*
 * attempts to insert a new value
 * return the level where value is inserted, -1 if value is already present
 */
int sl_insert(skiplist_t l, VALUE_TYPE v) {
  int lvl = 0;
  int32_t rand = 0;
  boolean found;
  if (random_r(&rand_dat, &rand))
    perror("random_r:");
  while( !( ( rand>>lvl ) & 1 )) lvl++; //get index of the first set bit in rand
  lvl %= l->max_lvl;//ensure we conerve the max level
  //FIXME: do we want to dynamically update the max level?

  node_t* new = new_node_stack(v, lvl);
  node_t* where = sl_search(l, v, &found);
  node_t* current = NULL;
  if (where && !found) { //default insert
    current = where;
    //current may be higher than what we want
    while (current->level > new->level)
      current = current->down;
    //or new could be higher than what exists there
    while(current->level < new->level)
      new = new->down;
    assert(current->level == new->level); //now we're sure

  } else if (!where) {  //insert in an empty list
    current = l->slist[lvl];
  } else {              //value already exists
    delete_nodestack(new); //don't duplicate values
    return -1;
  }

  //now we insert the new stack where it belongs
  while ( new && new->level >= 0 ){

    //insert new between current and current->next
    //('up' and 'down' links are already made, just update 'next' and 'prev' links)
    new->next = current->next;
    current->next = new;

    if (new->next)
      new->next->prev = new;
    new->prev = current;

    assert(new->level == current->level); //everyone's on the same level
    assert( (new->level && new->down) || (new->level == 0 && new->down == NULL) ); //down link exists when it should

    /*
    //some debug prints
    printf("lvl:%d new->level:%d new->down=%p new->next=%p (new->value=%d)\n", lvl, new->level, 
        new->down, new->next, new->value);
    printf("lvl:%d current->level:%d current->down=%p current->next=%p (current->value=%d)\n", lvl, current->level, 
        current->down, current->next, current->value);
    printf("----\n");
    */

    current = current->down;
    new = new->down;
  }
  return lvl;
}

/*
 * removes a value
 * removes the whole corresponding node stack
 * returns true if done, false if value not present
 */
boolean sl_remove(skiplist_t l, VALUE_TYPE v) {
  boolean found;
  node_t* save_for_garbage = NULL, *to_remove = NULL;
  to_remove = sl_search(l, v, &found);
  if (!found)
    return false;
  
  save_for_garbage = to_remove;
  while(to_remove) {
    //relink
    if (to_remove->next && to_remove->prev) {
      to_remove->next->prev = to_remove->prev;
      to_remove->prev->next = to_remove->next;
    } else if (to_remove->next) {
      to_remove->next->prev = NULL;
    } else if (to_remove->prev) { //always true
      to_remove->prev->next = NULL;
    } else assert(false);
    //and go down
    to_remove = to_remove->down;
  }

  //then we destroy removed value stack
  delete_nodestack(save_for_garbage);
  return true;
}

/*
 * print a skiplist
 */
void sl_print(skiplist_t l) {
  int lvl = l->max_lvl;
  while (lvl >= 0) {
    printf("[%2d]:", lvl);
    node_t* current = l->slist[lvl];
    node_t* ref = l->slist[0];
    while (current) {
      assert(current->level == lvl);
      while(ref->value != current->value) {//check how much space we should add
        printf("--------");
        ref = ref->next;
      }
      printf("->%5d ", current->value);
      assert(current->next == NULL || current->next->prev == current);
      current = current->next;
      ref = ref->next;
    }
    printf("\n");
    lvl--;
  }
}

