#include "rdbms/storage/ipc.h"

#include <CUnit/Basic.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      CU_cleanup_registry();                             \
      return CU_get_error();                             \
    }                                                    \
  } while (0)

static void test_create() {
  int sem_num = 16;
  int start_value = 1;

  IpcSemaphoreId id1 = ipc_semaphore_create(PrivateIPCKey, sem_num, IPCProtection, start_value, true);

  CU_ASSERT(id1 != -1);

  // Test invalid arguments.
  IpcSemaphoreId id2 = ipc_semaphore_create(PrivateIPCKey, 0, IPCProtection, 0, true);
  CU_ASSERT(id2 == -1);
}

static long long Gcount;
static IpcSemaphoreId GSemaId;

static void* routine() {
  for (int i = 0; i < 100000; i++) {
    ipc_semaphore_lock(GSemaId, 0, 1);

    if (i % 2 == 0) {
      Gcount += i;
    }

    ipc_semaphore_unlock(GSemaId, 0, 1);
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

void test_get_value() {
  int sem_num = 16;
  int start_value = 100;
  IpcSemaphoreId semid = ipc_semaphore_create(PrivateIPCKey, sem_num, IPCProtection, start_value, true);

  for (int i = 0; i < sem_num; i++) {
    CU_ASSERT(ipc_semaphore_get_value(semid, i) == start_value);
  }

  ipc_semaphore_unlock(semid, 0, 49);
  CU_ASSERT(ipc_semaphore_get_value(semid, 0) == 51);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  on_exit_reset();

  // Initialize the global variables.
  Gcount = 0;
  GSemaId = ipc_semaphore_create(PrivateIPCKey, 1, IPCProtection, 0, true);

  suite = CU_add_suite("Semaphore", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  ADD_TEST_MACRO(suite, "test create.", test_create);
  ADD_TEST_MACRO(suite, "test lock and unlock", test_lock_and_unlock);
  // ADD_TEST_MACRO(suite, "test get value", test_get_value);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  proc_exit(CU_get_error());
}