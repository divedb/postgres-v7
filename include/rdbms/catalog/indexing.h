// =========================================================================
//
// indexing.h
//  This include provides some definitions to support indexing
//  on system catalogs
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: indexing.h,v 1.37 2000/04/12 17:16:27 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_CATALOG_INDEXING_H_
#define RDBMS_CATALOG_INDEXING_H_

// Some definitions for indices on pg_attribute.
#define NUM_PG_AGGREGATE_INDICES   1
#define NUM_PG_AM_INDICES          1
#define NUM_PG_AMOP_INDICES        2
#define NUM_PG_ATTR_INDICES        2
#define NUM_PG_ATTR_DEF_INDICES    1
#define NUM_PG_CLASS_INDICES       2
#define NUM_PG_DESCRIPTION_INDICES 1
#define NUM_PG_GROUP_INDICES       2
#define NUM_PG_INDEX_INDICES       1
#define NUM_PG_INHERITS_INDICES    1
#define NUM_PG_LANGUAGE_INDICES    2
#define NUM_PG_LISTENER_INDICES    1
#define NUM_PG_OP_CLASS_INDICES    2
#define NUM_PG_OPERATOR_INDICES    2
#define NUM_PG_PROC_INDICES        2
#define NUM_PG_REL_CHECK_INDICES   1
#define NUM_PG_REWRITE_INDICES     2
#define NUM_PG_SHADOW_INDICES      2
#define NUM_PG_STATISTIC_INDICES   1
#define NUM_PG_TRIGGER_INDICES     3
#define NUM_PG_TYPE_INDICES        2

// Names of indices on system catalogs.
#define AMOP_OPID_INDEX              "pg_amop_opid_index"
#define AMOP_STRATEGY_INDEX          "pg_amop_strategy_index"
#define AGGREGATE_NAME_TYPE_INDEX    "pg_aggregate_name_type_index"
#define AM_NAME_INDEX                "pg_am_name_index"
#define ATTRDEF_ADRELID_INDEX        "pg_attrdef_adrelid_index"
#define ATTRIBUTE_RELID_ATTNAM_INDEX "pg_attribute_relid_attnam_index"
#define ATTRIBUTE_RELID_ATTNUM_INDEX "pg_attribute_relid_attnum_index"
#define CLASS_RELNAME_INDEX          "pg_class_relname_index"
#define CLASS_OID_INDEX              "pg_class_oid_index"
#define DESCRIPTION_OBJOID_INDEX     "pg_description_objoid_index"
#define GROUP_NAME_INDEX             "pg_group_name_index"
#define GROUP_SYSID_INDEX            "pg_group_sysid_index"
#define INDEX_INDEXRELID_INDEX       "pg_index_indexrelid_index"
#define INHERITS_RELID_SEQNO_INDEX   "pg_inherits_relid_seqno_index"
#define LANGUAGE_NAME_INDEX          "pg_language_name_index"
#define LANGUAGE_OID_INDEX           "pg_language_oid_index"
#define LISTENER_RELNAME_PID_INDEX   "pg_listener_relname_pid_index"
#define OPCLASS_DEFTYPE_INDEX        "pg_opclass_deftype_index"
#define OPCLASS_NAME_INDEX           "pg_opclass_name_index"
#define OPERATOR_OPRNAME_L_R_K_INDEX "pg_operator_oprname_l_r_k_index"
#define OPERATOR_OID_INDEX           "pg_operator_oid_index"
#define PROC_PRONAME_NARG_TYPE_INDEX "pg_proc_proname_narg_type_index"
#define PROC_OID_INDEX               "pg_proc_oid_index"
#define RELCHECK_RCRELID_INDEX       "pg_relcheck_rcrelid_index"
#define REWRITE_OID_INDEX            "pg_rewrite_oid_index"
#define REWRITE_RULENAME_INDEX       "pg_rewrite_rulename_index"
#define SHADOW_NAME_INDEX            "pg_shadow_name_index"
#define SHADOW_SYSID_INDEX           "pg_shadow_sysid_index"
#define STATISTIC_RELID_ATT_INDEX    "pg_statistic_relid_att_index"
#define TRIGGER_TGCONSTRNAME_INDEX   "pg_trigger_tgconstrname_index"
#define TRIGGER_TGCONSTRRELID_INDEX  "pg_trigger_tgconstrrelid_index"
#define TRIGGER_TGRELID_INDEX        "pg_trigger_tgrelid_index"
#define TYPE_TYPNAME_INDEX           "pg_type_typname_index"
#define TYPE_OID_INDEX               "pg_type_oid_index"

extern char* NamePgAggregateIndices[];
extern char* NamePgAmIndices[];
extern char* NamePgAmopIndices[];
extern char* NamePgAttrIndices[];
extern char* NamePgAttrdefIndices[];
extern char* NamePgClassIndices[];
extern char* NamePgDescriptionIndices[];
extern char* NamePgGroupIndices[];
extern char* NamePgIndexIndices[];
extern char* NamePgInheritsIndices[];
extern char* NamePgLanguageIndices[];
extern char* NamePgListenerIndices[];
extern char* NamePgOpclassIndices[];
extern char* NamePgOperatorIndices[];
extern char* NamePgProcIndices[];
extern char* NamePgRelcheckIndices[];
extern char* NamePgRewriteIndices[];
extern char* NamePgShadowIndices[];
extern char* NamePgStatisticIndices[];
extern char* NamePgTriggerIndices[];
extern char* NamePgTypeIndices[];

extern char* IndexedCatalogNames[];

// TODO(gc): fix this.

#endif  // RDBMS_CATALOG_INDEXING_H_
