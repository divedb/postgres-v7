//===----------------------------------------------------------------------===//
//
// catname.h
//  POSTGRES system catalog relation name definitions.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: catname.h,v 1.18 2001/01/24 19:43:20 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_CATALOG_CATNAME_H_
#define RDBMS_CATALOG_CATNAME_H_

#define AGGREGATE_RELATION_NAME                   "pg_aggregate"
#define ACCESS_METHOD_RELATION_NAME               "pg_am"
#define ACCESS_METHOD_OPERATOR_RELATION_NAME      "pg_amop"
#define ACCESS_METHOD_PROCEDURE_RELATION_NAME     "pg_amproc"
#define ATTRIBUTE_RELATION_NAME                   "pg_attribute"
#define DATABASE_RELATION_NAME                    "pg_database"
#define DESCRIPTION_RELATION_NAME                 "pg_description"
#define GROUP_RELATION_NAME                       "pg_group"
#define INDEX_RELATION_NAME                       "pg_index"
#define INHERIT_PROCEDURE_RELATION_NAME           "pg_inheritproc"
#define INHERITS_RELATION_NAME                    "pg_inherits"
#define INHERITANCE_PRECIDENCE_LIST_RELATION_NAME "pg_ipl"
#define LANGUAGE_RELATION_NAME                    "pg_language"
#define LARGE_OBJECT_RELATION_NAME                "pg_largeobject"
#define LISTENER_RELATION_NAME                    "pg_listener"
#define LOG_RELATION_NAME                         "pg_log"
#define OPERATOR_CLASS_RELATION_NAME              "pg_opclass"
#define OPERATOR_RELATION_NAME                    "pg_operator"
#define PROCEDURE_RELATION_NAME                   "pg_proc"
#define RELATION_RELATION_NAME                    "pg_class"
#define REWRITE_RELATION_NAME                     "pg_rewrite"
#define SHADOW_RELATION_NAME                      "pg_shadow"
#define STATISTIC_RELATION_NAME                   "pg_statistic"
#define TYPE_RELATION_NAME                        "pg_type"
#define VARIABLE_RELATION_NAME                    "pg_variable"
#define VERSION_RELATION_NAME                     "pg_version"
#define ATTR_DEFAULT_RELATION_NAME                "pg_attrdef"
#define REL_CHECK_RELATION_NAME                   "pg_relcheck"
#define TRIGGER_RELATION_NAME                     "pg_trigger"

extern char* SharedSystemRelationNames[];

#endif  // RDBMS_CATALOG_CATNAME_H_
