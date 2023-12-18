// =========================================================================
//
// rel.h
//  POSTGRES relation descriptor (a/k/a relcache entry) definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: rel.h,v 1.36 2000/04/12 17:16:55 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_REL_H_
#define RDBMS_UTILS_REL_H_

#include "rdbms/access/strat.h"
#include "rdbms/access/tupdesc.h"
#include "rdbms/catalog/pg_am.h"
#include "rdbms/catalog/pg_class.h"
#include "rdbms/postgres.h"
#include "rdbms/rewrite/prs2lock.h"
#include "rdbms/storage/fd.h"

// Added to prevent circular dependency.  bjm 1999/11/15
char* get_temp_rel_by_physicalname(const char* relname);

// LockRelId and LockInfo really belong to lmgr.h, but it's more convenient
// to declare them here so we can have a LockInfoData field in a Relation.
typedef struct LockRelId {
  Oid rel_id;  // A relation identifier.
  Oid db_id;   // A database identifier.
} LockRelId;

typedef struct LockInfoData {
  LockRelId lock_rel_id;
} LockInfoData;

typedef LockInfoData* LockInfo;

// Likewise, this struct really belongs to trigger.h, but for convenience we put it here.
typedef struct Trigger {
  Oid tg_oid;
  char* tg_name;
  Oid tg_foid;
  FmgrInfo tg_func;
  int16 tg_type;
  bool tg_enabled;
  bool tg_is_constraint;
  bool tg_deferrable;
  bool tg_init_deferred;
  int16 tg_nargs;
  int16 tg_attr[FUNC_MAX_ARGS];
  char** tg_args;
} Trigger;

typedef struct TriggerDesc {
  // Index data to identify which triggers are which.
  uint16 n_before_statement[4];
  uint16 n_before_row[4];
  uint16 n_after_row[4];
  uint16 n_after_statement[4];
  Trigger** tg_before_statement[4];
  Trigger** tg_before_row[4];
  Trigger** tg_after_row[4];
  Trigger** tg_after_statement[4];

  // The actual array of triggers is here.
  Trigger* triggers;
  int num_triggers;
} TriggerDesc;

// Here are the contents of a relation cache entry.
typedef struct RelationData {
  File rd_fd;                 // Open file descriptor.
  int rd_nblocks;             // Number of blocks in relation.
  uint16 rd_ref_cnt;          // Reference count.
  bool rd_my_xact_only;       // Relation uses the local buffer manager.
  bool rd_is_nailed;          // Relation is nailed in cache.
  bool rd_is_no_name;         // Relation has no name.
  bool rd_unlinked;           // Relation already unlinked or not created yet.
  Form_pg_am rd_am;           // AM tuple.
  Form_pg_class rd_rel;       // RELATION tuple.
  LockInfoData rd_lock_info;  // Lock manager's info for locking relation.
  TupleDesc rd_att;           // Tuple descriptor.
  RuleLock* rd_rules;         // Rewrite rules.
  IndexStrategy rd_istrat;
  RegProcedure* rd_support;
  TriggerDesc* trig_desc;  // Trigger info, or NULL if relation has none.
} RelationData;

typedef RelationData* Relation;

// RelationPtr is used in the executor to support index scans
// where we have to keep track of several index relations in an
// array.	-cim 9/10/89
typedef Relation* RelationPtr;

#define RELATION_IS_VALID(relation)                   POINTER_IS_VALID(relation)
#define INVALID_RELATION                              NULL
#define RELATION_HAS_REFERENCE_COUNT_ZERO(relation)   (relation->rd_refcnt == 0)
#define RELATION_SET_REFERENCE_COUNT(relation, count) ((relation)->rd_refcnt = (count))
#define RELATION_INCREMENT_REFERENCE_COUNT(relation)  ((relation)->rd_refcnt += 1)
#define RELATION_DECREMENT_REFERENCE_COUNT(relation)  (assert((relation)->rd_refcnt > 0), (relation)->rd_refcnt -= 1)
#define RELATION_GET_FORM(relation)                   ((relation)->rd_rel)
#define RELATION_GET_REL_ID(relation)                 ((relation)->rd_id)
#define RELATION_GET_FILE(relation)                   ((relation)->rd_fd)
#define RELATION_GET_RELATION_NAME(relation)                                                    \
  (strncmp(RELATION_GET_PHYSICAL_RELATION_NAME(relation), "pg_temp.", strlen("pg_temp.")) != 0) \
      ? RELATION_GET_PHYSICAL_RELATION_NAME(relation)                                           \
      : get_temp_rel_by_physicalname(RELATION_GET_PHYSICAL_RELATION_NAME(relation))
#define RELATION_GET_PHYSICAL_RELATION_NAME(relation) (NAME_STR((relation)->rd_rel->relname))
#define RELATION_GET_NUMBER_OF_ATTRIBUTES(relation)   ((relation)->rd_rel->relnatts)
#define RELATION_GET_DESCR(relation)                  ((relation)->rd_att)
#define RELATION_GET_INDEX_STRATEGY(relation)         ((relation)->rd_istrat)

void relation_set_index_support(Relation relation, IndexStrategy strategy, RegProcedure* support);

#endif  // RDBMS_UTILS_REL_H_
