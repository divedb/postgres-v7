//===----------------------------------------------------------------------===//
//
// miscinit.c
//  miscellaneous initialization support stuff
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/init/miscinit.c
//  v 1.65.2.1 2001/08/08 22:25:45 tgl Exp $
//
//===----------------------------------------------------------------------===//
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"

#ifdef CYR_RECODE
unsigned char RecodeForwTable[128];
unsigned char RecodeBackTable[128];
#endif

ProcessingMode Mode = InitProcessing;

// Note: we rely on these to initialize as zeroes.
static char DirectoryLockFile[MAX_PG_PATH];
static char SocketLockFile[MAX_PG_PATH];

static bool IsIgnoringSystemIndexes = false;

// The session user is determined at connection start and never
// changes. The current user may change when "setuid" functions
// are implemented. Conceptually there is a stack, whose bottom
// is the session user. You are yourself responsible to save and
// restore the current user id if you need to change it.
static Oid CurrentUserId = INVALID_OID;
static Oid SessionUserId = INVALID_OID;

void set_database_name(const char* name) {
  free(DatabaseName);

  if (name) {
    DatabaseName = strdup(name);
    ASSERT_STATE(DatabaseName);
  }
}

void set_database_path(const char* path) {
  free(DatabasePath);

  // Use strdup since this is done before memory contexts are set up.
  if (path) {
    DatabasePath = strdup(path);
    ASSERT_STATE(DatabasePath);
  }
}

Oid get_user_id() {
  ASSERT_STATE(OID_IS_VALID(CurrentUserId));
  return CurrentUserId;
}

void set_user_id(Oid user_id) {
  ASSERT_ARG(OID_IS_VALID(user_id));

  CurrentUserId = user_id;
}

Oid get_session_user_id() {
  ASSERT_STATE(OID_IS_VALID(SessionUSerId));

  return SessionUserId;
}

void set_session_user_id(Oid user_id) {
  ASSERT_ARG(OID_IS_VALID(user_id));

  SessionUserId = user_id;

  // Current user defaults to session user.
  if (!OID_IS_VALID(CurrentUserId)) {
    CurrentUserId = user_id;
  }
}
// TODO(gc): add this definition later.
void set_session_user_id_from_username(const char* username) {}

// Set data directory, but make sure it's an absolute path.  Use this,
// never set DataDir directly.
void set_data_dir(const char* dir) {
  char* new_dir;

  ASSERT_ARG(dir);

  if (DataDir) {
    free(DataDir);
  }

  if (dir[0] != '/') {
    char* buf;
    size_t buflen;

    buflen = MAX_PG_PATH;
    for (;;) {
      buf = malloc(buflen);

      if (!buf) {
        elog(FATAL, "%s: out of memory", __func__);
      }

      if (getcwd(buf, buflen)) {
        break;
      } else if (errno == ERANGE) {
        free(buf);
        buflen *= 2;
        continue;
      } else {
        free(buf);
        elog(FATAL, "cannot get current working directory: %m");
      }
    }

    new_dir = malloc(strlen(buf) + 1 + strlen(dir) + 1);
    sprintf(new_dir, "%s/%s", buf, dir);
    free(buf);
  } else {
    new_dir = strdup(dir);
  }

  if (!new_dir) {
    elog(FATAL, "%s: out of memory", __func__);
  }

  DataDir = new_dir;
}

int find_exec(char* fullpath, const char* argv0, const char* binary_name);
int check_path_access(char* path, char* name, int open_mode);

// Append information about a shared memory segment to the data directory
// lock file (if we have created one).
//
// This may be called multiple times in the life of a postmaster, if we
// delete and recreate shmem due to backend crash. Therefore, be prepared
// to overwrite existing information. (As of 7.1, a postmaster only creates
// one shm seg anyway; but for the purposes here, if we did have more than
// one then any one of them would do anyway.)
void record_shared_memory_in_lock_file(IpcMemoryKey shm_key,
                                       IpcMemoryId shm_id) {
  int fd;
  int len;
  char* ptr;
  char buffer[BLCKSZ];

  // Do nothing if we did not create a lockfile (probably because we are
  // running standalone).
  if (DirectoryLockFile[0] == '\0') {
    return;
  }

  fd = open(DirectoryLockFile, O_RDWR | PG_BINARY, 0);
  if (fd < 0) {
    elog(DEBUG, "%s: failed to rewrite %s: %m", __func__, DirectoryLockFile);
    return;
  }

  len = read(fd, buffer, sizeof(buffer) - 100);

  if (len <= 0) {
    elog(DEBUG, "%s: failed to read %s: %m", __func__, DirectoryLockFile);
    close(fd);
    return;
  }

  buffer[len] = '\0';

  // Skip over first two lines (PID and path).
  ptr = strchr(buffer, '\n');
  if (ptr == NULL || (ptr = strchr(ptr + 1, '\n')) == NULL) {
    elog(DEBUG, "%s: bogus data in %s", __func__, DirectoryLockFile);
    close(fd);
    return;
  }
  ptr++;

  // Append shm key and ID.  Format to try to keep it the same length
  // always (trailing junk won't hurt, but might confuse humans).
  sprintf(ptr, "%9lu %9lu\n", (unsigned long)shm_key, (unsigned long)shm_id);

  // And rewrite the data.  Since we write in a single kernel call, this
  // update should appear atomic to onlookers.
  len = strlen(buffer);

  if (lseek(fd, (off_t)0, SEEK_SET) != 0 ||
      (int)write(fd, buffer, len) != len) {
    elog(DEBUG, "%s: failed to write %s: %m", __func__, DirectoryLockFile);
    close(fd);
    return;
  }

  close(fd);
}

void validate_pg_version(const char* path);

bool is_ignoring_system_indexes() { return IsIgnoringSystemIndexes; }
void ignore_system_indexes(bool mode) { IsIgnoringSystemIndexes = mode; }

// Database path / name support stuff.
