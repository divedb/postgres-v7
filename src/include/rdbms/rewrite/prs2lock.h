// =========================================================================
//
// prs2lock.h
//  data structures for POSTGRES Rule System II (rewrite rules only)
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: prs2lock.h,v 1.11 2000/01/26 05:58:30 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_REWRITE_PRS_2_LOCK_H_
#define RDBMS_REWRITE_PRS_2_LOCK_H_

#include "rdbms/access/attnum.h"
#include "rdbms/nodes/pg_list.h"

// RewriteRule -
//  Holds a info for a rewrite rule.
typedef struct RewriteRule {
  Oid rule_id;
  CmdType event;
  AttrNumber attr_no;
  Node* qual;
  List* actions;
  bool is_instead;
} RewriteRule;

// RuleLock -
//	All rules that apply to a particular relation. Even though we only
//	have the rewrite rule system left and these are not really "locks",
//	the name is kept for historical reasons.
typedef struct RuleLock {
  int num_locks;
  RewriteRule** rules;
} RuleLock;

#endif  // RDBMS_REWRITE_PRS_2_LOCK_H_
