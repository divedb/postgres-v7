#ifndef RDBMS_STORAGE_SHMEM_QUEUE_H_
#define RDBMS_STORAGE_SHMEM_QUEUE_H_

#include "rdbms/storage/shmem_def.h"

typedef struct SHM_QUEUE {
  SHMEM_OFFSET prev;
  SHMEM_OFFSET next;
} SHM_QUEUE;

void shm_queue_init(SHM_QUEUE* queue);
void shm_queue_elem_init(SHM_QUEUE* queue);
void shm_queue_delete(SHM_QUEUE* queue);

#endif  // RDBMS_STORAGE_SHMEM_QUEUE_H_
