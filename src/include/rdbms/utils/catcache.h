//===----------------------------------------------------------------------===//
//
// catcache.h
//  Low-level catalog cache definitions.
//
// NOTE: every catalog cache must have a corresponding unique index on
// the system table that it caches --- ie, the index must match the keys
// used to do lookups in this cache.  All cache fetches are done with
// indexscans (under normal conditions).  The index should be unique to
// guarantee that there can only be one matching row for a key combination.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: catcache.h,v 1.32 2001/03/22 04:01:11 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_CATCACHE_H_
#define RDBMS_UTILS_CATCACHE_H_

#include "rdbms/access/htup.h"
#include "rdbms/access/skey.h"
#include "rdbms/access/tupdesc.h"
#include "rdbms/lib/dllist.h"

// struct catctup:  individual tuple in the cache.
// struct catcache: information for managing a cache.
typedef struct CatCTup {
  int ct_magic;
#define CT_MAGIC 0x57261502

  // Each tuple in a cache is a member of two lists: one lists all the
  // elements in that cache in LRU order, and the other lists just the
  // elements in one hashbucket, also in LRU order.
  //
  // A tuple marked "dead" must not be returned by subsequent searches.
  // However, it won't be physically deleted from the cache until its
  // refcount goes to zero.
  Dlelem lru_list_elem;  // List member of global LRU list
  Dlelem cache_elem;     // List member of per-bucket list
  int refcount;          // Number of active references
  bool dead;             // Dead but not yet removed?
  HeapTupleData tuple;   // Tuple management header
} CatCTup;

#define NCC_BUCK 500  // CatCache buckets
#define MAX_TUP  500  // Maximum # of tuples stored per cache

typedef struct CatCache {
  int id;                      // Cache identifier --- see syscache.h
  struct CatCache* cc_next;    // Link to next catcache
  char* cc_rel_name;           // Name of relation the tuples come from
  char* cc_ind_name;           // Name of index matching cache keys
  TupleDesc cc_tup_desc;       // Tuple descriptor (copied from reldesc)
  short cc_ntup;               // # of tuples in this cache
  short cc_max_tup;            // max # of tuples allowed (LRU)
  short cc_size;               // # of hash buckets in this cache
  short cc_nkeys;              // Number of keys (1..4)
  short cc_key[4];             // AttrNumber of each key
  PGFunction cc_hash_func[4];  // Hash function to use for each key
  ScanKeyData cc_skey[4];      // Precomputed key info for heap scans
  Dllist cc_lru_list;          // Overall LRU list, most recent first
  Dllist cc_cache[NCC_BUCK];   // Hash buckets
} CatCache;

#define INVALID_CATALOG_CACHE_ID (-1)

// This extern duplicates utils/memutils.h
extern MemoryContext CacheMemoryContext;

void create_cache_memory_context();

#endif  // RDBMS_UTILS_CATCACHE_H_
