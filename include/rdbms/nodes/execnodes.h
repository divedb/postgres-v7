//===----------------------------------------------------------------------===//
//
// execnodes.h
//  Definitions for executor state nodes.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: execnodes.h,v 1.57.2.1 2001/05/15 00:34:02 tgl Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_NODES_EXEC_NODES_H_
#define RDBMS_NODES_EXEC_NODES_H_

#include "rdbms/access/attnum.h"
#include "rdbms/nodes/nodes.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/fmgr.h"

//	  IndexInfo information
//
//		this class holds the information needed to construct new index
//		entries for a particular index.  Used for both index_build and
//		retail creation of index entries.
//
//		NumIndexAttrs		number of columns in this index
//							(1 if a func. index, else same as NumKeyAttrs)
//		NumKeyAttrs			number of key attributes for this index
//							(ie, number of attrs from underlying relation)
//		KeyAttrNumbers		underlying-rel attribute numbers used as keys
//		Predicate			partial-index predicate, or NULL if none
//		FuncOid				OID of function, or InvalidOid if not f. index
//		FuncInfo			fmgr lookup data for function, if FuncOid valid
//		Unique				is it a unique index?
typedef struct IndexInfo {
  NodeTag type;
  int ii_num_index_attrs;
  int ii_num_key_attrs;
  AttrNumber ii_key_attr_numbers[INDEX_MAX_KEYS];
  Node* ii_predicate;
  Oid ii_func_oid;
  FmgrInfo ii_func_info;
  bool ii_unique;
} IndexInfo;

// TODO(gc): fix this.

#endif  // RDBMS_NODES_EXEC_NODES_H_
