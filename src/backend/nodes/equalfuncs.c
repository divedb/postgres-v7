// =========================================================================
//
// equalfuncs.c
//  equality functions to compare node trees
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/nodes/equalfuncs.c,v 1.66 2000/04/12 17:15:16 momjian Exp $
//
// =========================================================================

#include "rdbms/nodes/pg_list.h"
#include "rdbms/nodes/primnodes.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/datum.h"

static bool equali(List* a, List* b);

static bool equal_resdom(Resdom* a, Resdom* b) {
  if (a->res_no != b->res_no) {
    return false;
  }

  if (a->res_type != b->res_type) {
    return false;
  }

  if (a->res_type_mod != b->res_type_mod) {
    return false;
  }

  if (a->res_name && b->res_name) {
    if (strcmp(a->res_name, b->res_name) != 0) {
      return false;
    }
  } else {
    // Must both be null to be equal.
    if (a->res_name != b->res_name) {
      return false;
    }
  }

  if (a->res_sort_group_ref != b->res_sort_group_ref) {
    return false;
  }

  if (a->res_key != b->res_key) {
    return false;
  }

  if (a->res_key_op != b->res_key_op) {
    return false;
  }

  // We ignore resjunk flag ... is this correct?
  return true;
}

static bool euqal_fjoin(Fjoin* a, Fjoin* b) {
  int nodes_count;

  if (a->fj_initialized != b->fj_initialized) {
    return false;
  }

  if (a->fj_n_nodes != b->fj_n_nodes) {
    return false;
  }

  if (!equal(a->fj_inner_node, b->fj_inner_node)) {
    return false;
  }

  nodes_count = a->fj_n_nodes;

  if (memcmp(a->fj_results, b->fj_results, nodes_count * sizeof(Datum)) != 0) {
    return false;
  }

  if (memcmp(a->fj_always_done, a->fj_always_done, nodes_count * sizeof(bool)) != 0) {
    return false;
  }

  return true;
}

static bool equal_expr(Expr* a, Expr* b) {
  // We do not examine typeOid, since the optimizer often doesn't bother
  // to set it in created nodes, and it is logically a derivative of the
  // oper field anyway.
  if (a->op_type != b->op_type) {
    return false;
  }

  if (!equal(a->oper, b->oper)) {
    return false;
  }

  if (!equal(a->args, b->args)) {
    return false;
  }

  return true;
}