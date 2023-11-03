// =========================================================================
//
// buf_internals.h
//  Internal definitions for buffer manager.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: buf_internals.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_BUF_INTERNALS_H_
#define RDBMS_STORAGE_BUF_INTERNALS_H_

#include "rdbms/c.h"
#include "rdbms/storage/block.h"
#include "rdbms/storage/buf.h"
#include "rdbms/storage/shmem.h"
#include "rdbms/utils/rel.h"

// Flags for buffer descriptors.
#define BM_DIRTY          (1 << 0)
#define BM_PRIVATE        (1 << 1)
#define BM_VALID          (1 << 2)
#define BM_DELETED        (1 << 3)
#define BM_FREE           (1 << 4)
#define BM_IO_IN_PROGRESS (1 << 5)
#define BM_IO_ERROR       (1 << 6)
#define BM_JUST_DIRTIED   (1 << 7)

typedef bits16 BufFlags;

// long* so alignment will be correct.
typedef long** BufferBlock;

typedef struct buftag {
  LockRelId relid;
  BlockNumber block_num;  // Block number relative to begin of relation.
} BufferTag;

#define CLEAR_BUFFERTAG(a) \
  ((a)->relid.dbid = INVALID_OID, (a)->relid.relid = INVALID_OID, (a)->block_num = INVALID_BLOCK_NUMBER)

#define INIT_BUFFERTAG(a, xx_reln, xx_block_num) \
  ((a)->block_num = (xx_block_num), (a)->relid = (xx_reln)->rd_lock_info.lock_relid)

// If we have to write a buffer "blind" (without a relcache entry),
// the BufferTag is not enough information.  BufferBlindId carries the
// additional information needed.
typedef struct bufblindid {
  char dbname[NAMEDATALEN];   // Name of db in which buf belongs.
  char relname[NAMEDATALEN];  // Name of reln.
} BufferBlindId;

#define BAD_BUFFER_ID(bid) ((bid) < 1 || (bid) > NBuffers)
#define INVALID_DESCRIPTOR (-3)

//	BufferDesc
//   shared buffer cache metadata for a single
//   shared buffer descriptor.
//
//  We keep the name of the database and relation in which this
//  buffer appears in order to avoid a catalog lookup on cache
//  flush if we don't have the reldesc in the cache.  It is also
//  possible that the relation to which this buffer belongs is
//  not visible to all backends at the time that it gets flushed.
//  Dbname, relname, dbid, and relid are enough to determine where
//  to put the buffer, for all storage managers.
typedef struct sbufdesc {
  Buffer free_next;  // Links for freelist chain.
  Buffer free_prev;
  SHMEM_OFFSET data;  // Pointer to data in buf pool.

  /* tag and id must be together for table lookup to work */
  BufferTag tag;  // File/block identifier.
  int buf_id;     // maps global desc to local desc.

  BufFlags flags;     // See bit definitions above.
  unsigned refcount;  // # of times buffer is pinned.

#ifdef HAS_TEST_AND_SET
  /* can afford a dedicated lock if test-and-set locks are available */
  slock_t io_in_progress_lock;
  slock_t cntx_lock; /* to lock access to page context */
#endif               // HAS_TEST_AND_SET

  unsigned r_locks;  // # of shared locks.
  bool ri_lock;      // Read-intent lock.
  bool w_lock;       // context exclusively locked.

  BufferBlindId blind;  // Extra info to support blind write.
} BufferDesc;

// Each backend has its own BufferLocks[] array holding flag bits
// showing what locks it has set on each buffer.
//
// We have to free these locks in elog(ERROR)...
#define BL_IO_IN_PROGRESS (1 << 0) /* unimplemented */
#define BL_R_LOCK         (1 << 1)
#define BL_RI_LOCK        (1 << 2)
#define BL_W_LOCK         (1 << 3)

// Mao tracing buffer allocation.
#ifdef BMTRACE

typedef struct _bmtrace {
  int bmt_pid;
  long bmt_buf;
  long bmt_dbid;
  long bmt_relid;
  int bmt_blkno;
  int bmt_op;

#define BMT_NOTUSED     0
#define BMT_ALLOCFND    1
#define BMT_ALLOCNOTFND 2
#define BMT_DEALLOC     3

} bmtrace;

#endif  // BMTRACE

// freelist.c
extern int FreeListDescriptor;
extern BufferDesc* BufferDescriptors;
extern long* PrivateRefCount;

void add_buffer_to_freelist(BufferDesc* buf_desc);
void pin_buffer(BufferDesc* buf_desc);
void pin_buffer_debug(char* file, int line, BufferDesc* buf_desc);
void unpin_buffer(BufferDesc* buf_desc);
BufferDesc* get_free_buffer(void);
void init_freelist(bool init);

// buf_table.c
void init_buf_table();
BufferDesc* buf_table_lookup(BufferTag* tag_ptr);
bool buf_table_delete(BufferDesc* buf_desc);
bool buf_table_insert(BufferDesc* buf_desc);

// localbuf.c
extern long* LocalRefCount;
extern BufferDesc* LocalBufferDescriptors;
extern int NLocBuffer;

Buffer* local_buffer_alloc(Relation relation, BlockNumber block_num, bool* found_ptr);
int write_local_buffer(Buffer buffer, bool release);
int flush_local_buffer(Buffer buffer, bool release);
void init_local_buffer(void);
void local_buffer_sync();
void reset_local_buffer_pool();

#endif  // RDBMS_STORAGE_BUF_INTERNALS_H_
