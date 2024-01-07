//===----------------------------------------------------------------------===//
//
// md.c
// This code manages relations that reside on magnetic disk.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header:
//  /home/projects/pgsql/cvsroot/pgsql/src/backend/storage/smgr/md.c
//  v 1.83 2001/04/02 23:20:24 tgl Exp $
//
//===----------------------------------------------------------------------===//
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "rdbms/catalog/catalog.h"
#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/smgr.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"
#include "rdbms/utils/rel.h"

#undef DIAGNOSTIC

// These are the assigned bits in mdfd_flags:.
#define MD_FD_FREE (1 << 0)  // Unused entry.

// The magnetic disk storage manager keeps track of open file descriptors
// in its own descriptor pool. This happens for two reasons. First, at
// transaction boundaries, we walk the list of descriptors and flush
// anything that we've dirtied in the current transaction. Second, we want
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
// mdfd_chain links. All chunks except the last MUST have size exactly
// equal to RELSEG_SIZE blocks --- see mdnblocks() and mdtruncate().
typedef struct MdfdVec {
  int md_fd_vfd;        // fd number in vfd pool
  int md_fd_flags;      // fd status flags
  int md_fd_lst_bcnt;   // Most recent block count
  int md_fd_next_free;  // Next free vector

#ifndef LET_OS_MANAGE_FILESIZE
  struct MdfdVec* md_fd_chain;  // For large relations
#endif
} MdfdVec;

static int Nfds = 100;  // Initial/current size of Md_fdvec array
static MdfdVec* Md_fdvec = NULL;
static int MdFree = -1;      // Head of freelist of unused fdvec entries
static int CurFd = 0;        // First never-used fdvec index
static MemoryContext MdCxt;  // Context for all my allocations

static void md_close_fd(int fd);
static int md_fd_get_reln_fd(Relation relation);
static MdfdVec* md_fd_open_seg(Relation relation, int seg_no, int oflags);
static MdfdVec* md_fd_get_seg(Relation relation, int blk_no);
static int md_fd_blind_get_seg(RelFileNode rnode, int blk_no);
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
  int i;

  MdCxt = alloc_set_context_create(
      TopMemoryContext, "MdSmgr", ALLOCSET_DEFAULT_MIN_SIZE,
      ALLOCSET_DEFAULT_INIT_SIZE, ALLOCSET_DEFAULT_MAX_SIZE);
  Md_fdvec = (MdfdVec*)memory_context_alloc(MdCxt, Nfds * sizeof(MdfdVec));
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
  char* path;
  int fd;
  int vfd;

  ASSERT(relation->rd_fd < 0);

  path = relpath(relation->rd_node);
  fd = file_name_open_file(path, O_RDWR | O_CREAT | O_EXCL | PG_BINARY, 0600);

  if (fd < 0) {
    int save_errno = errno;

    // During bootstrap, there are cases where a system relation will
    // be accessed (by internal backend processes) before the
    // bootstrap script nominally creates it. Therefore, allow the
    // file to exist already, but in bootstrap mode only. (See also
    // mdopen)
    if (IS_BOOTSTRAP_PROCESSING_MODE()) {
      fd = file_name_open_file(path, O_RDWR | PG_BINARY, 0600);
    }

    if (fd < 0) {
      pfree(path);
      // Be sure to return the error reported by create, not open.
      errno = save_errno;
      return -1;
    }

    errno = 0;
  }

  pfree(path);
  vfd = fdvec_alloc();

  if (vfd < 0) {
    return -1;
  }

  Md_fdvec[vfd].md_fd_vfd = fd;
  Md_fdvec[vfd].md_fd_flags = 0;
  Md_fdvec[vfd].md_fd_lst_bcnt = 0;

#ifndef LET_OS_MANAGE_FILESIZE
  Md_fdvec[vfd].md_fd_chain = NULL;
#endif

  return vfd;
}

// Unlink a relation.
int md_unlink(RelFileNode rnode) {
  int status = SM_SUCCESS;
  int save_errno = 0;
  char* path;

  path = relpath(rnode);

  // Delete the first segment, or only segment if not doing segmenting.
  if (unlink(path) < 0) {
    status = SM_FAIL;
    save_errno = errno;
  }

#ifndef LET_OS_MANAGE_FILESIZE

  if (status == SM_SUCCESS) {
    char* segpath = (char*)palloc(strlen(path) + 12);
    int segno;

    for (segno = 1;; segno++) {
      sprintf(segpath, "%s.%d", path, segno);

      if (unlink(segpath) < 0) {
        // ENOENT is expected after the last segment...
        if (errno != ENOENT) {
          status = SM_FAIL;
          save_errno = errno;
        }
        break;
      }
    }

    pfree(segpath);
  }

#endif

  pfree(path);
  errno = save_errno;

  return status;
}

// Add a block to the specified relation.
//
// This routine returns SM_FAIL or SM_SUCCESS, with errno set as
// appropriate.
int md_extend(Relation relation, char* buffer) {
  long pos;
  long nbytes;
  int nblocks;
  MdfdVec* v;

  nblocks = md_nblocks(relation);
  v = md_fd_get_seg(relation, nblocks);

  if ((pos = file_seek(v->md_fd_vfd, 0L, SEEK_END)) < 0) {
    return SM_FAIL;
  }

  // The last block is incomplete.
  if (pos % BLCKSZ != 0) {
    pos -= pos % BLCKSZ;

    if (file_seek(v->md_fd_vfd, pos, SEEK_SET) < 0) {
      return SM_FAIL;
    }
  }

  // TODO(gc): 这里为什么没有考虑缓冲区溢出？
  if ((nbytes = file_write(v->md_fd_vfd, buffer, BLCKSZ)) != BLCKSZ) {
    if (nbytes > 0) {
      file_truncate(v->md_fd_vfd, pos);
      file_seek(v->md_fd_vfd, pos, SEEK_SET);
    }

    return SM_FAIL;
  }

// Try to keep the last block count current, though it's just a hint.
#ifndef LET_OS_MANAGE_FILESIZE

  if ((v->md_fd_lst_bcnt = (++nblocks % RELSEG_SIZE)) == 0) {
    v->md_fd_lst_bcnt = RELSEG_SIZE;
  }

#ifdef DIAGNOSTIC

  if (md_nblocks_aux(v->md_fd_vfd, BLCKSZ) > RELSEG_SIZE ||
      v->md_fd_lst_bcnt > RELSEG_SIZE) {
    elog(FATAL, "segment too big!");
  }

#endif

#else

  v->md_fd_lst_bcnt = ++nblocks;

#endif

  return SM_SUCCESS;
}

int md_open(Relation relation) {
  char* path;
  int fd;
  int vfd;

  // 确认这张表还没打开
  ASSERT(relation->rd_fd < 0);

  // 找到这张表路径
  path = rel_path(relation->rd_node);
  fd = file_name_open_file(path, O_RDWR, 0600);

  if (fd < 0) {
    // In bootstrap mode, accept mdopen as substitute for mdcreate.
    if (IS_BOOTSTRAP_PROCESSING_MODE()) {
      fd = file_name_open_file(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    }

    if (fd < 0) {
      elog(NOTICE, "%s: couldn't open %s", __func__, path);
      pfree(path);

      return -1;
    }
  }

  pfree(path);
  vfd = fdvec_alloc();

  if (vfd < 0) {
    return -1;
  }

  Md_fdvec[vfd].md_fd_vfd = fd;
  Md_fdvec[vfd].md_fd_flags = 0;
  Md_fdvec[vfd].md_fd_lst_bcnt = md_nblocks_aux(fd, BLCKSZ);

#ifndef LET_OS_MANAGE_FILESIZE

  Md_fdvec[vfd].md_fd_chain = NULL;

#ifdef DIAGNOSTIC

  if (Md_fdvec[vfd].md_fd_lst_bcnt > RELSEG_SIZE) {
    elog(FATAL, "segment too big on relopen!");
  }

#endif

#endif

  return vfd;
}

// Close the specified relation, if it isn't closed already.
//
// AND FREE fd vector! It may be re-used for other relation!
// reln should be flushed from cache after closing !..
//
// Returns SM_SUCCESS or SM_FAIL with errno set as appropriate.
int md_close(Relation relation) {
  int fd;

  fd = RELATION_GET_FILE(relation);

  // Already closed, so no work.
  if (fd < 0) {
    return SM_SUCCESS;
  }

  md_close_fd(fd);
  relation->rd_fd = -1;

  return SM_SUCCESS;
}

int md_read(Relation relation, BlockNumber block_num, char* buffer) {
  int status;
  long seek_pos;
  int nbytes;
  MdfdVec* v;

  v = md_fd_get_seg(relation, block_num);

#ifndef LET_OS_MANAGE_FILESIZE

  seek_pos = (long)(BLCKSZ * (block_num % RELSEG_SIZE));

#ifdef DIAGNOSTIC
  if (seek_pos >= BLCKSZ * RELSEG_SIZE) {
    elog(FATAL, "seekpos too big!");
  }
#endif

#else
  seekpos = (long)(BLCKSZ * (blocknum));
#endif

  if (file_seek(v->md_fd_vfd, seek_pos, SEEK_SET) != seek_pos) {
    return SM_FAIL;
  }

  status = SM_SUCCESS;

  if ((nbytes = file_read(v->md_fd_vfd, buffer, BLCKSZ)) != BLCKSZ) {
    if (nbytes == 0) {
      MEMSET(buffer, 0, BLCKSZ);
    } else if (block_num == 0 && nbytes > 0 && md_nblocks(relation) == 0) {
      // TODO(gc): 为什么block的数量为0，能读到nbytes大于0？
      MEMSET(buffer, 0, BLCKSZ);
    } else {
      status = SM_FAIL;
    }
  }

  return status;
}

// Write the supplied block at the appropriate location.
// Returns SM_SUCCESS or SM_FAIL.
int md_write(Relation relation, BlockNumber block_num, char* buffer) {
  int status;
  long seek_pos;
  MdfdVec* v;

  v = md_fd_get_seg(relation, block_num);

#ifndef LET_OS_MANAGE_FILESIZE、
  seek_pos = (long)(BLCKSZ * (block_num % RELSEG_SIZE));

#ifdef DIAGNOSTIC
  if (seek_pos >= BLCKSZ * RELSEG_SIZE) {
    elog(FATAL, "seekpos too big!");
  }
#endif

#else
  seek_pos = (long)(BLCKSZ * (block_num));
#endif

  if (file_seek(v->md_fd_vfd, seek_pos, SEEK_SET) != seek_pos) {
    return SM_FAIL;
  }

  status = SM_SUCCESS;

  if (file_write(v->md_fd_vfd, buffer, BLCKSZ) != BLCKSZ) {
    status = SM_FAIL;
  }

  return status;
}

// Synchronously write a block to disk.
//
// This is exactly like mdwrite(), but doesn't return until the file
// system buffer cache has been flushed.
int md_flush(Relation relation, BlockNumber block_num, char* buffer) {
  int status;
  long seek_pos;
  MdfdVec* v;

  v = md_fd_get_seg(relation, block_num);

#ifndef LET_OS_MANAGE_FILESIZE
  seek_pos = (long)(BLCKSZ * (block_num % RELSEG_SIZE));

#ifdef DIAGNOSTIC
  if (seek_pos >= BLCKSZ * RELSEG_SIZE) {
    elog(FATAL, "seekpos too big!");
  }
#endif

#else
  seek_pos = (long)(BLCKSZ * (blocknum));
#endif

  if (file_seek(v->md_fd_vfd, seek_pos, SEEK_SET) != seek_pos) {
    return SM_FAIL;
  }

  // Write and sync the block.
  status = SM_SUCCESS;

  if (file_write(v->md_fd_vfd, buffer, BLCKSZ) != BLCKSZ ||
      file_sync(v->md_fd_vfd) < 0) {
    status = SM_FAIL;
  }

  return status;
}

// Write a block to disk blind.
//
// We have to be able to do this using only the name and OID of
// the database and relation in which the block belongs.  Otherwise
// this is much like mdwrite().  If dofsync is TRUE, then we fsync
// the file, making it more like mdflush().
int md_blind_wrt(RelFileNode rnode, BlockNumber block_num, char* buffer,
                 bool do_fsync) {
  int status;
  long seek_pos;
  int fd;

  fd = md_fd_blind_get_seg(rnode, block_num);

  if (fd < 0) {
    return SM_FAIL;
  }

#ifndef LET_OS_MANAGE_FILESIZE
  seek_pos = (long)(BLCKSZ * (block_num % RELSEG_SIZE));

#ifdef DIAGNOSTIC
  if (seek_pos >= BLCKSZ * RELSEG_SIZE) {
    elog(FATAL, "seekpos too big!");
  }
#endif

#else
  seek_pos = (long)(BLCKSZ * (block_num));
#endif

  errno = 0;

  if (lseek(fd, seek_pos, SEEK_SET) != seek_pos) {
    elog(DEBUG, "%s: lseek(%ld) failed: %m", __func__, seek_pos);
    close(fd);

    return SM_FAIL;
  }

  status = SM_SUCCESS;

  // Write and optionally sync the block.
  if (write(fd, buffer, BLCKSZ) != BLCKSZ) {
    elog(DEBUG, "%s: write() failed: %m", __func__);
    status = SM_FAIL;
  }

  if (close(fd) < 0) {
    elog(DEBUG, "%s: close() failed: %m", __func__);
    status = SM_FAIL;
  }

  return status;
}

// Mark the specified block "dirty" (ie, needs fsync).
//
// Returns SM_SUCCESS or SM_FAIL.
int md_mark_dirty(Relation relation, BlockNumber block_num) {
  MdfdVec* v;

  v = md_fd_get_seg(relation, block_num);

  file_mark_dirty(v->md_fd_vfd);

  return SM_SUCCESS;
}

int md_blind_mark_dirty(RelFileNode rnode, BlockNumber block_num) {
  int status;
  int fd;

  fd = md_fd_blind_get_seg(rnode, block_num);

  if (fd < 0) {
    return SM_FAIL;
  }

  status = SM_SUCCESS;

  if (pg_fsync(fd) < 0) {
    status = SM_FAIL;
  }

  if (close(fd) < 0) {
    status = SM_FAIL;
  }

  return status;
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
  int seg_no;
#endif

  fd = md_fd_get_reln_fd(relation);
  v = &Md_fdvec[fd];

#ifndef LET_OS_MANAGE_FILESIZE
  seg_no = 0;

  for (;;) {
    nblocks = md_nblocks_aux(v->md_fd_vfd, BLCKSZ);

    if (nblocks > RELSEG_SIZE) {
      elog(FATAL, "%s: segment too big in mdnblocks!.\n", __func__);
    }

    v->md_fd_lst_bcnt = nblocks;

    if (nblocks == RELSEG_SIZE) {
      seg_no++;

      if (v->md_fd_chain == NULL) {
        v->md_fd_chain = md_fd_open_seg(relation, seg_no, O_CREAT);

        if (v->md_fd_chain == NULL) {
          elog(ERROR, "%s: cannot count blocks for %s -- open failed", __func__,
               RELATION_GET_RELATION_NAME(relation));
        }
      }

      v = v->md_fd_chain;
    } else {
      return (seg_no * RELSEG_SIZE) + nblocks;
    }
  }

#else
  return md_nblocks_aux(v->mdfd_vfd, BLCKSZ);
#endif
}

// Truncate relation to specified number of blocks.
//
// Returns # of blocks or -1 on error.
int md_truncate(Relation relation, int nblocks) {
  int cur_nblk;
  int fd;
  MdfdVec* v;

#ifndef LET_OS_MANAGE_FILESIZE
  int prior_blocks;
#endif

  // NOTE: mdnblocks makes sure we have opened all existing segments, so
  // that truncate/delete loop will get them all!
  cur_nblk = md_nblocks(relation);

  // bogus request.
  if (nblocks < 0 || nblocks > cur_nblk) {
    return -1;
  }

  // No work.
  if (nblocks == cur_nblk) {
    return nblocks;
  }

  fd = md_fd_get_reln_fd(relation);
  v = &Md_fdvec[fd];

#ifndef LET_OS_MANAGE_FILESIZE
  prior_blocks = 0;

  while (v != NULL) {
    MdfdVec* ov = v;

    if (prior_blocks > nblocks) {
      // This segment is no longer wanted at all (and has already
      // been unlinked from the mdfd_chain). We truncate the file
      // before deleting it because if other backends are holding
      // the file open, the unlink will fail on some platforms.
      // Better a zero-size file gets left around than a big file...
      file_truncate(v->md_fd_vfd, 0);
      file_unlink(v->md_fd_vfd);
      v = v->md_fd_chain;
      ASSERT(ov != &Md_fdvec[fd]);  // we never drop the 1st segment.
      pfree(ov);
    } else if (prior_blocks + RELSEG_SIZE > nblocks) {
      // This is the last segment we want to keep. Truncate the file
      // to the right length, and clear chain link that points to
      // any remaining segments (which we shall zap). NOTE: if
      // nblocks is exactly a multiple K of RELSEG_SIZE, we will
      // truncate the K+1st segment to 0 length but keep it. This is
      // mainly so that the right thing happens if nblocks=0.
      int lastsegblocks = nblocks - prior_blocks;

      if (file_truncate(v->md_fd_vfd, lastsegblocks * BLCKSZ) < 0) {
        return -1;
      }

      v->md_fd_lst_bcnt = lastsegblocks;
      v = v->md_fd_chain;
      ov->md_fd_chain = NULL;
    } else {
      // We still need this segment and 0 or more blocks beyond it,
      // so nothing to do here.
      v = v->md_fd_chain;
    }

    prior_blocks += RELSEG_SIZE;
  }

#else
  if (file_truncate(v->md_fd_vfd, nblocks * BLCKSZ) < 0) {
    return -1;
  }

  v->mdfd_lst_bcnt = nblocks;

#endif

  return nblocks;
}

// Commit a transaction.
//
// All changes to magnetic disk relations must be forced to stable
// storage.  This routine makes a pass over the private table of
// file descriptors.  Any descriptors to which we have done writes,
// but not synced, are synced here.
//
// Returns SM_SUCCESS or SM_FAIL with errno set as appropriate.
int md_commit() {
  int i;
  MdfdVec* v;

  for (i = 0; i < CurFd; i++) {
    v = &Md_fdvec[i];
    if (v->md_fd_flags & MD_FD_FREE) {
      continue;
    }

    // Sync the file entry.
#ifndef LET_OS_MANAGE_FILESIZE
    for (; v != (MdfdVec*)NULL; v = v->md_fd_chain)
#else
    if (v != (MdfdVec*)NULL)
#endif
    {
      if (file_sync(v->md_fd_vfd) < 0) {
        return SM_FAIL;
      }
    }
  }

  return SM_SUCCESS;
}

// Abort a transaction.
//
// Changes need not be forced to disk at transaction abort.  We mark
// all file descriptors as clean here.  Always returns SM_SUCCESS.
int md_abort() {
  // We don't actually have to do anything here. fd.c will discard
  // fsync-needed bits in its AtEOXact_Files() routine.
  return SM_SUCCESS;
}

static void md_close_fd(int fd) {
  MdfdVec* v;

#ifndef LET_OS_MANAGE_FILESIZE

  for (v = &Md_fdvec[fd]; v != NULL;) {
    MdfdVec* ov = v;

    // If not closed already.
    if (v->md_fd_vfd >= 0) {
      // We sync the file descriptor so that we don't need to reopen
      // it at transaction commit to force changes to disk. (This
      // is not really optional, because we are about to forget that
      // the file even exists...).
      file_sync(v->md_fd_vfd);
      file_close(v->md_fd_vfd);
    }

    // Now free vector.
    v = v->md_fd_chain;

    if (ov != &Md_fdvec[fd]) {
      pfree(ov);
    }
  }

  Md_fdvec[fd].md_fd_chain = NULL;

#else

  v = &Md_fdvec[fd];

  if (v != NULL) {
    if (v->md_fd_vfd >= 0) {
      file_sync(v->md_fd_vfd);
      file_close(v->md_fd_vfd);
    }
  }

#endif

  fdvec_free(fd);
}

// Get the fd for the relation, opening it if it's not already open.
static int md_fd_get_reln_fd(Relation relation) {
  int fd;

  fd = RELATION_GET_FILE(relation);

  if (fd < 0) {
    if ((fd = md_open(relation)) < 0) {
      elog(ERROR, "%s cannot open relation %s", __func__,
           RELATION_GET_RELATION_NAME(relation));
    }

    relation->rd_fd = fd;
  }

  return fd;
}

static MdfdVec* md_fd_open_seg(Relation relation, int seg_no, int oflags) {
  MdfdVec* v;
  int fd;
  char* path;
  char* full_path;

  // Be sure we have enough space for the '.segno', if any.
  path = relpath(relation->rd_node);

  if (seg_no > 0) {
    full_path = (char*)palloc(strlen(path) + 12);
    sprintf(full_path, "%s.%d", path, seg_no);
    pfree(path);
  } else {
    full_path = path;
  }

  // Open the file.
  fd = file_name_open_file(full_path, O_RDWR | PG_BINARY | oflags, 0600);
  pfree(full_path);

  if (fd < 0) {
    return NULL;
  }

  // Allocate an mdfdvec entry for it.
  v = (MdfdVec*)memory_context_alloc(MdCxt, sizeof(MdfdVec));

  // Fill the entry.
  v->md_fd_vfd = fd;
  v->md_fd_flags = 0;
  v->md_fd_lst_bcnt = md_nblocks_aux(fd, BLCKSZ);

#ifndef LET_OS_MANAGE_FILESIZE

  v->md_fd_chain = NULL;

#ifdef DIAGNOSTIC
  if (v->mdfd_lstbcnt > RELSEG_SIZE) elog(FATAL, "segment too big on open!");
#endif

#endif

  return v;
}

// Find the segment of the relation holding the specified block.
// 假如一个segment的大小是1G，一个block的大小是8192字节
// 那么一个segment可以容纳1G/8192=131072个block
static MdfdVec* md_fd_get_seg(Relation relation, int block_num) {
  MdfdVec* v;
  int seg_no;
  int fd;
  int i;

  fd = md_fd_get_reln_fd(relation);

#ifndef LET_OS_MANAGE_FILESIZE

  for (v = &Md_fdvec[fd], seg_no = block_num / RELSEG_SIZE, i = 1; seg_no > 0;
       i++, seg_no--) {
    if (v->md_fd_chain == NULL) {
      // We will create the next segment only if the target block is
      // within it. This prevents Sorcerer's Apprentice syndrome if
      // a bug at higher levels causes us to be handed a
      // ridiculously large blkno --- otherwise we could create many
      // thousands of empty segment files before reaching the
      // "target" block. We should never need to create more than
      // one new segment per call, so this restriction seems
      // reasonable.
      v->md_fd_chain = md_fd_open_seg(relation, i, (seg_no == 1) ? O_CREAT : 0);

      if (v->md_fd_chain == NULL) {
        elog(ERROR, "%s: cannot open segment %d of relation %s", __func__, i,
             RELATION_GET_RELATION_NAME(relation));
      }
    }

    v = v->md_fd_chain;
  }

#else
  v = &Md_fdvec[fd];
#endif

  return v;
}

static int md_fd_blind_get_seg(RelFileNode rnode, int block_num) {
  char* path;
  int fd;

#ifndef LET_OS_MANAGE_FILESIZE
  int segno;
#endif

  path = relpath(rnode);

#ifndef LET_OS_MANAGE_FILESIZE
  // Append the '.segno', if needed.
  segno = block_num / RELSEG_SIZE;
  if (segno > 0) {
    char* segpath = (char*)palloc(strlen(path) + 12);

    sprintf(segpath, "%s.%d", path, segno);
    pfree(path);
    path = segpath;
  }
#endif

  // Call fd.c to allow other FDs to be closed if needed.
  fd = basic_open_file(path, O_RDWR | PG_BINARY, 0600);
  if (fd < 0) {
    elog(DEBUG, "%s: couldn't open %s: %m", __func__, path);
  }

  pfree(path);

  return fd;
}

static int fdvec_alloc() {
  MdfdVec* nvec;
  int fdvec;
  int i;

  // Get from free list.
  if (MdFree >= 0) {
    fdvec = MdFree;
    MdFree = Md_fdvec[fdvec].md_fd_next_free;

    ASSERT(Md_fdvec[fdvec].md_fd_flags == MD_FD_FREE);

    Md_fdvec[fdvec].md_fd_flags = 0;

    if (fdvec >= CurFd) {
      ASSERT(fdvec == CurFd);
      CurFd++;
    }

    return fdvec;
  }

  // Must allocate more room.
  if (Nfds != CurFd) {
    elog(FATAL, "%s error.\n", __func__);
  }

  Nfds *= 2;
  nvec = memory_context_alloc(MdCxt, Nfds * sizeof(MdfdVec));
  MEMSET(nvec, 0, Nfds * sizeof(MdfdVec));
  memmove(nvec, (char*)Md_fdvec, CurFd * sizeof(MdfdVec));
  pfree(Md_fdvec);

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
  ASSERT(MdFree < 0 || Md_fdvec[MdFree].md_fd_flags == MD_FD_FREE);
  ASSERT(Md_fdvec[fdvec].md_fd_flags != MD_FD_FREE);

  Md_fdvec[fdvec].md_fd_next_free = MdFree;
  Md_fdvec[fdvec].md_fd_flags = MD_FD_FREE;
  MdFree = fdvec;
}

// 一个文件有几个blocks
static BlockNumber md_nblocks_aux(File file, Size blck_sz) {
  long len;

  len = file_seek(file, 0L, SEEK_END);

  // On failure, assume file is empty.
  if (len < 0) {
    return 0;
  }

  return len / blck_sz;
}