//===----------------------------------------------------------------------===//
//
// syscache.c
//  System cache management routines
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/cache/syscache.c
//  v 1.60 2001/03/22 03:59:57 momjian Exp $
//
// NOTES
//  These routines allow the parser/planner/executor to perform
//  rapid lookups on the contents of the system catalogs.
//
//  see catalog/syscache.h for a list of the cache id's
//
//===----------------------------------------------------------------------===//
#include "rdbms/utils/syscache.h"

#include "rdbms/catalog/catname.h"
#include "rdbms/catalog/indexing.h"
#include "rdbms/catalog/pg_aggregate.h"
#include "rdbms/catalog/pg_amop.h"
#include "rdbms/catalog/pg_group.h"
#include "rdbms/catalog/pg_index.h"
#include "rdbms/catalog/pg_inherits.h"
#include "rdbms/catalog/pg_language.h"
#include "rdbms/catalog/pg_listener.h"
#include "rdbms/catalog/pg_opclass.h"
#include "rdbms/catalog/pg_operator.h"
#include "rdbms/catalog/pg_proc.h"
#include "rdbms/catalog/pg_rewrite.h"
#include "rdbms/catalog/pg_shadow.h"
#include "rdbms/catalog/pg_statistic.h"
#include "rdbms/catalog/pg_type.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/catcache.h"
#include "rdbms/utils/elog.h"

// Adding system caches:
//
// Add your new cache to the list in include/utils/syscache.h. Keep
// the list sorted alphabetically and adjust the cache numbers
// accordingly.
//
// Add your entry to the cacheinfo[] array below. All cache lists are
// alphabetical, so add it in the proper place. Specify the relation
// name, index name, number of keys, and key attribute numbers.
//
// In include/catalog/indexing.h, add a define for the number of indexes
// on the relation, add define(s) for the index name(s), add an extern
// array to hold the index names, and use DECLARE_UNIQUE_INDEX to define
// the index. Cache lookups return only one row, so the index should
// be unique in most cases.
//
// In backend/catalog/indexing.c, initialize the relation array with
// the index names for the relation.
//
// Finally, any place your relation gets heap_insert() or
// heap_update calls, include code to do a CatalogIndexInsert() to update
// the system indexes.  The heap_* calls do not update indexes.
//
// bjm 1999/11/22

// Information defining a single syscache.
typedef struct CacheDesc {
  char* name;      // Name of the relation being cached
  char* ind_name;  // Name of index relation for this cache
  int nkeys;       // # of keys needed for cache lookup
  int key[4];      // Attribute numbers of key attrs
} CacheDesc;

static struct CacheDesc CacheInfo[] = {
    {AggregateRelationName,  // AGGNAME
     AggregateNameTypeIndex,
     2,
     {Anum_pg_aggregate_aggname, Anum_pg_aggregate_aggbasetype, 0, 0}},
    {AccessMethodRelationName,  // AMNAME
     AmNameIndex,
     1,
     {Anum_pg_am_amname, 0, 0, 0}},
    {AccessMethodOperatorRelationName,  // AMOPOPID
     AccessMethodOpidIndex,
     3,
     {Anum_pg_amop_amopclaid, Anum_pg_amop_amopopr, Anum_pg_amop_amopid, 0}},
    {AccessMethodOperatorRelationName,  // AMOPSTRATEGY
     AccessMethodStrategyIndex,
     3,
     {Anum_pg_amop_amopid, Anum_pg_amop_amopclaid, Anum_pg_amop_amopstrategy,
      0}},
    {AttributeRelationName,  // ATTNAME
     AttributeRelidNameIndex,
     2,
     {Anum_pg_attribute_attrelid, Anum_pg_attribute_attname, 0, 0}},
    {AttributeRelationName,  // ATTNUM
     AttributeRelidNumIndex,
     2,
     {Anum_pg_attribute_attrelid, Anum_pg_attribute_attnum, 0, 0}},
    {OperatorClassRelationName,  // CLADEFTYPE
     OpclassDeftypeIndex,
     1,
     {Anum_pg_opclass_opcdeftype, 0, 0, 0}},
    {OperatorClassRelationName,  // CLANAME
     OpclassNameIndex,
     1,
     {Anum_pg_opclass_opcname, 0, 0, 0}},
    {GroupRelationName,  // GRONAME
     GroupNameIndex,
     1,
     {Anum_pg_group_groname, 0, 0, 0}},
    {GroupRelationName,  // GROSYSID
     GroupSysidIndex,
     1,
     {Anum_pg_group_grosysid, 0, 0, 0}},
    {IndexRelationName,  // INDEXRELID
     IndexRelidIndex,
     1,
     {Anum_pg_index_indexrelid, 0, 0, 0}},
    {InheritsRelationName,  // INHRELID
     InheritsRelidSeqnoIndex,
     2,
     {Anum_pg_inherits_inhrelid, Anum_pg_inherits_inhseqno, 0, 0}},
    {LanguageRelationName,  // LANGNAME
     LanguageNameIndex,
     1,
     {Anum_pg_language_lanname, 0, 0, 0}},
    {LanguageRelationName,  // LANGOID
     LanguageOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}},
    {ListenerRelationName,  // LISTENREL
     ListenerPidRelnameIndex,
     2,
     {Anum_pg_listener_pid, Anum_pg_listener_relname, 0, 0}},
    {OperatorRelationName,  // OPERNAME
     OperatorNameIndex,
     4,
     {Anum_pg_operator_oprname, Anum_pg_operator_oprleft,
      Anum_pg_operator_oprright, Anum_pg_operator_oprkind}},
    {OperatorRelationName,  // OPEROID
     OperatorOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}},
    {ProcedureRelationName,  // PROCNAME
     ProcedureNameIndex,
     3,
     {Anum_pg_proc_proname, Anum_pg_proc_pronargs, Anum_pg_proc_proargtypes,
      0}},
    {ProcedureRelationName,  // PROCOID
     ProcedureOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}},
    {RelationRelationName,  // RELNAME
     ClassNameIndex,
     1,
     {Anum_pg_class_relname, 0, 0, 0}},
    {RelationRelationName,  // RELOID
     ClassOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}},
    {RewriteRelationName,  // REWRITENAME
     RewriteRulenameIndex,
     1,
     {Anum_pg_rewrite_rulename, 0, 0, 0}},
    {RewriteRelationName,  // RULEOID
     RewriteOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}},
    {ShadowRelationName,  // SHADOWNAME
     ShadowNameIndex,
     1,
     {Anum_pg_shadow_usename, 0, 0, 0}},
    {ShadowRelationName,  // SHADOWSYSID
     ShadowSysidIndex,
     1,
     {Anum_pg_shadow_usesysid, 0, 0, 0}},
    {StatisticRelationName,  // STATRELID
     StatisticRelidAttnumIndex,
     2,
     {Anum_pg_statistic_starelid, Anum_pg_statistic_staattnum, 0, 0}},
    {TypeRelationName,  // TYPENAME
     TypeNameIndex,
     1,
     {Anum_pg_type_typname, 0, 0, 0}},
    {TypeRelationName,  // TYPEOID
     TypeOidIndex,
     1,
     {OBJECT_ID_ATTRIBUTE_NUMBER, 0, 0, 0}}};

static CatCache* SysCache[LENGTH_OF(CacheInfo)];
static int SysCacheSize = LENGTH_OF(CacheInfo);
static bool CacheInitialized = false;

bool is_cache_initialized() { return CacheInitialized; }

// Note that no database access is done here; we only allocate memory
// and initialize the cache structure. Interrogation of the database
// to complete initialization of a cache happens only upon first use
// of that cache.
void init_catalog_cache() {
  int cache_id;
  ASSERT(!CacheInitialized);

  MEMSET((char*)SysCache, 0, sizeof(SysCache));

  for (cache_id = 0; cache_id < SysCacheSize; cache_id++) {
    SysCache[cache_id] = init_cat_cache(
        cache_id, CacheInfo[cache_id].name, CacheInfo[cache_id].ind_name,
        CacheInfo[cache_id].nkeys, CacheInfo[cache_id].key);

    if (!POINTER_IS_VALID(SysCache[cache_id])) {
      elog(ERROR, "%s: Can't init cache %s (%d)", __func__,
           CacheInfo[cache_id].name, cache_id);
    }
  }

  CacheInitialized = true;
}

// A layer on top of SearchCatCache that does the initialization and
// key-setting for you.
//
// Returns the cache copy of the tuple if one is found, NULL if not.
// The tuple is the 'cache' copy and must NOT be modified!
//
// When the caller is done using the tuple, call ReleaseSysCache()
// to release the reference count grabbed by SearchSysCache(). If this
// is not done, the tuple will remain locked in cache until end of
// transaction, which is tolerable but not desirable.
//
// CAUTION: The tuple that is returned must NOT be freed by the caller!
HeapTuple search_sys_cache(int cache_id, Datum key1, Datum key2, Datum key3,
                           Datum key4) {
  if (cache_id < 0 || cache_id >= SysCacheSize) {
    elog(ERROR, "%s: Bad cache id %d", __func__, cache_id);
    return NULL;
  }

  ASSERT(POINTER_IS_VALID(SysCache[cache_id]));

  // If someone tries to look up a relname, translate temp relation
  // names to real names. Less obviously, apply the same translation to
  // type names, so that the type tuple of a temp table will be found
  // when sought. This is a kluge ... temp table substitution should be
  // happening at a higher level ...
  if (cache_id == RELNAME || cache_id == TYPENAME) {
    char* nontemp_relname;

    nontemp_relname = get_temp_rel_by_username(DATUM_GET_CSTRING(key1));

    if (nontemp_relname != NULL) {
      key1 = CSTRING_GET_DATUM(nontemp_relname);
    }
  }

  return search_cat_cache(SysCache[cache_id], key1, key2, key3, key4);
}

void release_sys_cache(HeapTuple tuple) { release_cat_cache(tuple); }

// A convenience routine that does SearchSysCache and (if successful)
// returns a modifiable copy of the syscache entry. The original
// syscache entry is released before returning. The caller should
// heap_freetuple() the result when done with it.
HeapTuple search_sys_cache_copy(int cache_id, Datum key1, Datum key2,
                                Datum key3, Datum key4) {
  HeapTuple tuple;
  HeapTuple new_tuple;

  tuple = search_sys_cache(cache_id, key1, key2, key3, key4);

  if (!HEAP_TUPLE_IS_VALID(tuple)) {
    return tuple;
  }
}