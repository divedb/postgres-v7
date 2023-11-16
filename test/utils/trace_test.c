#include "rdbms/utils/trace.h"

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

void test_tprintf() {
  int age = 100;

  tprintf(TRACE_ALL, "%d\n", age);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("Trace", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  ADD_TEST_MACRO(suite, "test of tprintf", test_tprintf);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}