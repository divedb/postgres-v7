#ifndef TEST_TEMPLATE_H_
#define TEST_TEMPLATE_H_

#include <CUnit/Basic.h>

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      goto clean_up;                                     \
    }                                                    \
  } while (0)

typedef void (*FuncPtr)();

typedef struct TestCase {
  const char* description;
  FuncPtr fp;
} TestCase;

static int count = 0;
static TestCase ts[1024];

#define TEST(desc, func_ptr)      \
  do {                            \
    ts[count].description = desc; \
    ts[count++].fp = func_ptr;    \
  } while (0)

#define MAIN(suite)                                        \
  int main(int argc, char** argv) {                        \
    if (CUE_SUCCESS != CU_initialize_registry()) {         \
      return CU_get_error();                               \
    }                                                      \
    CU_pSuite _suite = CU_add_suite((suite), NULL, NULL);  \
    if (_suite == NULL) {                                  \
      goto clean_up;                                       \
    }                                                      \
    register_test();                                       \
    for (int i = 0; i < count; i++) {                      \
      ADD_TEST_MACRO(_suite, ts[i].description, ts[i].fp); \
    }                                                      \
    CU_basic_set_mode(CU_BRM_VERBOSE);                     \
    CU_basic_run_tests();                                  \
  clean_up:                                                \
    CU_cleanup_registry();                                 \
    return CU_get_error();                                 \
  }

#endif  // TEST_TEMPLATE_H_
