// =========================================================================
//
// bootstrap.h
//  include file for the bootstrapping code
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: bootstrap.h,v 1.17 2000/01/26 05:57:53 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_BOOTSTRAP_BOOTSTRAP_H_
#define RDBMS_BOOTSTRAP_BOOTSTRAP_H_

#include "rdbms/access/attnum.h"
#include "rdbms/access/func_index.h"
#include "rdbms/catalog/pg_attribute.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/rel.h"

#define MAX_ATTR 40  // Max number of attributes in a relation.

typedef struct HashNode {
  int str_num;  // Index into string table.
  struct HashNode* next;
} HashNode;

#define EMIT_PROMPT printf("> ")

extern Relation RelDesc;
extern Form_pg_attribute AttrTypes[MAX_ATTR];
extern int NumAttr;
extern int DebugMode;

int bootstrap_main(int argc, char* argv[]);
void index_register(char* heap, char* index, int natts, AttrNumber* attnos, uint16 nparams, Datum* params,
                    FuncIndexInfo* finfo, PredInfo* pred_info);

void err_out();
void insert_one_tuple(Oid object_id);

#endif  // RDBMS_BOOTSTRAP_BOOTSTRAP_H_
