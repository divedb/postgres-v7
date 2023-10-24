#include <CUnit/Basic.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "rdbms/storage/s_lock.h"

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      CU_cleanup_registry();                             \
      return CU_get_error();                             \
    }                                                    \
  } while (0)

static long long Gcount;
static slock_t GLock;

static void* routine() {
  for (int i = 0; i < 100000; i++) {
    S_LOCK(&GLock);

    if (i % 2 == 0) {
      Gcount += i;
    }

    S_UNLOCK(&GLock);
  }

  pthread_exit(NULL);
}

static void test_lock_and_unlock() {
#define NTask 4

  pthread_t task_group[NTask];

  for (int i = 0; i < NTask; i++) {
    if (pthread_create(&task_group[i], NULL, routine, NULL) < 0) {
      fprintf(stderr, "pthread_create error\n");
      exit(i);
    }
  }

  for (int i = 0; i < NTask; i++) {
    if (pthread_join(task_group[i], NULL) < 0) {
      fprintf(stderr, "pthread_join error\n");
      exit(i);
    }
  }

  CU_ASSERT(Gcount == 9999800000);

#undef NTask
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("SLock", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  Gcount = 0;
  S_INIT_LOCK(&GLock);

  ADD_TEST_MACRO(suite, "test of lock and unlock.", test_lock_and_unlock);

  S_LOCK_FREE(&GLock);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}