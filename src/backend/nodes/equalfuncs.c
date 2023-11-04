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

#include "rdbms/nodes/params.h"
#include "rdbms/nodes/parsenodes.h"
#include "rdbms/nodes/pg_list.h"
#include "rdbms/nodes/primnodes.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/datum.h"
#include "rdbms/utils/elog.h"

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

static bool equal_attr(Attr* a, Attr* b) {
  if (strcmp(a->rel_name, b->rel_name) != 0) {
    return false;
  }

  if (!equal(a->attrs, b->attrs)) {
    return false;
  }

  return true;
}

static bool equal_var(Var* a, Var* b) {
  if (a->varno != b->varno) {
    return false;
  }

  if (a->var_attno != b->var_attno) {
    return false;
  }

  if (a->var_type != b->var_type) {
    return false;
  }

  if (a->var_type_mod != b->var_type_mod) {
    return false;
  }

  if (a->var_levels_up != b->var_levels_up) {
    return false;
  }

  if (a->varno_old != b->varno_old) {
    return false;
  }

  if (a->var_oattno != b->var_oattno) {
    return false;
  }

  return true;
}

static bool equal_oper(Oper* a, Oper* b) {
  if (a->op_no != b->op_no) {
    return false;
  }

  if (a->op_result_type != b->op_result_type) {
    return false;
  }

  // We do not examine opid, opsize, or op_fcache, since these are
  // logically derived from opno, and they may not be set yet depending
  // on how far along the node is in the parse/plan pipeline.
  //
  // It's probably not really necessary to check opresulttype either...
  return true;
}

static bool equal_const(Const* a, Const* b) {
  if (a->const_type != b->const_type) {
    return false;
  }

  if (a->const_len != b->const_len) {
    return false;
  }

  if (a->const_is_null != b->const_is_null) {
    return false;
  }

  if (a->const_by_val != b->const_by_val) {
    return false;
  }

  // XXX What about constisset and constiscast?

  // We treat all NULL constants of the same type as equal. Someday this
  // might need to change?  But datumIsEqual doesn't work on nulls,
  // so...
  if (a->const_is_null) {
    return true;
  }

  return datum_is_equal(a->const_value, b->const_value, a->const_type, a->const_by_val, a->const_len);
}

static bool equal_param(Param* a, Param* b) {
  if (a->param_kind != b->param_kind) {
    return false;
  }

  if (a->param_type != b->param_type) {
    return false;
  }

  if (!equal(a->param_tlist, b->param_tlist)) {
    return false;
  }

  switch (a->param_kind) {
    case PARAM_NAMED:

    case PARAM_NEW:

    case PARAM_OLD:
      if (strcmp(a->param_name, b->param_name) != 0) {
        return false;
      }
      break;

    case PARAM_NUM:

    case PARAM_EXEC:
      if (a->param_id != b->param_id) {
        return false;
      }
      break;

    case PARAM_INVALID:
      // XXX: Hmmm... What are we supposed to return in this case ??
      return true;
    default:
      elog(ERROR, "%s: invalid param kind value: %d.", __func__, a->param_kind);
  }

  return true;
}

static bool equal_func(Func* a, Func* b) {
  if (a->func_id != b->func_id) {
    return false;
  }

  if (a->func_type != b->func_type) {
    return false;
  }

  if (a->func_is_index != b->func_is_index) {
    return false;
  }

  if (a->func_size != b->func_size) {
    return false;
  }

  // Note we do not look at func_fcache.
  if (!equal(a->func_tlist, b->func_tlist)) {
    return false;
  }

  if (!euqal(a->func_planlist, b->func_planlist)) {
    return false;
  }

  return true;
}

static bool euqal_aggref(Aggref* a, Aggref* b) {
  if (strcmp(a->agg_name, b->agg_name) != 0) {
    return false;
  }

  if (a->base_type != b->base_type) {
    return false;
  }

  if (a->agg_type != b->agg_type) {
    return false;
  }

  if (!equal(a->target, b->target)) {
    return false;
  }

  if (a->use_nulls != b->use_nulls) {
    return false;
  }

  if (a->agg_star != b->agg_star) {
    return false;
  }

  if (a->agg_distinct != b->agg_distinct) {
    return false;
  }

  // Ignore aggno, which is only a private field for the executor .
  return true;
}

static bool euqal_sublink(SubLink* a, SubLink* b) {
  if (a->sublink_type != b->sublink_type) {
    return false;
  }

  if (a->use_or != b->use_or) {
    return false;
  }

  if (!equal(a->lefthand, b->lefthand)) {
    return false;
  }

  if (!equal(a->oper, b->oper)) {
    return false;
  }

  if (!equal(a->subselect, b->subselect)) {
    return false;
  }

  return true;
}

static bool equal_relabel_type(RelabelType* a, RelabelType* b) {
  if (!equal(a->arg, b->arg)) {
    return false;
  }

  if (a->result_type != b->result_type) {
    return false;
  }

  if (a->result_type_mod != b->result_type_mod) {
    return false;
  }

  return true;
}

static bool equal_array(Array* a, Array* b) {
  if (a->array_elem_type != b->array_elem_type) {
    return false;
  }

  // We need not check arrayelemlength, arrayelembyval if types match.
  if (a->array_ndim != b->array_ndim) {
    return false;
  }

  // XXX shouldn't we be checking all indices???
  if (a->array_low.indx[0] != b->array_low.indx[0]) {
    return false;
  }

  if (a->array_high.indx[0] != b->array_high.indx[0]) {
    return false;
  }

  if (a->array_len != b->array_len) {
    return false;
  }

  return true;
}

static bool equal_array_ref(ArrayRef* a, ArrayRef* b) {
  if (a->ref_elem_type != b->ref_elem_type) {
    return false;
  }

  if (a->ref_attr_length != b->ref_attr_length) {
    return false;
  }

  if (a->ref_elem_length != b->ref_elem_length) {
    return false;
  }

  if (a->ref_elem_by_val != b->ref_elem_by_val) {
    return false;
  }

  if (!equal(a->ref_upper_ind_expr, b->ref_upper_ind_expr)) {
    return false;
  }

  if (!equal(a->ref_lower_ind_expr, b->ref_lower_ind_expr)) {
    return false;
  }

  if (!equal(a->ref_expr, b->ref_expr)) {
    return false;
  }

  return equal(a->ref_assgn_expr, b->ref_assgn_expr);
}

// Stuff from relation.h.
static bool equal_rel_opt_info(RelOptInfo* a, RelOptInfo* b) {}