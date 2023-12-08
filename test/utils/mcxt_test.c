#include <CUnit/Basic.h>

#define MEMORY_CONTEXT_CHECKING
#define HAVE_ALLOC_INFO

#include "rdbms/utils/memutils.h"

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      goto clean_up;                                     \
    }                                                    \
  } while (0)

void test_alloc_and_free() {
  memory_context_init();

  int[] sizes = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6};

  for (int sz : sizes) {
    palloc(sz);
  }

  for (int sz : sizes) {
    pfree(sz);
  }
}

int main() {
  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  CU_pSuite suite = CU_add_suite("Memory Context", NULL, NULL);

  if (suite == NULL) {
    goto clean_up;
  }

  ADD_TEST_MACRO(suite, "Test alloc and free.", test_alloc_and_free);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

clean_up:
  CU_cleanup_registry();

  return CU_get_error();
}