//===----------------------------------------------------------------------===//
//
// indexing.h
//  This file provides some definitions to support indexing
//  on system catalogs
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: indexing.h,v 1.48 2001/03/22 04:00:36 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_CATALOG_INDEXING_H_
#define RDBMS_CATALOG_INDEXING_H_

#include "rdbms/access/htup.h"
#include "rdbms/utils/rel.h"

// Number of indices that exist for each system catalog
#define NUM_PG_AGGREGATE_INDICES   1
#define NUM_PG_AM_INDICES          1
#define NUM_PG_AMOP_INDICES        2
#define NUM_PG_ATTR_INDICES        2
#define NUM_PG_ATTRDEF_INDICES     1
#define NUM_PG_CLASS_INDICES       2
#define NUM_PG_DESCRIPTION_INDICES 1
#define NUM_PG_GROUP_INDICES       2
#define NUM_PG_INDEX_INDICES       2
#define NUM_PG_INHERITS_INDICES    1
#define NUM_PG_LANGUAGE_INDICES    2
#define NUM_PG_LARGEOBJECT_INDICES 1
#define NUM_PG_LISTENER_INDICES    1
#define NUM_PG_OPCLASS_INDICES     2
#define NUM_PG_OPERATOR_INDICES    2
#define NUM_PG_PROC_INDICES        2
#define NUM_PG_RELCHECK_INDICES    1
#define NUM_PG_REWRITE_INDICES     2
#define NUM_PG_SHADOW_INDICES      2
#define NUM_PG_STATISTIC_INDICES   1
#define NUM_PG_TRIGGER_INDICES     3
#define NUM_PG_TYPE_INDICES        2

// Names of indices on system catalogs
#define ACCESS_METHOD_OPID_INDEX     "pg_amop_opid_index"
#define ACCESS_METHOD_STRATEGY_INDEX "pg_amop_strategy_index"
#define AGGREGATE_NAME_TYPE_INDEX    "pg_aggregate_name_type_index"
#define AM_NAME_INDEX                "pg_am_name_index"
#define ATTR_DEFAULT_INDEX           "pg_attrdef_adrelid_index"
#define ATTRIBUTE_RELID_NAME_INDEX   "pg_attribute_relid_attnam_index"
#define ATTRIBUTE_RELID_NUM_INDEX    "pg_attribute_relid_attnum_index"
#define CLASS_NAME_INDEX             "pg_class_relname_index"
#define CLASS_OID_INDEX              "pg_class_oid_index"
#define DESCRIPTION_OBJ_INDEX        "pg_description_objoid_index"
#define GROUP_NAME_INDEX             "pg_group_name_index"
#define GROUP_SYSID_INDEX            "pg_group_sysid_index"
#define INDEX_INDRELID_INDEX         "pg_index_indrelid_index"
#define INDEX_RELID_INDEX            "pg_index_indexrelid_index"
#define INHERITS_RELID_SEQNO_INDEX   "pg_inherits_relid_seqno_index"
#define LANGUAGE_NAME_INDEX          "pg_language_name_index"
#define LANGUAGE_OID_INDEX           "pg_language_oid_index"
#define LARGE_OBJECT_L_OID_P_N_INDEX "pg_largeobject_loid_pn_index"
#define LISTENER_PID_RELNAME_INDEX   "pg_listener_pid_relname_index"
#define OPCLASS_DEFTYPE_INDEX        "pg_opclass_deftype_index"
#define OPCLASS_NAME_INDEX           "pg_opclass_name_index"
#define OPERATOR_NAME_INDEX          "pg_operator_oprname_l_r_k_index"
#define OPERATOR_OID_INDEX           "pg_operator_oid_index"
#define PROCEDURE_NAME_INDEX         "pg_proc_proname_narg_type_index"
#define PROCEDURE_OID_INDEX          "pg_proc_oid_index"
#define REL_CHECK_INDEX              "pg_relcheck_rcrelid_index"
#define REWRITE_OID_INDEX            "pg_rewrite_oid_index"
#define REWRITE_RULENAME_INDEX       "pg_rewrite_rulename_index"
#define SHADOW_NAME_INDEX            "pg_shadow_name_index"
#define SHADOW_SYSID_INDEX           "pg_shadow_sysid_index"
#define STATISTIC_RELID_ATTNUM_INDEX "pg_statistic_relid_att_index"
#define TRIGGER_CONSTR_NAME_INDEX    "pg_trigger_tgconstrname_index"
#define TRIGGER_CONSTR_RELID_INDEX   "pg_trigger_tgconstrrelid_index"
#define TRIGGER_RELID_INDEX          "pg_trigger_tgrelid_index"
#define TYPE_NAME_INDEX              "pg_type_typname_index"
#define TYPE_OID_INDEX               "pg_type_oid_index"

extern char* NamePGAggregateIndices[];
extern char* NamePGAmIndices[];
extern char* NamePGAmopIndices[];
extern char* NamePGAttrIndices[];
extern char* NamePGAttrdefIndices[];
extern char* NamePGClassIndices[];
extern char* NamePGDescriptionIndices[];
extern char* NamePGGroupIndices[];
extern char* NamePGIndexIndices[];
extern char* NamePGInheritsIndices[];
extern char* NamePGLanguageIndices[];
extern char* NamePGLargeObjectIndices[];
extern char* NamePGListenerIndices[];
extern char* NamePGOpclassIndices[];
extern char* NamePGOperatorIndices[];
extern char* NamePGProcIndices[];
extern char* NamePGRelCheckIndices[];
extern char* NamePGRewriteIndices[];
extern char* NamePGShadowIndices[];
extern char* NamePGStatisticIndices[];
extern char* NamePGTriggerIndices[];
extern char* NamePGTypeIndices[];

extern char* IndexedCatalogNames[];

// indexing.c
extern void catalog_open_indices(int nindices, char** names, Relation* idescs);
extern void catalog_close_indices(int nindices, Relation* idescs);
extern void catalog_index_insert(Relation* idescs, int nindices, Relation heap_relation, HeapTuple heap_tuple);

// Canned functions for indexscans on certain system indexes.
// All index-value arguments should be passed as Datum for portability!
extern HeapTuple attribute_rel_id_num_index_scan(Relation heapRelation, Datum relid, Datum attnum);
extern HeapTuple class_name_index_scan(Relation heap_relation, Datum rel_name);
extern HeapTuple class_oid_index_scan(Relation heap_relation, Datum rel_id);

// What follows are lines processed by genbki.sh to create the statements
// the bootstrap parser will turn into DefineIndex commands.
//
// The keyword is DECLARE_INDEX every thing after that is just like in a
// normal specification of the 'define index' POSTQUEL command.

DECLARE_UNIQUE_INDEX(pg_aggregate_name_type_index on pg_aggregate using btree(aggname name_ops, aggbasetype oid_ops));
DECLARE_UNIQUE_INDEX(pg_am_name_index on pg_am using btree(amname name_ops));
DECLARE_UNIQUE_INDEX(pg_amop_opid_index on pg_amop using btree(amopclaid oid_ops, amopopr oid_ops, amopid oid_ops));
DECLARE_UNIQUE_INDEX(pg_amop_strategy_index on pg_amop using btree(amopid oid_ops, amopclaid oid_ops,
                                                                   amopstrategy int2_ops));
// This following index is not used for a cache and is not unique.
DECLARE_INDEX(pg_attrdef_adrelid_index on pg_attrdef using btree(adrelid oid_ops));
DECLARE_UNIQUE_INDEX(pg_attribute_relid_attnam_index on pg_attribute using btree(attrelid oid_ops, attname name_ops));
DECLARE_UNIQUE_INDEX(pg_attribute_relid_attnum_index on pg_attribute using btree(attrelid oid_ops, attnum int2_ops));
DECLARE_UNIQUE_INDEX(pg_class_oid_index on pg_class using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_class_relname_index on pg_class using btree(relname name_ops));
DECLARE_UNIQUE_INDEX(pg_description_objoid_index on pg_description using btree(objoid oid_ops));
DECLARE_UNIQUE_INDEX(pg_group_name_index on pg_group using btree(groname name_ops));
DECLARE_UNIQUE_INDEX(pg_group_sysid_index on pg_group using btree(grosysid int4_ops));
// This following index is not used for a cache and is not unique.
DECLARE_INDEX(pg_index_indrelid_index on pg_index using btree(indrelid oid_ops));
DECLARE_UNIQUE_INDEX(pg_index_indexrelid_index on pg_index using btree(indexrelid oid_ops));
DECLARE_UNIQUE_INDEX(pg_inherits_relid_seqno_index on pg_inherits using btree(inhrelid oid_ops, inhseqno int4_ops));
DECLARE_UNIQUE_INDEX(pg_language_name_index on pg_language using btree(lanname name_ops));
DECLARE_UNIQUE_INDEX(pg_language_oid_index on pg_language using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_largeobject_loid_pn_index on pg_largeobject using btree(loid oid_ops, pageno int4_ops));
DECLARE_UNIQUE_INDEX(pg_listener_pid_relname_index on pg_listener using btree(listenerpid int4_ops, relname name_ops));
// This column needs to allow multiple zero entries, but is in the cache.
DECLARE_INDEX(pg_opclass_deftype_index on pg_opclass using btree(opcdeftype oid_ops));
DECLARE_UNIQUE_INDEX(pg_opclass_name_index on pg_opclass using btree(opcname name_ops));
DECLARE_UNIQUE_INDEX(pg_operator_oid_index on pg_operator using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_operator_oprname_l_r_k_index on pg_operator using btree(oprname name_ops, oprleft oid_ops,
                                                                                oprright oid_ops, oprkind char_ops));
DECLARE_UNIQUE_INDEX(pg_proc_oid_index on pg_proc using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_proc_proname_narg_type_index on pg_proc using btree(proname name_ops, pronargs int2_ops,
                                                                            proargtypes oidvector_ops));
// This following index is not used for a cache and is not unique.
DECLARE_INDEX(pg_relcheck_rcrelid_index on pg_relcheck using btree(rcrelid oid_ops));
DECLARE_UNIQUE_INDEX(pg_rewrite_oid_index on pg_rewrite using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_rewrite_rulename_index on pg_rewrite using btree(rulename name_ops));

// xDECLARE_UNIQUE_INDEX(pg_shadow_name_index on pg_shadow using btree(usename name_ops));
// xDECLARE_UNIQUE_INDEX(pg_shadow_sysid_index on pg_shadow using btree(usesysid int4_ops));
DECLARE_INDEX(pg_statistic_relid_att_index on pg_statistic using btree(starelid oid_ops, staattnum int2_ops));
DECLARE_INDEX(pg_trigger_tgconstrname_index on pg_trigger using btree(tgconstrname name_ops));
DECLARE_INDEX(pg_trigger_tgconstrrelid_index on pg_trigger using btree(tgconstrrelid oid_ops));
DECLARE_INDEX(pg_trigger_tgrelid_index on pg_trigger using btree(tgrelid oid_ops));
DECLARE_UNIQUE_INDEX(pg_type_oid_index on pg_type using btree(oid oid_ops));
DECLARE_UNIQUE_INDEX(pg_type_typname_index on pg_type using btree(typname name_ops));

// Now build indices in the initialization scripts.
BUILD_INDICES

#endif  // RDBMS_CATALOG_INDEXING_H_
