#include <stdio.h>
#include <assert.h>
#include "skiplist.h"

void test_insert(skiplist_t l, VALUE_TYPE v) {
  int lvl_inserted;
  lvl_inserted = sl_insert(l, v);
  printf("%s value %d in skiplist (lvl=%d)\n", lvl_inserted>-1?"inserted":"didn't insert", v, lvl_inserted);
  sl_print(l);
}

void test_remove(skiplist_t l, VALUE_TYPE v) {
  boolean removed;
  removed = sl_remove(l, v);
  printf("%s %d from skiplist\n", removed?"removed":"didn't remove",v);
  sl_print(l);
}

void test_search(skiplist_t l, VALUE_TYPE v) {
  boolean found;
  node_t* n = NULL;
  n = sl_search(l, v, &found);
  printf("search %d on list: %s (%p)\n", v, found?"found":"not found", n);
}

int main(int argc, char** argv) {
  VALUE_TYPE v = 42;

  skiplist_t l = new_skiplist(5);
  sl_print(l);

  test_search(l, v);

  test_insert(l, v);

  v = 5;
  test_insert(l, v);

  v = 3;
  test_insert(l, v);

  v = 42;
  test_search(l, v);

  v = 5;
  test_remove(l, v);

  v = 42;
  test_remove(l, v);

  v = 3;
  test_remove(l, v);

  v = 3;
  test_remove(l, v);

  delete_skiplist(l);
  return 0;
}
