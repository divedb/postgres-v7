#include <CUnit/Basic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "rdbms/storage/s_lock.h"

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      goto clean_up;                                     \
    }                                                    \
  } while (0)

static TasLock Lock;
static long long Count;

static void* routine() {
  for (int i = 0; i < 100000; i++) {
    LOCK_ACQUIRE(&Lock);

    if (i % 2 == 0) {
      Count += i;
    }

    LOCK_RELEASE(&Lock);
  }

  pthread_exit(NULL);
}

static void test_lock_and_unlock() {
  Count = 0;
  INIT_LOCK(&Lock);

#define NTASK 4

  int err;
  pthread_t task_group[NTASK];

  for (int i = 0; i < NTASK; i++) {
    if ((err = pthread_create(&task_group[i], NULL, routine, NULL)) != 0) {
      fprintf(stderr, "pthread_create error: %s\n", strerror(err));
      exit(i);
    }
  }

  for (int i = 0; i < NTASK; i++) {
    if ((err = pthread_join(task_group[i], NULL)) < 0) {
      fprintf(stderr, "pthread_join error: %s\n", strerror(err));
      exit(i);
    }
  }

  CU_ASSERT(Count == 9999800000);

#undef NTASK

  CU_ASSERT(LOCK_IS_FREE(&Lock));
}

int main() {
  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  CU_pSuite suite = CU_add_suite("SLock", NULL, NULL);

  if (suite == NULL) {
    goto clean_up;
  }

  ADD_TEST_MACRO(suite, "Test lock and unlock with 4 threads.", test_lock_and_unlock);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

clean_up:
  CU_cleanup_registry();

  return CU_get_error();
}