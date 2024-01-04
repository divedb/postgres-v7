//===----------------------------------------------------------------------===//
//
// fd.c
//  Virtual file descriptor code.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// IDENTIFICATION
//  $Header:
/// home/projects/pgsql/cvsroot/pgsql/src/backend/storage/file/fd.c
//  v 1.76 2001/04/03 04:07:02 tgl Exp $
//
//  NOTES:
//
//  This code manages a cache of 'virtual' file descriptors (VFDs).
//  The server opens many file descriptors for a variety of reasons,
//  including base tables, scratch files (e.g., sort and hash spool
//  files), and random calls to C library routines like system(3); it
//  is quite easy to exceed system limits on the number of open files a
//  single process can have.  (This is around 256 on many modern
//  operating systems, but can be as low as 32 on others.)
//
//  VFDs are managed as an LRU pool, with actual OS file descriptors
//  being opened and closed as needed.  Obviously, if a routine is
//  opened using these interfaces, all subsequent operations must also
//  be through these interfaces (the File type is not a real file
//  descriptor).
//
//  For this scheme to work, most (if not all) routines throughout the
//  server should use these interfaces instead of calling the C library
//  routines (e.g., open(2) and fopen(3)) themselves.  Otherwise, we
//  may find ourselves short of real file descriptors anyway.
//
//  This file used to contain a bunch of stuff to support RAID levels 0
//  (jbod), 1 (duplex) and 5 (xor parity).  That stuff is all gone
//  because the parallel query processing code that called it is all
//  gone.  If you really need it you could get it from the original
//  POSTGRES source.
//===----------------------------------------------------------------------===//

#include "rdbms/storage/fd.h"

#include <fcntl.h>
#include <sys/param.h>
#include <unistd.h>

#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/ipc.h"
#include "rdbms/utils/elog.h"

// Problem: Postgres does a system(ld...) to do dynamic loading.
// This will open several extra files in addition to those used by
// Postgres. We need to guarantee that there are file descriptors free
// for ld to use.
//
// The current solution is to limit the number of file descriptors
// that this code will allocate at one time: it leaves RESERVE_FOR_LD free.
//
// (Even though most dynamic loaders now use dlopen(3) or the
// equivalent, the OS must still open several files to perform the
// dynamic loading. Keep this here.)
#ifndef RESERVE_FOR_LD
#define RESERVE_FOR_LD 10
#endif

// We need to ensure that we have at least some file descriptors
// available to postgreSQL after we've reserved the ones for LD,
// so we set that value here.
//
// I think 10 is an appropriate value so that's what it'll be
// for now.
#ifndef FD_MINFREE
#define FD_MINFREE 10
#endif

#ifdef FDDEBUG
#define DO_DB(a) a
#else
#define DO_DB(a)
#endif

#define VFD_CLOSED (-1)
#define FILE_IS_VALID(file) \
  ((file) > 0 && (file) < (int)SizeVfdCache && VfdCache[file].filename != NULL)
#define FILE_IS_NOT_OPEN(file) (VfdCache[file].fd == VFD_CLOSED)
#define FILE_UNKNOWN_POS       (-1)

typedef struct Vfd {
  signed short fd;         // Current FD, or VFD_CLOSED if none
  unsigned short fdstate;  // Bitflags for VFD's state

// These are the assigned bits in fdstate.
#define FD_DIRTY     (1 << 0)  // Written to, but not yet fsync'd
#define FD_TEMPORARY (1 << 1)  // Should be unlinked when closed

  File next_free;          // Link to next free VFD, if in freelist
  File lru_more_recently;  // Doubly linked recency-of-use list
  File lru_less_recently;
  long seek_pos;   // Current logical file position
  char* filename;  // Name of file, or NULL for unused VFD
  int file_flags;  // open(2) flags for opening the file
  int file_mode;   // Mode to pass to open(2)
} Vfd;

// Virtual File Descriptor array pointer and size. This grows as
// needed. 'File' values are indexes into this array.
// Note that VfdCache[0] is not a usable VFD, just a list header.
static Vfd* VfdCache;
static Size SizeVfdCache = 0;

// Number of file descriptors known to be in use by VFD entries.
static int NFile = 0;

// List of stdio FILEs opened with AllocateFile.
//
// Since we don't want to encourage heavy use of AllocateFile, it seems
// OK to put a pretty small maximum limit on the number of simultaneously
// allocated files.
#define MAX_ALLOCATED_FILES 32

static int NumAllocatedFiles = 0;
static FILE* AllocatedFiles[MAX_ALLOCATED_FILES];

// Number of temporary files opened during the current transaction;
// this is used in generation of tempfile names.
static long TempFileCounter = 0;

// Private Routines
//
// Delete           - delete a file from the Lru ring
// LruDelete        - remove a file from the Lru ring and close its FD
// Insert           - put a file at the front of the Lru ring
// LruInsert        - put a file at the front of the Lru ring and open it
// ReleaseLruFile   - Release an fd by closing the last entry in the Lru ring
// AllocateVfd      - grab a free (or new) file record (from VfdArray)
// FreeVfd          - free a file record
//
// The Least Recently Used ring is a doubly linked list that begins and
// ends on element zero. Element zero is special -- it doesn't represent
// a file and its "fd" field always == VFD_CLOSED. Element zero is just an
// anchor that shows us the beginning/end of the ring.
// Only VFD elements that are currently really open (have an FD assigned) are
// in the Lru ring. Elements that are "virtually" open can be recognized
// by having a non-null fileName field.
//
// example:
//
//     /--less----\                /---------\
//     v           \              v           \
//   #0 --more---> LeastRecentlyUsed --more-\ \
//   ^\                                     | |
//    \\less--> MostRecentlyUsedFile    <---/ |
//     \more---/                     \--less--/
//
static void delete (File file);
static void lru_delete(File file);
static void insert(File file);
static int lru_insert(File file);
static bool release_lru_file();
static File allocate_vfd();
static void free_vfd(File file);

static int file_access(File file);
static File file_name_open_file_aux(FileName filename, int file_flags,
                                    int file_mode);
static char* filepath(char* filename);
static long pg_nofile(void);
static void dump_lru();

File file_name_open_file(FileName filename, int file_flags, int file_mode) {
  File fd;
  char* fname;

  fname = filepath(filename);
  fd = file_name_open_file_aux(fname, file_flags, file_mode);
  pfree(fname);

  return fd;
}

// Open a file in the database directory ($PGDATA/base/...).
File path_name_open_file(FileName filename, int file_flags, int file_mode) {
  return file_name_open_file(filename, file_flags, file_mode);
}

// Open a temporary file that will disappear when we close it.
//
// This routine takes care of generating an appropriate tempfile name.
// There's no need to pass in fileFlags or fileMode either, since only
// one setting makes any sense for a temp file.
File open_temporary_file(void) {
  char temp_filename[64];
  File file;

  // Generate a tempfile name that's unique within the current transaction.
  snprintf(temp_filename, sizeof(temp_filename), "pg_sorttemp%d.%ld", MyProcPid,
           TempFileCounter++);
  file = file_name_open_file(temp_filename,
                             O_RDWR | O_CREAT | O_TRUNC | PG_BINARY, 0600);

  if (file <= 0) {
    elog(ERROR, "%s: failed to create temporary file %s", __func__,
         temp_filename);
  }

  // Mark it for deletion at close or EOXact.
  VfdCache[file].fdstate |= FD_TEMPORARY;

  return file;
}

// Close a file when done with it.
void file_close(File file) {
  int return_value;

  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s %d (%s).\n", __func__, file, VfdCache[file].filename));

  // TODO(gc): 这个地方和lru_delete高度重复
  if (!FILE_IS_NOT_OPEN(file)) {
    // Remove the file from the lru ring.
    delete (file);

    // If we did any writes, sync the file before closing.
    if (VfdCache[file].fdstate & FD_DIRTY) {
      return_value = pg_fsync(VfdCache[file].fd);

      ASSERT(return_value != -1);

      VfdCache[file].fdstate &= ~FD_DIRTY;
    }

    // Close the file.
    return_value = close(VfdCache[file].fd);

    ASSERT(return_value != -1);

    --NFile;
    VfdCache[file].fd = VFD_CLOSED;
  }

  // Delete the file if it was temporary.
  if (VfdCache[file].fdstate & FD_TEMPORARY) {
    VfdCache[file].fdstate &= ~FD_TEMPORARY;
    unlink(VfdCache[file].filename);
  }

  // Return the Vfd slot to the free list.
  free_vfd(file);
}

// Close a file and forcibly delete the underlying Unix file.
void file_unlink(File file) {
  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: %d (%s).\n", __func__, file, VfdCache[file].filename));

  // Force FileClose to delete it.
  VfdCache[file].fdstate |= FD_TEMPORARY;
  file_close(file);
}

int file_read(File file, char* buffer, int amount) {
  int return_code;

  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: %d (%s) %d %p.\n", __func__, file,
             VfdCache[file].filename, amount, buffer));

  file_access(file);
  return_code = read(VfdCache[file].fd, buffer, amount);

  if (return_code > 0) {
    VfdCache[file].seek_pos += return_code;
  } else {
    VfdCache[file].seek_pos = FILE_UNKNOWN_POS;
  }

  return return_code;
}

int file_write(File file, char* buffer, int amount) {
  int return_code;

  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: (%d) (%s) %d %p.\n", __func__, file,
             VfdCache[file].filename, amount, buffer));

  file_access(file);
  return_code = write(VfdCache[file].fd, buffer, amount);

  if (return_code > 0) {
    VfdCache[file].seek_pos += return_code;
  } else {
    VfdCache[file].seek_pos = FILE_UNKNOWN_POS;
  }

  // Mark the file as needing fsync.
  // 文件创建的时候已经mark为FD_DIRTY
  // VfdCache[file].fdstate |= FD_DIRTY;

  return return_code;
}

// 1. If whence is SEEK_SET, the offset is set to offset bytes.
// 2. If whence is SEEK_CUR, the offset is set to its current location plus
//    offset bytes.
// 3. If whence is SEEK_END, the offset is set to the size of the file plus
//    offset bytes.
// 4. If whence is SEEK_HOLE, the offset is set to the start of the next hole
//    greater than or equal to the supplied offset.  The definition of a hole is
//    provided below.
// 5. If whence is SEEK_DATA, the offset is set to the start of the next non-
//    hole file region greater than or equal to the supplied offset.
// RETURN VALUES
//  Upon successful completion, lseek() returns the resulting offset location as
//  measured in bytes from the beginning of the file.  Otherwise, a value of -1
//  is returned and errno is set to indicate the error.
long file_seek(File file, long offset, int whence) {
  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: %d (%s) %ld %d.\n", __func__, file,
             VfdCache[file].filename, offset, whence));

  if (FILE_IS_NOT_OPEN(file)) {
    switch (whence) {
      case SEEK_SET:
        if (offset < 0) {
          elog(ERROR, "%s: invalid offset: %ld", __func__, offset);
        }

        VfdCache[file].seek_pos = offset;
        break;

      case SEEK_CUR:
        VfdCache[file].seek_pos += offset;
        break;

      case SEEK_END:
        // 如果SEEK_END，文件可能需要扩容，这个时候需要打开文件
        file_access(file);
        VfdCache[file].seek_pos = lseek(VfdCache[file].fd, offset, whence);
        break;

      default:
        elog(ERROR, "%s: invalid whence %d", __func__, whence);
        break;
    }
  } else {
    switch (whence) {
      case SEEK_SET:
        if (offset < 0) {
          elog(ERROR, "%s: invalid offset: %ld", __func__, offset);
        }

        if (VfdCache[file].seek_pos != offset) {
          VfdCache[file].seek_pos = lseek(VfdCache[file].fd, offset, whence);
        }

        break;

      case SEEK_CUR:
        if (offset != 0 || VfdCache[file].seek_pos == FILE_UNKNOWN_POS) {
          VfdCache[file].seek_pos = lseek(VfdCache[file].fd, offset, whence);
        }

        break;

      case SEEK_END:
        VfdCache[file].seek_pos = lseek(VfdCache[file].fd, offset, whence);
        break;

      default:
        elog(ERROR, "%s: invalid whence: %d", __func__, whence);
        break;
    }
  }

  return VfdCache[file].seek_pos;
}

int file_truncate(File file, long offset) {
  int return_code;

  ASERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: %d (%s).\n", __func__, file, VfdCache[file].filename));

  // TODO(gc): truncate之前为什么要sync
  file_sync(file);
  file_access(file);
  return_code = ftruncate(VfdCache[file].fd, (size_t)offset);

  return return_code;
}

// FileSync --- if a file is marked as dirty, fsync it.
//
// The FD_DIRTY bit is slightly misnamed: it doesn't mean that we need to
// write the file, but that we *have* written it and need to execute an
// fsync() to ensure the changes are down on disk before we mark the current
// transaction committed.
//
// FD_DIRTY is set by FileWrite or by an explicit FileMarkDirty() call.
// It is cleared after successfully fsync'ing the file. FileClose() will
// fsync a dirty File that is about to be closed, since there will be no
// other place to remember the need to fsync after the VFD is gone.
//
// Note that the DIRTY bit is logically associated with the actual disk file,
// not with any particular kernel FD we might have open for it. We assume
// that fsync will force out any dirty buffers for that file, whether or not
// they were written through the FD being used for the fsync call --- they
// might even have been written by some other backend!
//
// Note also that LruDelete currently fsyncs a dirty file that it is about
// to close the kernel file descriptor for. The idea there is to avoid
// having to re-open the kernel descriptor later. But it's not real clear
// that this is a performance win; we could end up fsyncing the same file
// multiple times in a transaction, which would probably cost more time
// than is saved by avoiding an open() call. This should be studied.
//
// This routine used to think it could skip the fsync if the file is
// physically closed, but that is now WRONG; see comments for FileMarkDirty.
int file_sync(File file) {
  int return_code;

  ASSERT(FILE_IS_VALID(file));

  if (!(VfdCache[file].fdstate & FD_DIRTY)) {
    // Need not sync if file is not dirty.
    return_code = 0;
  } else if (!EnableFsync) {
    return_code = 0;
    VfdCache[file].fdstate &= ~FD_DIRTY;
  } else {
    // We don't use FileAccess() because we don't want to force the
    // file to the front of the LRU ring; we aren't expecting to
    // access it again soon.
    // TODO(gc): 如果文件都没有打开 fsync有什么意义
    if (FILE_IS_NOT_OPEN(file)) {
      return_code = lru_insert(file);

      if (return_code != 0) {
        return return_code;
      }

      return_code = pg_fsync(VfdCache[file].fd);

      if (return_code == 0) {
        VfdCache[file].fdstate &= ~FD_DIRTY;
      }
    }
  }

  return return_code;
}

// FileMarkDirty --- mark a file as needing fsync at transaction commit.
//
// Since FileWrite marks the file dirty, this routine is not needed in
// normal use.	It is called when the buffer manager detects that some other
// backend has written out a shared buffer that this backend dirtied (but
// didn't write) in the current xact.  In that scenario, we need to fsync
// the file before we can commit.  We cannot assume that the other backend
// has fsync'd the file yet; we need to do our own fsync to ensure that
// (a) the disk page is written and (b) this backend's commit is delayed
// until the write is complete.
//
// Note we are assuming that an fsync issued by this backend will write
// kernel disk buffers that were dirtied by another backend.  Furthermore,
// it doesn't matter whether we currently have the file physically open;
// we must fsync even if we have to re-open the file to do it.
void file_mark_dirty(File file) {
  ASSERT(FILE_IS_VALID(file));

  DO_DB(elog(DEBUG, "%s: %d (%s).\n", __func__, file, VfdCache[file].filename));

  VfdCache[file].fdstate |= FD_DIRTY;
}

// Routines that want to use stdio (ie, FILE*) should use AllocateFile
// rather than plain fopen().  This lets fd.c deal with freeing FDs if
// necessary to open the file.	When done, call FreeFile rather than fclose.
//
// Note that files that will be open for any significant length of time
// should NOT be handled this way, since they cannot share kernel file
// descriptors with other files; there is grave risk of running out of FDs
// if anyone locks down too many FDs.  Most callers of this routine are
// simply reading a config file that they will read and close immediately.
//
// fd.c will automatically close all files opened with AllocateFile at
// transaction commit or abort; this prevents FD leakage if a routine
// that calls AllocateFile is terminated prematurely by elog(ERROR).
//
// Ideally this should be the *only* direct call of fopen() in the backend.
FILE* allocate_file(char* name, char* mode) {
  FILE* file;

  DO_DB(elog(DEBUG, "%s: allocated %d", __func__, NumAllocatedFiles));

  if (NumAllocatedFiles >= MAX_ALLOCATED_FILES) {
    elog(ERROR, "%s: too many private FDs demanded", __func__);
  }

try_again:
  if ((file = fopen(name, mode)) != NULL) {
    AllocatedFiles[NumAllocatedFiles] = file;
    NumAllocatedFiles++;
    return file;
  }

  if (errno == EMFILE || errno == ENFILE) {
    int save_errno = errno;

    DO_DB(elog(DEBUG, "%s: not enough descs, retry, er= %d", __func__, errno));
    errno = 0;

    if (release_lru_file()) {
      goto try_again;
    }

    errno = save_errno;
  }

  return NULL;
}

void free_file(FILE* file) {
  int i;

  DO_DB(elog(DEBUG, "%s: allocated %d", __func__, NumAllocatedFiles));

  /* Remove file from list of allocated files, if it's present */
  for (i = NumAllocatedFiles; --i >= 0;) {
    if (AllocatedFiles[i] == file) {
      NumAllocatedFiles--;
      AllocatedFiles[i] = AllocatedFiles[NumAllocatedFiles];
      break;
    }
  }

  if (i < 0) {
    elog(NOTICE, "%s: file was not obtained from AllocateFile", __func__);
  }

  fclose(file);
}

int basic_open_file(FileName filename, int file_flags, int file_mode) {
  int fd;

try_again:
  fd = open(filename, file_flags, file_mode);

  if (fd >= 0) {
    return fd;
  }

  // EMFILE表示too many open files
  // ENFILE表示too many open files in system
  if (errno == EMFILE || errno == ENFILE) {
    int save_errno = errno;
    DO_DB(elog(DEBUG, "%s: not enough descs, retry, er = %d", errno));
    errno = 0;

    // 如果成功能够重新释放旧文件描述符
    // 那么再尝试打开
    if (release_lru_file) {
      goto try_again;
    }

    errno = save_errno;
  }

  return -1;
}

// closeAllVfds
//
// Force all VFDs into the physically-closed state, so that the fewest
// possible number of kernel file descriptors are in use.  There is no
// change in the logical state of the VFDs.
void close_all_vfds(void) {
  Index i;

  if (SizeVfdCache > 0) {
    ASSERT(FILE_IS_NOT_OPEN(0));

    for (i = 1; i < SizeVfdCache; i++) {
      if (!FILE_IS_NOT_OPEN(i)) {
        lru_delete(i);
      }
    }
  }
}

// This routine is called during transaction commit or abort or backend
// exit (it doesn't particularly care which).  All still-open temporary-file
// VFDs are closed, which also causes the underlying files to be deleted.
// Furthermore, all "allocated" stdio files are closed.
//
// This routine is not involved in fsync'ing non-temporary files at xact
// commit; that is done by FileSync under control of the buffer manager.
// During a commit, that is done *before* control gets here.  If we still
// have any needs-fsync bits set when we get here, we assume this is abort
// and clear them.
void at_eo_xact_files(void) {
  Index i;

  if (SizeVfdCache > 0) {
    ASSERT(FILE_IS_NOT_OPEN(0));

    for (i = 1; i < SizeVfdCache; i++) {
      if ((VfdCache[i].fdstate & FD_TEMPORARY) &&
          VfdCache[i].filename != NULL) {
        file_close(i);
      } else {
        VfdCache[i].fdstate &= ~FD_DIRTY;
      }
    }
  }

  while (NumAllocatedFiles > 0) {
    free_file(AllocatedFiles[0]);
  }

  // Reset the tempfile name counter to 0; not really necessary, but
  // helps keep the names from growing unreasonably long.
  TempFileCounter = 0;
}

// pg_fsync --- same as fsync except does nothing if -F switch was given.
int pg_fsync(int fd) {
  if (EnableFsync) {
    return fsync(fd);
  }

  return 0;
}

// pg_fdatasync --- same as fdatasync except does nothing if enableFsync is off
//
// Not all platforms have fdatasync; treat as fsync if not available.
int pg_fdatasync(int fd) {
  if (EnableFsync) {
#ifdef HAVE_FDATASYNC
    return fdatasync(fd);
#else
    return fsync(fd);
#endif
  } else {
    return 0;
  }
}

static void dump_lru() {}

static void delete (File file) {
  Vfd* vfdp;

  ASSERT(file != 0);

  DO_DB(elog(DEBUG, "Delete %d (%s)", file, VfdCache[file].filename));
  DO_DB(dump_lru());

  vfdp = &VfdCache[file];
  VfdCache[vfdp->lru_less_recently].lru_more_recently = vfdp->lru_more_recently;
  VfdCache[vfdp->lru_more_recently].lru_less_recently = vfdp->lru_less_recently;

  DO_DB(dump_lru());
}

static void lru_delete(File file) {
  Vfd* vfdp;
  int return_value;

  ASSERT(file != 0);

  DO_DB(elog(DEBUG, "%s %d (%s)", __func__, file, VfdCache[file].filename));

  vfdp = &VfdCache[file];

  // Delete the vfd record from the LRU ring.
  delete (file);

  // Save the seek position.
  // TODO(gc): why save the seek position?
  vfdp->seek_pos = (long)lseek(vfdp->fd, 0L, SEEK_CUR);
  ASSERT(vfdp->seek_pos != -1);

  // If we have written to the file, sync it before closing.
  if (vfdp->fdstate & FD_DIRTY) {
    return_value = pg_fsync(vfdp->fd);
    ASSERT(return_value != -1);
    vfdp->fdstate &= ~FD_DIRTY;
  }

  // Close the file.
  return_value = close(vfdp->fd);
  ASSERT(return_value != -1);

  --NFile;
  vfdp->fd = VFD_CLOSED;
}

static void insert(File file) {
  Vfd* vfdp;

  ASSERT(file != 0);

  DO_DB(elog(DEBUG, "Insert %d (%s)", file, VfdCache[file].filename));
  DO_DB(dump_lru());

  vfdp = &VfdCache[file];
  vfdp->lru_more_recently = 0;
  vfdp->lru_less_recently = VfdCache[0].lru_less_recently;
  VfdCache[0].lru_less_recently = file;
  VfdCache[vfdp->lru_less_recently].lru_more_recently = file;

  DO_DB(dump_lru());
}

// TODO(gc): 这个函数有什么用法？
static int lru_insert(File file) {
  Vfd* vfdp;
  int return_value;

  ASSERT(file != 0);

  DO_DB(elog(DEBUG, "LRU insert %d (%s)", file, VfdCache[file].filename));

  vfdp = &VfdCache[file];

  if (FILE_IS_NOT_OPEN(file)) {
    while (NFile + NumAllocatedFiles >= pg_nofile()) {
      // release_lru_file返回false表示NFile等于0 没有打开的文件
      if (!release_lru_file()) {
        break;
      }
    }

    // The open could still fail for lack of file descriptors, eg due
    // to overall system file table being full.  So, be prepared to
    // release another FD if necessary...
    vfdp->fd =
        basic_open_file(vfdp->filename, vfdp->file_flags, vfdp->file_mode);

    if (vfdp->fd < 0) {
      DO_DB(elog(DEBUG, "%s: reopen failed: %d", __func__, errno));
      return vfdp->fd;
    } else {
      DO_DB(elog(DEBUG, "%s: reopen success", __func__));
      ++NFile;
    }

    // Seek to the right position.
    if (vfdp->seek_pos != 0L) {
      return_value = lseek(vfdp->fd, vfdp->seek_pos, SEEK_SET);
      ASSERT(return_value != -1);
    }
  }

  // Put it at the head of the Lru ring.
  insert(file);

  return 0;
}

static bool release_lru_file() {
  DO_DB(elog(DEBUG, "%s: Opened %d", __func__, NFile));

  if (NFile > 0) {
    // There are opened files and so there should be at least one used vfd in
    // the ring.
    ASSERT(VfdCache[0].lru_more_recently != 0);
    lru_delete(VfdCache[0].lru_more_recently);

    return true;
  }

  return false;
}

static File allocate_vfd(void) {
  Index i;
  File file;

  DO_DB(elog(DEBUG, "%s: allocate vfd. Size %d", __func__, SizeVfdCache));

  if (SizeVfdCache == 0) {
    VfdCache = (Vfd*)malloc(sizeof(Vfd));
    if (VfdCache == NULL) {
      elog(FATAL, "%s: no room for VFD array", __func__);
    }

    MEMSET((char*)&(VfdCache[0]), 0, sizeof(Vfd));
    VfdCache->fd = VFD_CLOSED;
    SizeVfdCache = 1;

    // Register proc-exit call to ensure temp files are dropped at exit.
    on_proc_exit(at_eo_xact_files, 0);
  }

  if (VfdCache[0].next_free == 0) {
    // The free list is empty so it is time to increase the size of
    // the array. We choose to double it each time this happens.
    // However, there's not much point in starting *real* small.
    Size new_cache_size = SizeVfdCache * 2;
    Vfd* new_vfd_cache;

    if (new_cache_size < 32) {
      new_cache_size = 32;
    }

    // Be careful not to clobber VfdCache ptr if realloc fails;
    // we will need it during proc_exit cleanup!
    new_vfd_cache = (Vfd*)realloc(VfdCache, sizeof(Vfd) * new_cache_size);

    if (new_vfd_cache == NULL) {
      elog(FATAL, "%s: no room to enlarge VFD array", __func__);
    }

    VfdCache = new_vfd_cache;

    // Initialize the new entries and link them into the free list.
    for (i = SizeVfdCache; i < new_cache_size; i++) {
      MEMSET((char*)&(VfdCache[i]), 0, sizeof(Vfd));
      VfdCache[i].next_free = i + 1;
      VfdCache[i].fd = VFD_CLOSED;
    }

    VfdCache[new_cache_size - 1].next_free = 0;
    VfdCache[0].next_free = SizeVfdCache;

    // Record the new size.
    SizeVfdCache = new_cache_size;
  }

  file = VfdCache[0].next_free;
  VfdCache[0].next_free = VfdCache[file].next_free;

  return file;
}

static void free_vfd(File file) {
  Vfd* vfdp = &VfdCache[file];

  DO_DB(elog(DEBUG, "%s: `%s` %d (%s).\n", __func__, file,
             vfdp->filename ? vfdp->filename : ""));

  if (vfdp->filename != NULL) {
    free(vfdp->filename);
    vfdp->filename = NULL;
  }

  vfdp->fdstate = 0;
  vfdp->next_free = VfdCache[0].next_free;
  VfdCache[0].next_free = file;
}

// 将这个文件描述符放在最前面
static int file_access(File file) {
  int return_value;

  DO_DB(elog(DEBUG, "%s %s (%s)", __func__, file, VfdCache[file].filename));

  // Is the file open? If not, open it and put it at the head of the
  // LRU ring (possibly closing the least recently used file to get an
  // FD).
  if (FILE_IS_NOT_OPEN(file)) {
    return_value = lru_insert(file);

    if (return_value != 0) {
      return return_value;
    }
  } else if (VfdCache[0].lru_less_recently != file) {
    // We now know that the file is open and that it is not the last
    // one accessed, so we need to move it to the head of the Lru
    // ring.
    delete (file);
    insert(file);
  }

  return 0;
}

static File file_name_open_file_aux(FileName filename, int file_flags,
                                    int file_mode) {
  File file;
  Vfd* vfdp;

  if (filename == NULL) {
    elog(ERROR, "%s: NULL file name", __func__);
  }

  DO_DB(elog(DEBUG, "%s: %s %x %o", __func__, filename, file_flags, file_mode));

  file = allocate_vfd();
  // file如果不安全 在allocate中直接FATAL了
  vfdp = &VfdCache[file];

  while (NFile + NumAllocatedFiles >= pg_nofile()) {
    if (!release_lru_file()) {
      break;
    }
  }

  vfdp->fd = basic_open_file(filename, file_flags, file_mode);

  if (vfdp->fd < 0) {
    free_vfd(file);
    return -1;
  }

  ++NFile;

  DO_DB(elog(DEBUG, "%s: success %d", __func__, vfdp->fd);)

  insert(file);
  vfdp->filename = (char*)malloc(strlen(filename) + 1);

  if (vfdp->filename == NULL) {
    elog(FATAL, "%s: no room to save VFD filename", __func__);
  }

  strcpy(vfdp->filename, filename);

  // Saved flags are adjusted to be OK for re-opening file.
  vfdp->file_flags = file_flags & ~(O_CREAT | O_TRUNC | O_EXCL);
  vfdp->file_mode = file_mode;
  vfdp->seek_pos = 0;

  // Have to fsync file on commit. Alternative way - log file creation
  // and fsync log before actual file creation.
  if (file_flags & O_CREAT) {
    vfdp->fdstate = FD_DIRTY;
  } else {
    vfdp->fdstate = 0;
  }

  return file;
}

// Convert given pathname to absolute.
//
// (Generally, this isn't actually necessary, considering that we
// should be cd'd into the database directory. Presently it is only
// necessary to do it in "bootstrap" mode. Maybe we should change
// bootstrap mode to do the cd, and save a few cycles/bytes here.)
static char* filepath(char* filename) {
  char* buf;
  int len;

  // Not an absolute path name? Then fill in with database path...
  if (*filename != SEP_CHAR) {
    len = strlen(DatabasePath) + strlen(filename) + 2;
    buf = (char*)malloc(len);
    sprintf(buf, "%s%c%s", DatabasePath, SEP_CHAR, filename);
  } else {
    buf = (char*)palloc(strlen(filename) + 1);
    strcpy(buf, filename);
  }

#ifdef FILEDEBUG
  printf("%s: path is %s\n", __func__, buf);
#endif

  return buf;
}

// Determine number of file descriptors that fd.c is allowed to use.
static long pg_nofile(void) {
  static long MaxFilePerProcess = 0;

  if (MaxFilePerProcess == 0) {
#ifndef HAVE_SYSCONF
    MaxFilePerProcess = (long)NOFILE;
#else
    MaxFilePerProcess = sysconf(_SC_OPEN_MAX);

    if (MaxFilePerProcess == -1) {
      elog(DEBUG, "%s: unable to get _SC_OPEN_MAX using sysconf(); using %d",
           __func__, NOFILE);
      MaxFilePerProcess = (long)NOFILE;
    }
#endif

    if ((MaxFilePerProcess - RESERVE_FOR_LD) < FD_MINFREE) {
      elog(FATAL,
           "%s: insufficient File Descriptors in postmaster to start backend "
           "(%ld).\n O/S allows %ld, Postmaster reserves %d, We need %d (MIN) "
           "after that.",
           __func__, MaxFilePerProcess - RESERVE_FOR_LD, MaxFilePerProcess,
           RESERVE_FOR_LD, FD_MINFREE);
    }

    MaxFilePerProcess -= RESERVE_FOR_LD;
  }

  return MaxFilePerProcess;
}
