// =========================================================================
//
// hsearch.h
//  for hash tables, particularly hash tables in shared memory
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: hsearch.h,v 1.15 2000/04/12 17:16:55 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_HSEARCH_H_
#define RDBMS_UTILS_HSEARCH_H_

#include <stdbool.h>

// Constants
//
// A hash table has a top-level "directory", each of whose entries points
// to a "segment" of ssize bucket headers. The maximum number of hash
// buckets is thus dsize * ssize (but dsize may be expansible). Of course,
// the number of records in the table can be larger, but we don't want a
// whole lot of records per bucket or performance goes down.
//
// In a hash table allocated in shared memory, the directory cannot be
// expanded because it must stay at a fixed address. The directory size
// should be selected using hash_select_dirsize (and you'd better have
// a good idea of the maximum number of entries!). For non-shared hash
// tables, the initial directory size can be left at the default.
#define DEF_SEGSIZE       256
#define DEF_SEGSIZE_SHIFT 8  // Must be log2(DEF_SEGSIZE)
#define DEF_DIRSIZE       256
#define DEF_FFACTOR       1  // Default fill factor

#define PRIME1 37  // For the hash function
#define PRIME2 1048583

// Hash bucket is actually bigger than this. Key field can have
// variable length and a variable length data field follows it.
typedef struct element {
  unsigned long next;  // Secret from user.
  long key;
} ELEMENT;

typedef unsigned long BUCKET_INDEX;

// Segment is an array of bucket pointers.
typedef BUCKET_INDEX* SEGMENT;
typedef unsigned long SEG_OFFSET;

typedef struct hashhdr {
  long dsize;                      // Directory size.
  long ssize;                      // Segment size, must be power of 2.
  long sshift;                     // Segment shift.
  long max_bucket;                 // ID of maximum bucket in use.
  long high_mask;                  // Mask to modulo into entire table.
  long low_mask;                   // Mask to modulo into lower half of table.
  long ffactor;                    // Fill factor.
  long nkeys;                      // Number of keys in hash table.
  long nsegs;                      // Number of allocated segments.
  long keysize;                    // Hash key length in bytes.
  long datasize;                   // Element data length in bytes.
  long max_dsize;                  // 'dsize' limit if directory is fixed size.
  BUCKET_INDEX free_bucket_index;  // Index of first free bucket.
  long accesses;
  long collisions;
} HHDR;

typedef struct htab {
  HHDR* hctl;        // Shared control information.
  long (*hash)();    // Hash function.
  char* segbase;     // Segment base addres for calculating pointer values.
  SEG_OFFSET* dir;   // 'directory' of segm starts.
  long* (*alloc)();  // Memory allocator (long* for alignment reasons).
} HTAB;

typedef struct hashctl {
  long ssize;        // Segment size.
  long dsize;        // Dirsize size.
  long ffactor;      // Fill factor.
  long (*hash)();    // Hash function.
  long keysize;      // Hash key length in bytes.
  long datasize;     // Element data length in bytes.
  long max_dsize;    // Limit to dsize if directory size is limited.
  long* segbase;     // Base for calculating bucket + seg ptrs
  long* (*alloc)();  // Memory allocation function.
  long* dir;         // Directory if allocated already.
  long* hctl;        // Location of header information in shared memory.
} HASHCTL;

// Flags to indicate action for hctl.
#define HASH_SEGMENT    0x002  // Setting segment size.
#define HASH_DIRSIZE    0x004  // Setting directory size.
#define HASH_FFACTOR    0x008  // Setting fill factor.
#define HASH_FUNCTION   0x010  // Set user defined hash function.
#define HASH_ELEM       0x020  // Setting key/data size.
#define HASH_SHARED_MEM 0x040  // Setting shared mem const.
#define HASH_ATTACH     0x080  // Do not initialize hctl.
#define HASH_ALLOC      0x100  // Setting memory allocator.

// seg_alloc assumes that INVALID_INDEX is 0.
#define INVALID_INDEX (0)
#define NO_MAX_DSIZE  (-1)

// Number of hash buckets allocated at once
#define BUCKET_ALLOC_INCR (30)

// hash_search operations.
typedef enum { HASH_FIND, HASH_ENTER, HASH_REMOVE, HASH_FIND_SAVE, HASH_REMOVE_SAVED } HASHACTION;

HTAB* hash_create(int nelem, HASHCTL* info, int flags);
void hash_destroy(HTAB* hashp);
void hash_stats(char* where, HTAB* hashp);
long* hash_search(HTAB* hashp, char* key_ptr, HASHACTION action, bool* found_ptr);
long* hash_seq(HTAB* hashp);
long hash_estimate_size(long num_entries, long keysize, long datasize);
long hash_select_dirsize(long num_entries);

#endif  // RDBMS_UTILS_HSEARCH_H_
