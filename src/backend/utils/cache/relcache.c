#include "rdbms/utils/relcache.h"

#include "rdbms/access/heapam.h"
#include "rdbms/catalog/pg_attribute.h"
#include "rdbms/catalog/pg_log.h"
#include "rdbms/catalog/pg_proc.h"
#include "rdbms/catalog/pg_type.h"
#include "rdbms/catalog/pg_variable.h"
#include "rdbms/nodes/pg_list.h"
#include "rdbms/utils/builtins.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/hsearch.h"

static FormData_pg_attribute Desc_pg_class[Natts_pg_class] = {Schema_pg_class};
static FormData_pg_attribute Desc_pg_attribute[Natts_pg_attribute] = {
    Schema_pg_attribute};
static FormData_pg_attribute Desc_pg_proc[Natts_pg_proc] = {Schema_pg_proc};
static FormData_pg_attribute Desc_pg_type[Natts_pg_type] = {Schema_pg_type};
static FormData_pg_attribute Desc_pg_variable[Natts_pg_variable] = {
    Schema_pg_variable};
static FormData_pg_attribute Desc_pg_log[Natts_pg_log] = {Schema_pg_log};

// Hash tables that index the relation cache
//
// Relations are looked up two ways, by name and by id,
// thus there are two hash tables for referencing them.
static HashTable* RelationNameCache;
static HashTable* RelationIdCache;

// Bufmgr uses RelFileNode for lookup. Actually, I would like to do
// not pass Relation to bufmgr & beyond at all and keep some cache
// in smgr, but no time to do it right way now. -- vadim 10/22/2000
static HashTable* RelationNodeCache;

// Relations created during this transaction. We need to keep track of these.
static List* NewlyCreatedRelns = NULL;

// This flag is false until we have prepared the critical relcache entries
// that are needed to do indexscans on the tables read by relcache building.
static bool CriticalRelcachesBuilt = false;

// RelationBuildDescInfo exists so code can be shared
// between RelationIdGetRelation() and RelationNameGetRelation()
typedef struct RelationBuildDescInfo {
  int info_type;  // Lookup by id or by name
#define INFO_RELID   1
#define INFO_RELNAME 2
  union {
    Oid info_id;      // Relation object id
    char* info_name;  // Relation name
  } i;
} RelationBuildDescInfo;

typedef struct RelNameCacheEnt {
  NameData rel_name;
  Relation rel_desc;
} RelNameCacheEnt;

typedef struct RelIdCacheEnt {
  Oid rel_oid;
  Relation rel_desc;
} RelIdCacheEnt;

typedef struct RelNodeCacheEnt {
  RelFileNode rel_node;
  Relation rel_desc;
} RelNodeCacheEnt;

// Macros to manipulate name cache and id cache.
#define RELATION_CACHE_INSERT(relation)                                      \
  do {                                                                       \
    RelIdCacheEnt* idhentry;                                                 \
    RelNameCacheEnt* namehentry;                                             \
    char* relname;                                                           \
    RelNodeCacheEnt* nodentry;                                               \
    bool found;                                                              \
    relname = RElATION_GET_PHYSICAL_RELATION_NAME(relation);                 \
    namehentry = (RelNameCacheEnt*)hash_search(RelationNameCache, relname,   \
                                               HASH_ENTER, &found);          \
    if (namehentry == NULL)                                                  \
      elog(FATAL, "can't insert into relation descriptor cache");            \
    if (found && !IS_BOOTSTRAP_PROCESSING_MODE())                            \
      /* used to give notice -- now just keep quiet */;                      \
    namehentry->rel_desc = relation;                                         \
    idhentry = (RelIdCacheEnt*)hash_search(                                  \
        RelationIdCache, (char*)&(relation->rd_id), HASH_ENTER, &found);     \
    if (idhentry == NULL)                                                    \
      elog(FATAL, "can't insert into relation descriptor cache");            \
    if (found && !IS_BOOTSTRAP_PROCESSING_MODE())                            \
      /* used to give notice -- now just keep quiet */;                      \
    idhentry->rel_desc = relation;                                           \
    nodentry = (RelNodeCacheEnt*)hash_search(                                \
        RelationNodeCache, (char*)&(relation->rd_node), HASH_ENTER, &found); \
    if (nodentry == NULL)                                                    \
      elog(FATAL, "can't insert into relation descriptor cache");            \
    if (found && !IS_BOOTSTRAP_PROCESSING_MODE())                            \
      /* used to give notice -- now just keep quiet */;                      \
    nodentry->rel_desc = relation;                                           \
  } while (0)

#define RELATION_NAME_CACHE_LOOK_UP(name, relation)                        \
  do {                                                                     \
    RelNameCacheEnt* hentry;                                               \
    bool found;                                                            \
    hentry = (RelNameCacheEnt*)hash_search(RelationNameCache, (char*)name, \
                                           HASH_FIND, &found);             \
    if (hentry == NULL) elog(FATAL, "error in CACHE");                     \
    if (found)                                                             \
      relation = hentry->rel_desc;                                         \
    else                                                                   \
      relation = NULL;                                                     \
  } while (0)

#define RELATION_ID_CACHE_LOOK_UP(id, relation)                         \
  do {                                                                  \
    RelIdCacheEnt* hentry;                                              \
    bool found;                                                         \
    hentry = (RelIdCacheEnt*)hash_search(RelationIdCache, (char*)&(id), \
                                         HASH_FIND, &found);            \
    if (hentry == NULL) elog(FATAL, "error in CACHE");                  \
    if (found)                                                          \
      relation = hentry->reldesc;                                       \
    else                                                                \
      relation = NULL;                                                  \
  } while (0)

#define RELATION_NODE_CACHE_LOOK_UP(node, relation)                           \
  do {                                                                        \
    RelNodeCacheEnt* hentry;                                                  \
    bool found;                                                               \
    hentry = (RelNodeCacheEnt*)hash_search(RelationNodeCache, (char*)&(node), \
                                           HASH_FIND, &found);                \
    if (hentry == NULL) elog(FATAL, "error in CACHE");                        \
    if (found)                                                                \
      relation = hentry->reldesc;                                             \
    else                                                                      \
      relation = NULL;                                                        \
  } while (0)

#define RELATION_CACHE_DELETE(relation)                                       \
  do {                                                                        \
    RelNameCacheEnt* namehentry;                                              \
    RelIdCacheEnt* idhentry;                                                  \
    char* relname;                                                            \
    RelNodeCacheEnt* nodentry;                                                \
    bool found;                                                               \
    relname = RELATION_GET_PHYSICAL_RELATION_NAME(relation);                  \
    namehentry = (RelNameCacheEnt*)hash_search(RelationNameCache, relname,    \
                                               HASH_REMOVE, &found);          \
    if (namehentry == NULL)                                                   \
      elog(FATAL, "can't delete from relation descriptor cache");             \
    if (!found)                                                               \
      elog(NOTICE, "trying to delete a reldesc that does not exist.");        \
    idhentry = (RelIdCacheEnt*)hash_search(                                   \
        RelationIdCache, (char*)&(relation->rd_id), HASH_REMOVE, &found);     \
    if (idhentry == NULL)                                                     \
      elog(FATAL, "can't delete from relation descriptor cache");             \
    if (!found)                                                               \
      elog(NOTICE, "trying to delete a reldesc that does not exist.");        \
    nodentry = (RelNodeCacheEnt*)hash_search(                                 \
        RelationNodeCache, (char*)&(relation->rd_node), HASH_REMOVE, &found); \
    if (nodentry == NULL)                                                     \
      elog(FATAL, "can't delete from relation descriptor cache");             \
    if (!found)                                                               \
      elog(NOTICE, "trying to delete a reldesc that does not exist.");        \
  } while (0)

static void relation_clear_relation(Relation relation, bool rebuild_it);
static void relation_flush_relation(Relation relation);
static Relation relation_name_cache_get_relation(const char* relation_name);
static void relation_cache_invalidate_walker(Relation* relationPtr,
                                             Datum listp);
static void relation_cache_abort_walker(Relation* relation_ptr, Datum dummy);
static void init_irels(void);
static void write_irels(void);

static void formrdesc(char* relationName, int natts,
                      FormData_pg_attribute* att);
static void fixrdesc(char* relationName);

static HeapTuple ScanPgRelation(RelationBuildDescInfo buildinfo);
static HeapTuple scan_pg_rel_seq(RelationBuildDescInfo buildinfo);
static HeapTuple scan_pg_rel_ind(RelationBuildDescInfo buildinfo);
static Relation AllocateRelationDesc(Relation relation, Form_pg_class relp);
static void RelationBuildTupleDesc(RelationBuildDescInfo buildinfo,
                                   Relation relation);
static void build_tupdesc_seq(RelationBuildDescInfo buildinfo,
                              Relation relation);
static void build_tupdesc_ind(RelationBuildDescInfo buildinfo,
                              Relation relation);
static Relation RelationBuildDesc(RelationBuildDescInfo buildinfo,
                                  Relation oldrelation);
static void IndexedAccessMethodInitialize(Relation relation);
static void AttrDefaultFetch(Relation relation);
static void RelCheckFetch(Relation relation);
static List* insert_ordered_oid(List* list, Oid datum);

// RelationIdGetRelation
//
// Lookup a reldesc by OID; make one if not already in cache.
//
// NB: relation ref count is incremented, or set to 1 if new entry.
// Caller should eventually decrement count.  (Usually,
// that happens by calling RelationClose().)
Relation relation_id_get_relation(Oid relation_id) {
  Relation rd;
  RelationBuildDescInfo build_info;

  INCR_HEAP_ACCESS_STAT(local_relation_id_get_relation);
  INCR_HEAP_ACCESS_STAT(global_relation_id_get_relation);

  rd = relation_id_cache_get_relation(relation_id);

  if (RELATION_IS_VALID(rd)) {
    return rd;
  }
}

static void relation_clear_relation(Relation relation, bool rebuild_it);
static void relation_flush_relation(Relation relation);
static Relation relation_name_cache_get_relation(const char* relation_name) {
  Relation rd;
  NameData name;

  // Make sure that the name key used for hash lookup is properly
  // null-padded.
  name_strcpy(&name, relation_name);
  RELATION_NAME_CACHE_LOOK_UP(NAME_STR(name), rd);

  if (RELATION_IS_VALID(rd)) {
    // Re-open files if necessary.
    if (rd->rd_fd == -1 && rd->rd_rel->relkind != RELKIND_VIEW) {
      rd->rd_fd = smgr_open(DEFAULT_SMGR, rd, false);
    }

    RELATION_INCREMENT_REFERENCE_COUNT(rd);
  }

  return rd;
}

static void relation_cache_invalidate_walker(Relation* relationPtr,
                                             Datum listp);
static void relation_cache_abort_walker(Relation* relation_ptr, Datum dummy);
static void init_irels(void);
static void write_irels(void);

static void formrdesc(char* relationName, int natts,
                      FormData_pg_attribute* att);
static void fixrdesc(char* relationName);
