//===----------------------------------------------------------------------===//
//
// catcache.c
//  System catalog cache for tuples matching a key.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/cache/catcache.c
//  v 1.77 2001/03/22 03:59:55 momjian Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/utils/catcache.h"

#include "rdbms/access/heapam.h"
#include "rdbms/utils/elog.h"

static void cat_cache_remove_ctup(CatCache* cache, CatCTup* ct);
static Index catalog_cache_compute_hash_index(CatCache* cache,
                                              ScanKey cur_skey);
static Index catalog_cache_compute_tuple_hash_index(CatCache* cache,
                                                    HeapTuple tuple);
static void catalog_cache_initialize_cache(CatCache* cache);

#ifdef CACHEDEBUG
#define CACHE1_elog(a, b)                elog(a, b)
#define CACHE2_elog(a, b, c)             elog(a, b, c)
#define CACHE3_elog(a, b, c, d)          elog(a, b, c, d)
#define CACHE4_elog(a, b, c, d, e)       elog(a, b, c, d, e)
#define CACHE5_elog(a, b, c, d, e, f)    elog(a, b, c, d, e, f)
#define CACHE6_elog(a, b, c, d, e, f, g) elog(a, b, c, d, e, f, g)
#else
#define CACHE1_elog(a, b)
#define CACHE2_elog(a, b, c)
#define CACHE3_elog(a, b, c, d)
#define CACHE4_elog(a, b, c, d, e)
#define CACHE5_elog(a, b, c, d, e, f)
#define CACHE6_elog(a, b, c, d, e, f, g)
#endif

/*
 *		CatalogCacheInitializeCache
 *
 * This function does final initialization of a catcache: obtain the tuple
 * descriptor and set up the hash and equality function links.	We assume
 * that the relcache entry can be opened at this point!
 *
 */
#ifdef CACHEDEBUG
#define CATALOG_CACHE_INITIALIZE_CACHE_DEBUG1 \
  elog(DEBUG, "%s: cache @%p %s", __func__, cache, cache->cc_rel_name)

#define CATALOG_CACHE_INITIALIZE_CACHE_DEBUG2                                  \
  do {                                                                         \
    if (cache->cc_key[i] > 0) {                                                \
      elog(DEBUG, "%s: load %d/%d w/%d, %u", __func__, i + 1, cache->cc_nkeys, \
           cache->cc_key[i], tup_desc->attrs[cache->cc_key[i] - 1]->atttypid); \
    } else {                                                                   \
      elog(DEBUG, "%s: load %d/%d w/%d", __func__, i + 1, cache->cc_nkeys,     \
           cache->cc_key[i]);                                                  \
    }                                                                          \
  } while (0)

#else
#define CATALOG_CACHE_INITIALIZE_CACHE_DEBUG1
#define CATALOG_CACHE_INITIALIZE_CACHE_DEBUG2
#endif

static CatCache* Caches = NULL;  // Head of list of caches

void create_cache_memory_context();

static void cat_cache_remove_ctup(CatCache* cache, CatCTup* ct) {
  ASSERT(ct->refcount == 0);

  // Delink from linked lists.
  DLRemove(&ct->lru_list_elem);
  DLRemove(&ct->cache_elem);

  // Free associated tuple data.
  if (ct->tuple.t_data != NULL) {
    pfree(ct->tuple.t_data);
  }

  pfree(ct);

  --cache->cc_ntup;
}

static Index catalog_cache_compute_hash_index(CatCache* cache,
                                              ScanKey cur_skey) {
  uint32 hash_index = 0;

  CACHE4_elog(DEBUG, "catalog_cache_compute_hash_index %s %d %p",
              cache->cc_rel_name, cache->cc_nkeys, cache);

  switch (cache->cc_nkeys) {
    case 4:
      hash_index ^= DATUM_GET_UINT32(direct_function_call1(
                        cache->cc_hash_func[3], cur_skey[3].sk_argument))
                    << 9;
      // FALLTHROUGH
    case 3:
      hash_index ^= DATUM_GET_UINT32(direct_function_call1(
                        cache->cc_hash_func[2], cur_skey[2].sk_argument))
                    << 6;
      // FALLTHROUGH
    case 2:
      hash_index ^= DATUM_GET_UINT32(direct_function_call1(
                        cache->cc_hash_func[1], cur_skey[1].sk_argument))
                    << 3;
      // FALLTHROUGH
    case 1:
      hash_index ^= DATUM_GET_UINT32(direct_function_call1(
          cache->cc_hash_func[0], cur_skey[0].sk_argument));
      break;

    default:
      elog(FATAL, "%s: %d cc_nkeys", __func__, cache->cc_nkeys);
      break;
  }

  hash_index %= (uint32)cache->cc_size;

  return hash_index;
}

static Index catalog_cache_compute_tuple_hash_index(CatCache* cache,
                                                    HeapTuple tuple) {
  ScanKeyData cur_skey[4];
  bool is_null = false;

  // Copy pre-initialized overhead data for scankey.
  memcpy(cur_skey, cache->cc_skey, sizeof(cur_skey));

  // Now extract key fields from tuple, insert into scankey.
  switch (cache->cc_nkeys) {
    case 4:
      cur_skey[3].sk_argument =
          (cache->cc_key[3] == OBJECT_ID_ATTRIBUTE_NUMBER)
              ? OBJECT_ID_GET_DATUM(tuple->t_data->t_oid)
              : FAST_GET_ATTR(tuple, cache->cc_key[3], cache->cc_tup_desc,
                              &is_null);
      ASSERT(!is_null);
      // FALLTHROUGH
    case 3:
      cur_skey[2].sk_argument =
          (cache->cc_key[2] == OBJECT_ID_ATTRIBUTE_NUMBER)
              ? ObjectIdGetDatum(tuple->t_data->t_oid)
              : FAST_GET_ATTR(tuple, cache->cc_key[2], cache->cc_tup_desc,
                              &is_null);
      ASSERT(!is_null);
      // FALLTHROUGH
    case 2:
      cur_skey[1].sk_argument =
          (cache->cc_key[1] == OBJECT_ID_ATTRIBUTE_NUMBER)
              ? OBJECT_ID_GET_DATUM(tuple->t_data->t_oid)
              : FAST_GET_ATTR(tuple, cache->cc_key[1], cache->cc_tup_desc,
                              &is_null);
      ASSERT(!is_null);
      // FALLTHROUGH
    case 1:
      cur_skey[0].sk_argument =
          (cache->cc_key[0] == OBJECT_ID_ATTRIBUTE_NUMBER)
              ? OBJECT_ID_GET_DATUM(tuple->t_data->t_oid)
              : FAST_GET_ATTR(tuple, cache->cc_key[0], cache->cc_tup_desc,
                              &is_null);
      ASSERT(!is_null);
      break;
    default:
      elog(FATAL, "%s: %d cc_nkeys", __func__, cache->cc_nkeys);
      break;
  }

  return catalog_cache_compute_hash_index(cache, cur_skey);
}

static void catalog_cache_initialize_cache(CatCache* cache) {
  Relation relation;
  MemoryContext old_cxt;
  TupleDesc tup_desc;
  short i;

  CATALOG_CACHE_INITIALIZE_CACHE_DEBUG1;

  // Open the relation without locking --- we only need the tupdesc,
  // which we assume will never change ...
}