#ifndef RDBMS_CONFIG_H_
#define RDBMS_CONFIG_H_

// Maximum number of columns in an index and maximum number of arguments
// to a function. They must be the same value.
//
// The minimum value is 9 (btree index creation has a 9-argument function).
//
// There is no maximum value, though if you want to pass more than 32
// arguments to a function, you will have to modify
// pgsql/src/backend/utils/fmgr/fmgr.c and add additional entries
// to the 'case' statement for the additional arguments.
#define INDEX_MAX_KEYS 16

// Maximum number of arguments to a function.
//
// The minimum value is 8 (index cost estimation uses 8-argument functions).
// The maximum possible value is around 600 (limited by index tuple size in
// pg_proc's index; BLCKSZ larger than 8K would allow more).  Values larger
// than needed will waste memory and processing time, but do not directly
// cost disk space.
//
// Changing this does not require an initdb, but it does require a full
// backend recompile (including any user-defined C functions).
#define FUNC_MAX_ARGS INDEX_MAX_KEYS

// Size of a disk block --- currently, this limits the size of a tuple.
// You can set it bigger if you need bigger tuples.
//
// Currently must be <= 32k bjm */
#define BLCKSZ 8192

// RELSEG_SIZE is the maximum number of blocks allowed in one disk file.
// Thus, the maximum size of a single file is RELSEG_SIZE * BLCKSZ;
// relations bigger than that are divided into multiple files.
//
// CAUTION: RELSEG_SIZE * BLCKSZ must be less than your OS' limit on file
// size.  This is typically 2Gb or 4Gb in a 32-bit operating system.  By
// default, we make the limit 1Gb to avoid any possible integer-overflow
// problems within the OS.  A limit smaller than necessary only means we
// divide a large relation into more chunks than necessary, so it seems
// best to err in the direction of a small limit.  (Besides, a power-of-2
// value saves a few cycles in md.c.)
//
// CAUTION: you had best do an initdb if you change either BLCKSZ or
// RELSEG_SIZE.
#define RELSEG_SIZE (0x40000000 / BLCKSZ)

// As soon as the backend blocks on a lock, it waits this number of seconds
// before checking for a deadlock.
// We don't check for deadlocks just before sleeping because a deadlock is
// a rare event, and checking is an expensive operation.
#define DEADLOCK_CHECK_TIMER 1

// Maximum number of columns in an index and maximum number of arguments
// to a function. They must be the same value.
//
// The minimum value is 9 (btree index creation has a 9-argument function).
//
// There is no maximum value, though if you want to pass more than 32
// arguments to a function, you will have to modify
// pgsql/src/backend/utils/fmgr/fmgr.c and add additional entries
// to the 'case' statement for the additional arguments.
#define INDEX_MAX_KEYS 16
#define FUNC_MAX_ARGS  INDEX_MAX_KEYS

#define MAX_PG_PATH 1024

// Memory context debug.
#define MEMORY_CONTEXT_CHECKING
#define HAVE_ALLOC_INFO
#define CLOBBER_FREED_MEMORY

// Part 2: feature symbols and limits that are user-configurable, but
// only by editing this file ... there's no configure support for them.
//
// Editing this file and doing a full rebuild (and an initdb if noted)
// should be sufficient to change any of these.

// Hard limit on number of backend server processes per postmaster.
// Increasing this costs about 32 bytes per process slot as of v 6.5.
#define DEF_MAXBACKENDS 32
#define MAX_BACKENDS    (DEF_MAXBACKENDS > 1024 ? DEF_MAXBACKENDS : 1024)

#define SIZEOF_DATUM 8

#endif  // RDBMS_CONFIG_H_