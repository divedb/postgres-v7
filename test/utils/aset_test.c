#include "rdbms/utils/aset.h"

#include <CUnit/Basic.h>
#include <assert.h>
#include <stdlib.h>

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      CU_cleanup_registry();                             \
      return CU_get_error();                             \
    }                                                    \
  } while (0)

void test_init() {
  AllocSetData set_data;
  AllocSet aset = &set_data;

  alloc_set_init(aset, DynamicAllocMode, 0);
  alloc_set_reset(aset);
}

void test_alloc_and_dealloc() {
  int* i;
  AllocSetData set_data;
  AllocSet aset = &set_data;

  alloc_set_init(aset, DynamicAllocMode, 0);

  i = (int*)malloc(sizeof(*i));
  CU_ASSERT(!alloc_set_contains(aset, (Pointer)i));
  free(i);

#define N 10

  Pointer array[N];
  int size[] = {1e1, 1e2, 1e3, 1e4, 1e5, 1e6};

  for (int i = 0; i < LENGTH_OF(size); i++) {
    int sz = size[i];

    for (int j = 0; j < N; j++) {
      array[j] = alloc_set_alloc(aset, sz);

      Pointer p = array[j];
      // Write some data into memory.
      for (int k = 0; k < sz; k++) {
        *p++ = 'A';
      }
    }

    for (int j = N - 1; j >= 0; j--) {
      CU_ASSERT(alloc_set_contains(aset, array[j]));
      alloc_set_free(aset, array[j]);
    }
  }

  alloc_set_reset(aset);

#undef N
}

void test_realloc() {
  int sz = 4096;
  AllocSetData set_data;
  AllocSet aset = &set_data;

  alloc_set_init(aset, DynamicAllocMode, 0);

  Pointer p = alloc_set_alloc(aset, sz);
  Pointer old_p = p;

  for (int i = 0; i < sz; i++) {
    *p++ = 'A';
  }

  p = alloc_set_realloc(aset, old_p, 1 << 20);

  // Check the content is copied.
  for (int i = 0; i < sz; i++) {
    CU_ASSERT(p[i] == 'A');
  }

  alloc_set_free(aset, p);
  alloc_set_reset(aset);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("OrderedSet", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  ADD_TEST_MACRO(suite, "test of init allocator set", test_init);
  ADD_TEST_MACRO(suite, "test of allocate and deallocate", test_alloc_and_dealloc);
  ADD_TEST_MACRO(suite, "test of reallocate", test_realloc);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}