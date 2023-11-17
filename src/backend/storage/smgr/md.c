#include <fcntl.h>

#include "rdbms/catalog/catalog.h"
#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/smgr.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/mctx.h"
#include "rdbms/utils/rel.h"

// These are the assigned bits in mdfd_flags:.
#define MD_FD_FREE (1 << 0)  // Unused entry.

// The magnetic disk storage manager keeps track of open file descriptors
// in its own descriptor pool.  This happens for two reasons.	First, at
// transaction boundaries, we walk the list of descriptors and flush
// anything that we've dirtied in the current transaction.  Second, we want
// to support relations larger than the OS' file size limit (often 2GBytes).
// In order to do that, we break relations up into chunks of < 2GBytes
// and store one chunk in each of several files that represent the relation.
// See the BLCKSZ and RELSEG_SIZE configuration constants in include/config.h.
//
// The file descriptor stored in the relation cache (see RelationGetFile())
// is actually an index into the Md_fdvec array.  -1 indicates not open.
//
// When a relation is broken into multiple chunks, only the first chunk
// has its own entry in the Md_fdvec array; the remaining chunks have
// palloc'd MdfdVec objects that are chained onto the first chunk via the
// mdfd_chain links.  All chunks except the last MUST have size exactly
// equal to RELSEG_SIZE blocks --- see mdnblocks() and mdtruncate().
typedef struct MdfdVec {
  int md_fd_vfd;        // fd number in vfd pool.
  int md_fd_flags;      // fd status flags.
  int md_fd_lst_bcnt;   // Most recent block count.
  int md_fd_next_free;  // Next free vector.

#ifndef LET_OS_MANAGE_FILESIZE
  struct MdfdVec* md_fd_chain;  // For large relations.
#endif
} MdfdVec;

static int Nfds = 100;  // Initial/current size of Md_fdvec array.
static MdfdVec* Md_fdvec = NULL;
static int MdFree = -1;      // Head of freelist of unused fdvec entries.
static int CurFd = 0;        // First never-used fdvec index.
static MemoryContext MdCxt;  // Context for all my allocations.

static void md_close_fd(int fd);
static int md_fd_get_reln_fd(Relation relation);
static MdfdVec* md_fd_open_seg(Relation relation, int seg_no, int oflags);
static MdfdVec* md_fd_get_seg(Relation relation, int blk_no);
static int md_fd_blind_get_seg(char* db_name, char* rel_name, Oid db_id, Oid rel_id, int blk_no);
static int fdvec_alloc();
static int fdvec_free(int);
static BlockNumber md_nblocks_aux(File file, Size blck_sz);

// Initialize private state for magnetic disk storage manager.
//
// We keep a private table of all file descriptors. Whenever we do
// a write to one, we mark it dirty in our table. Whenever we force
// changes to disk, we mark the file descriptor clean. At transaction
// commit, we force changes to disk for all dirty file descriptors.
// This routine allocates and initializes the table.
//
// Returns SM_SUCCESS or SM_FAIL with errno set as appropriate.
int md_init() {
  MemoryContext old_cxt;
  int i;

  MdCxt = (MemoryContext)create_global_memory("MdSmgr");

  if (MdCxt == NULL) {
    return SM_FAIL;
  }

  MEMSET(Md_fdvec, 0, Nfds * sizeof(MdfdVec));

  // Set free list.
  for (i = 0; i < Nfds; i++) {
    Md_fdvec[i].md_fd_next_free = i + 1;
    Md_fdvec[i].md_fd_flags = MD_FD_FREE;
  }

  MdFree = 0;
  Md_fdvec[Nfds - 1].md_fd_next_free = -1;

  return SM_SUCCESS;
}

int md_create(Relation relation) {
  int fd;
  int vfd;
  char* path;

  assert(relation->rd_unlinked && relation->rd_fd < 0);

  path = relpath(RELATION_GET_PHYSICAL_RELATION_NAME(relation));
  fd = file_name_open_file(path, O_RDWR | O_CREAT | O_EXCL, 0600);

  // During bootstrap processing, we skip that check, because pg_time,
  // pg_variable, and pg_log get created before their .bki file entries
  // are processed.
  //
  // For cataloged relations, pg_class is guaranteed to have an unique
  // record with the same relname by the unique index. So we are able to
  // reuse existent files for new catloged relations. Currently we reuse
  // them in the following cases. 1. they are empty. 2. they are used
  // for Index relations and their size == BLCKSZ * 2.
  if (fd < 0) {
    if (!IS_BOOTSTRAP_PROCESSING_MODE() && relation->rd_rel->relkind == RELKIND_UNCATALOGED) {
      return -1;
    }

    fd = file_name_open_file(path, O_RDWR, 0600);

    if (fd < 0) {
      return -1;
    }

    if (!IS_BOOTSTRAP_PROCESSING_MODE()) {
      bool reuse = false;
      int len = file_seek(fd, 0L, SEEK_END);

      if (len == 0) {
        reuse = true;
      } else if (relation->rd_rel->relkind == RELKIND_INDEX && len == BLCKSZ * 2) {
        reuse = true;
      }

      if (!reuse) {
        file_close(fd);

        return -1;
      }
    }
  }

  relation->rd_unlinked = false;
  vfd = fdvec_alloc();

  if (vfd < 0) {
    return -1;
  }

  Md_fdvec[vfd].md_fd_vfd = fd;
  Md_fdvec[vfd].md_fd_flags = 0;

#ifndef LET_OS_MANAGE_FILESIZE

  Md_fdvec[vfd].md_fd_chain = NULL;

#endif

  Md_fdvec[vfd].md_fd_lst_bcnt = 0;

  pfree(path);

  return vfd;
}

// Unlink a relation.
int md_unlink(Relation relation) {
  int nblocks;
  int fd;
  MdfdVec* v;
  MemoryContext old_cxt;

  // If the relation is already unlinked, we have nothing to do any more.
  if (relation->rd_unlinked && relation->rd_fd < 0) {
    return SM_SUCCESS;
  }

  // Force all segments of the relation to be opened, so that we won't
  // miss deleting any of them.
}

int md_open(Relation relation) {
  char* path;
  int fd;
  int vfd;

  assert(relation->rd_fd < 0);

  path = rel_path(RELATION_GET_PHYSICAL_RELATION_NAME(relation));
  fd = file_name_open_file(path, O_RDWR, 0600);
}

// Get the number of blocks stored in a relation.
//
// Important side effect: all segments of the relation are opened
// and added to the mdfd_chain list. If this routine has not been
// called, then only segments up to the last one actually touched
// are present in the chain...
//
// Returns # of blocks, elog's on error.
int md_nblocks(Relation relation) {
  int fd;
  MdfdVec* v;

#ifndef LET_OS_MANAGE_FILESIZE

  int nblocks;
  int segno;

#endif

  fd = md_fd_get_reln_fd(relation);
  v = &Md_fdvec[fd];

#ifndef LET_OS_MANAGE_FILESIZE

  segno = 0;

  for (;;) {
    nblocks = md_nblocks_aux(v->md_fd_vfd, BLCKSZ);

    if (nblocks > RELSEG_SIZE) {
      elog(FATAL, "%s: segment too big in mdnblocks!.\n", __func__);
    }

    v->md_fd_lst_bcnt = nblocks;

    if (nblocks == RELSEG_SIZE) {
      segno++;

      if (v->md_fd_chain == NULL) {
        v->md_fd_chain =
      }
    }
  }

#else

  return md_nblocks_aux(v->mdfd_vfd, BLCKSZ);

#endif
}

static void md_close_fd(int fd) {
  MdfdVec* v;
  MemoryContext old_cxt;

  old_cxt = memory_context_switch_to(MdCxt);
}

static int md_fd_get_reln_fd(Relation relation) {
  int fd;

  fd = RELATION_GET_FILE(relation);

  if (fd < 0) {
    if ((fd = md_open)) }
}

static int fdvec_alloc() {
  MdfdVec* nvec;
  int fdvec;
  int i;

  MemoryContext old_cxt;

  // Get from free list.
  if (MdFree >= 0) {
    fdvec = MdFree;
    MdFree = Md_fdvec[fdvec].md_fd_next_free;

    assert(Md_fdvec[fdvec].md_fd_flags == MD_FD_FREE);

    Md_fdvec[fdvec].md_fd_flags = 0;

    if (fdvec >= CurFd) {
      assert(fdvec == CurFd);
      CurFd++;
    }

    return fdvec;
  }

  // Must allocate more room.
  if (Nfds != CurFd) {
    elog(FATAL, "%s error.\n", __func__);
  }

  Nfds *= 2;
  old_cxt = memory_context_switch_to(MdCxt);

  nvec = (MdfdVec*)palloc(Nfds * sizeof(MdfdVec));
  MEMSET(nvec, 0, Nfds * sizeof(MdfdVec));
  memmove(nvec, (char*)Md_fdvec, CurFd * sizeof(MdfdVec));
  pfree(Md_fdvec);

  MemoryContextSwitchTo(old_cxt);

  Md_fdvec = nvec;

  // Set new free list.
  for (i = CurFd; i < Nfds; i++) {
    Md_fdvec[i].md_fd_next_free = i + 1;
    Md_fdvec[i].md_fd_flags = MD_FD_FREE;
  }

  Md_fdvec[Nfds - 1].md_fd_next_free = -1;
  MdFree = CurFd + 1;

  fdvec = CurFd;
  CurFd++;
  Md_fdvec[fdvec].md_fd_flags = 0;

  return fdvec;
}

// Free md file descriptor vector.
static int fdvec_free(int fdvec) {
  assert(MdFree < 0 || Md_fdvec[MdFree].md_fd_flags == MD_FD_FREE);
  assert(Md_fdvec[fdvec].md_fd_flags != MD_FD_FREE);

  Md_fdvec[fdvec].md_fd_next_free = MdFree;
  Md_fdvec[fdvec].md_fd_flags = MD_FD_FREE;
  MdFree = fdvec;
}

static BlockNumber md_nblocks_aux(File file, Size blck_sz) {
  long len;

  len = file_seek(file, 0L, SEEK_END);

  // On failure, assume file is empty.
  if (len < 0) {
    return 0;
  }

  return len / blck_sz;
}