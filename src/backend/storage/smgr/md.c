#include <fcntl.h>

#include "rdbms/catalog/catalog.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/smgr.h"
#include "rdbms/utils/mctx.h"

// These are the assigned bits in mdfd_flags:.
#define MDFD_FREE (1 << 0)  // Unused entry.

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
    Md_fdvec[i].md_fd_flags = MDFD_FREE;
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
}
