#include "rdbms/storage/ipc.h"

#include "../template.h"

// static void test_create() {
//   int sem_num = 16;
//   int start_value = 1;

//   IpcSemaphoreId id1 = ipc_semaphore_create(PrivateIPCKey, sem_num, IPCProtection, start_value, true);

//   CU_ASSERT(id1 != -1);

//   // Test invalid arguments.
//   IpcSemaphoreId id2 = ipc_semaphore_create(PrivateIPCKey, 0, IPCProtection, 0, true);
//   CU_ASSERT(id2 == -1);
// }

// static long long Gcount;
// static IpcSemaphoreId GSemaId;

// static void* routine() {
//   for (int i = 0; i < 100000; i++) {
//     ipc_semaphore_lock(GSemaId, 0, -1);

//     if (i % 2 == 0) {
//       Gcount += i;
//     }

//     ipc_semaphore_unlock(GSemaId, 0, -1);
//   }

//   pthread_exit(NULL);
// }

// static void test_lock_and_unlock() {
// #define NTask 4

//   pthread_t task_group[NTask];

//   for (int i = 0; i < NTask; i++) {
//     if (pthread_create(&task_group[i], NULL, routine, NULL) < 0) {
//       fprintf(stderr, "pthread_create error\n");
//       exit(i);
//     }
//   }

//   for (int i = 0; i < NTask; i++) {
//     if (pthread_join(task_group[i], NULL) < 0) {
//       fprintf(stderr, "pthread_join error\n");
//       exit(i);
//     }
//   }

//   CU_ASSERT(Gcount == 9999800000);

// #undef NTask
// }

// void test_get_value() {
//   int sem_num = 16;
//   int start_value = 100;
//   IpcSemaphoreId semid = ipc_semaphore_create(PrivateIPCKey, sem_num, IPCProtection, start_value, true);

//   for (int i = 0; i < sem_num; i++) {
//     CU_ASSERT(ipc_semaphore_get_value(semid, i) == start_value);
//   }

//   ipc_semaphore_unlock(semid, 0, 49);
//   CU_ASSERT(ipc_semaphore_get_value(semid, 0) == 51);
// }

static int GCount = 0;

static void incr() { GCount++; }

static void test_on_shmem_exit() {
  int n = 10;

  on_exit_reset();

  for (int i = 0; i < n; i++) {
    on_shmem_exit(incr, 0);
  }

  shmem_exit(0);

  CU_ASSERT(GCount == n);
}

static void register_test() { TEST("[test on shmem exit]", test_on_shmem_exit); }

MAIN("ipc")
