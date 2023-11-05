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
#include "rdbms/nodes/relation.h"
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

static bool equal_fjoin(Fjoin* a, Fjoin* b) {
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

  if (!equal(a->func_planlist, b->func_planlist)) {
    return false;
  }

  return true;
}

static bool equal_aggref(Aggref* a, Aggref* b) {
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

static bool equal_sublink(SubLink* a, SubLink* b) {
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
static bool equal_rel_opt_info(RelOptInfo* a, RelOptInfo* b) {
  // We treat RelOptInfos as equal if they refer to the same base rels
  // joined in the same order.  Is this sufficient?
  return equali(a->relids, b->relids);
}

static bool equal_index_opt_info(IndexOptInfo* a, IndexOptInfo* b) {
  // We treat IndexOptInfos as equal if they refer to the same index. Is
  // this sufficient?
  if (a->index_oid != b->index_oid) {
    return false;
  }

  return true;
}

static bool equal_path_key_item(PathKeyItem* a, PathKeyItem* b) {
  if (a->sort_op != b->sort_op) {
    return false;
  }

  if (!equal(a->key, b->key)) {
    return false;
  }

  return true;
}

static bool equal_path(Path* a, Path* b) {
  if (a->path_type != b->path_type) {
    return false;
  }

  if (!equal(a->parent, b->parent)) {
    return false;
  }

  // Do not check path costs, since they may not be set yet, and being
  // float values there are roundoff error issues anyway...
  if (!equal(a->path_keys, b->path_keys)) {
    return false;
  }

  return true;
}

static bool equal_index_path(IndexPath* a, IndexPath* b) {
  if (!equal_path((Path*)a, (Path*)b)) {
    return false;
  }

  if (!equali(a->index_id, b->index_id)) {
    return false;
  }

  if (!equal(a->index_qual, b->index_qual)) {
    return false;
  }

  if (a->index_scan_dir != b->index_scan_dir) {
    return false;
  }

  if (!equali(a->join_rel_ids, b->join_rel_ids)) {
    return false;
  }

  // Skip 'rows' because of possibility of floating-point roundoff
  // error. It should be derivable from the other fields anyway.

  return true;
}

static bool equal_tid_path(TidPath* a, TidPath* b) {
  if (!equal_path((Path*)a, (Path*)b)) {
    return false;
  }

  if (!equal(a->tid_eval, b->tid_eval)) {
    return false;
  }

  if (!equali(a->unjoined_rel_ids, b->unjoined_rel_ids)) {
    return false;
  }

  return true;
}

static bool equal_join_path(JoinPath* a, JoinPath* b) {
  if (!equal_path((Path*)a, (Path*)b)) {
    return false;
  }

  if (!equal(a->outer_join_path, b->outer_join_path)) {
    return false;
  }

  if (!equal(a->inner_join_path, b->inner_join_path)) {
    return false;
  }

  if (!equal(a->join_restrict_info, b->join_restrict_info)) {
    return false;
  }

  return true;
}

static bool equal_nest_path(NestPath* a, NestPath* b) {
  if (!equal_join_path((JoinPath*)a, (JoinPath*)b)) {
    return false;
  }

  return true;
}

static bool equal_merge_path(MergePath* a, MergePath* b) {
  if (!equal_join_path((JoinPath*)a, (JoinPath*)b)) {
    return false;
  }

  if (!equal(a->path_merge_clauses, b->path_merge_clauses)) {
    return false;
  }

  if (!equal(a->outer_sort_keys, b->outer_sort_keys)) {
    return false;
  }

  if (!equal(a->inner_sort_keys, b->inner_sort_keys)) {
    return false;
  }

  return true;
}

static bool equal_hash_path(HashPath* a, HashPath* b) {
  if (!equal_join_path((JoinPath*)a, (JoinPath*)b)) {
    return false;
  }

  if (!equal(a->path_hash_clauses, b->path_hash_clauses)) {
    return false;
  }

  return true;
}

// TODO(gc): fix this.
// extern IndexScan;

// // XXX This equality function is a quick hack, should be
// //     fixed to compare all fields.
// //
// // XXX Why is this even here? We don't have equal() funcs for
// //     any other kinds of Plan nodes... likely this is dead code...
// static bool equal_index_scan(IndexScan* a, IndexScan* b) { return true; }

static bool equal_restrict_info(RestrictInfo* a, RestrictInfo* b) {
  if (!equal(a->clause, b->clause)) {
    return false;
  }

  if (!equal(a->sub_clause_indices, b->sub_clause_indices)) {
    return false;
  }

  if (a->merge_join_operator != b->merge_join_operator) {
    return false;
  }

  if (a->left_sort_op != b->left_sort_op) {
    return false;
  }

  if (a->right_sort_op != b->right_sort_op) {
    return false;
  }

  if (a->hash_join_operator != b->hash_join_operator) {
    return false;
  }

  return true;
}

static bool equal_join_info(JoinInfo* a, JoinInfo* b) {
  if (!equali(a->unjoined_relids, b->unjoined_relids)) {
    return false;
  }

  if (!equal(a->jinfo_restrict_info, b->jinfo_restrict_info)) {
    return false;
  }

  return true;
}

static bool equal_iter(Iter* a, Iter* b) { return equal(a->iter_expr, b->iter_expr); }

static bool equal_stream(Stream* a, Stream* b) {
  if (a->clause_type != b->clause_type) {
    return false;
  }

  if (a->group_up != b->group_up) {
    return false;
  }

  if (a->group_cost != b->group_cost) {
    return false;
  }

  if (a->group_sel != b->group_sel) {
    return false;
  }

  if (!equal(a->path_ptr, b->path_ptr)) {
    return false;
  }

  if (!equal(a->cinfo, b->cinfo)) {
    return false;
  }

  if (!equal(a->upstream, b->upstream)) {
    return false;
  }

  return equal(a->downstream, b->downstream);
}

static bool equal_query(Query* a, Query* b) {
  if (a->command_type != b->command_type) {
    return false;
  }

  if (!equal(a->utility_stmt, b->utility_stmt)) {
    return false;
  }

  if (a->result_relation != b->result_relation) {
    return false;
  }

  if (a->into && b->into) {
    if (strcmp(a->into, b->into) != 0) {
      return false;
    }
  } else {
    if (a->into != b->into) {
      return false;
    }
  }

  if (a->is_portal != b->is_portal) {
    return false;
  }

  if (a->is_binary != b->is_binary) {
    return false;
  }

  if (a->is_temp != b->is_temp) {
    return false;
  }

  if (a->union_all != b->union_all) {
    return false;
  }

  if (a->has_aggs != b->has_aggs) {
    return false;
  }

  if (a->has_sublinks != b->has_sublinks) {
    return false;
  }

  if (!equal(a->rtable, b->rtable)) {
    return false;
  }

  if (!equal(a->target_list, b->target_list)) {
    return false;
  }

  if (!equal(a->qual, b->qual)) {
    return false;
  }

  if (!equal(a->row_mark, b->row_mark)) {
    return false;
  }

  if (!equal(a->distinct_clause, b->distinct_clause)) {
    return false;
  }

  if (!equal(a->sort_clause, b->sort_clause)) {
    return false;
  }

  if (!equal(a->group_clause, b->group_clause)) {
    return false;
  }

  if (!equal(a->having_qual, b->having_qual)) {
    return false;
  }

  if (!equal(a->intersect_clause, b->intersect_clause)) {
    return false;
  }

  if (!equal(a->union_clause, b->union_clause)) {
    return false;
  }

  if (!equal(a->limit_offset, b->limit_offset)) {
    return false;
  }

  if (!equal(a->limit_count, b->limit_count)) {
    return false;
  }

  // We do not check the internal-to-the-planner fields: base_rel_list,
  // join_rel_list, equi_key_list, query_pathkeys. They might not be set
  // yet, and in any case they should be derivable from the other
  // fields.
  return true;
}

static bool equal_range_tbl_entry(RangeTblEntry* a, RangeTblEntry* b) {
  if (a->rel_name && b->rel_name) {
    if (strcmp(a->rel_name, b->rel_name) != 0) {
      return false;
    }
  } else {
    if (a->rel_name != b->rel_name) {
      return false;
    }
  }

  if (!equal(a->ref, b->ref)) {
    return false;
  }

  if (a->rel_id != b->rel_id) {
    return false;
  }

  if (a->inh != b->inh) {
    return false;
  }

  if (a->in_from_cl != b->in_from_cl) {
    return false;
  }

  if (a->in_join_set != b->in_join_set) {
    return false;
  }

  if (a->skip_acl != b->skip_acl) {
    return false;
  }

  return true;
}

static bool equal_sort_clause(SortClause* a, SortClause* b) {
  if (a->tle_sort_group_ref != b->tle_sort_group_ref) {
    return false;
  }

  if (a->sort_op != b->sort_op) {
    return false;
  }

  return true;
}

static bool equal_target_entry(TargetEntry* a, TargetEntry* b) {
  if (!equal(a->resdom, b->resdom)) {
    return false;
  }

  if (!equal(a->fjoin, b->fjoin)) {
    return false;
  }

  if (!equal(a->expr, b->expr)) {
    return false;
  }

  return true;
}

static bool equal_case_expr(CaseExpr* a, CaseExpr* b) {
  if (a->case_type != b->case_type) {
    return false;
  }

  if (!equal(a->arg, b->arg)) {
    return false;
  }

  if (!equal(a->args, b->args)) {
    return false;
  }

  if (!equal(a->def_result, b->def_result)) {
    return false;
  }

  return true;
}

static bool equal_case_when(CaseWhen* a, CaseWhen* b) {
  if (!equal(a->expr, b->expr)) {
    return false;
  }

  if (!equal(a->result, b->result)) {
    return false;
  }

  return true;
}

static bool equal_value(Value* a, Value* b) {
  if (a->type != b->type) {
    return false;
  }

  switch (a->type) {
    case T_Integer:
      return a->val.ival == b->val.ival;

    case T_Float:
    case T_String:
      return strcmp(a->val.str, b->val.str) == 0;

    default:
      break;
  }

  return true;
}

bool equal(void* a, void* b) {
  bool retval = false;

  if (a == b) {
    return true;
  }

  // Note that a != b, so only one of them can be NULL.
  if (a == NULL || b == NULL) {
    return false;
  }

  // Are they the same type of nodes?
  if (NODE_TAG(a) != NODE_TAG(b)) {
    return false;
  }

  switch (NODE_TAG(a)) {
    case T_Resdom:
      retval = equal_resdom(a, b);
      break;

    case T_Fjoin:
      retval = equal_fjoin(a, b);
      break;

    case T_Expr:
      retval = equal_expr(a, b);
      break;

    case T_Iter:
      retval = equal_iter(a, b);
      break;

    case T_Stream:
      retval = equal_stream(a, b);
      break;

    case T_Attr:
      retval = equal_attr(a, b);
      break;

    case T_Var:
      retval = equal_var(a, b);
      break;

    case T_Array:
      retval = equal_array(a, b);
      break;

    case T_ArrayRef:
      retval = equal_array_ref(a, b);
      break;

    case T_Oper:
      retval = equal_oper(a, b);
      break;

    case T_Const:
      retval = equal_const(a, b);
      break;

    case T_Param:
      retval = equal_param(a, b);
      break;

    case T_Aggref:
      retval = equal_aggref(a, b);
      break;

    case T_SubLink:
      retval = equal_sublink(a, b);
      break;

    case T_RelabelType:
      retval = equal_relabel_type(a, b);
      break;

    case T_Func:
      retval = equal_func(a, b);
      break;

    case T_RestrictInfo:
      retval = equal_restrict_info(a, b);
      break;

    case T_RelOptInfo:
      retval = equal_rel_opt_info(a, b);
      break;

    case T_IndexOptInfo:
      retval = equal_index_opt_info(a, b);
      break;

    case T_PathKeyItem:
      retval = equal_path_key_item(a, b);
      break;

    case T_Path:
      retval = equal_path(a, b);
      break;

    case T_IndexPath:
      retval = equal_index_path(a, b);
      break;

    case T_TidPath:
      retval = equal_tid_path(a, b);
      break;

    case T_NestPath:
      retval = equal_nest_path(a, b);
      break;

    case T_MergePath:
      retval = equal_merge_path(a, b);
      break;

    case T_HashPath:
      retval = equal_hash_path(a, b);
      break;

    case T_IndexScan:
      // TODO(gc): fix this.
      // retval = equal_index_scan(a, b);
      break;

    case T_TidScan:
      // TODO(gc): fix this.
      // retval = equal_tid_scan(a, b);
      break;

    case T_SubPlan:
      // TODO(gc): fix this.
      // retval = equal_subplan(a, b);
      break;

    case T_JoinInfo:
      retval = equal_join_info(a, b);
      break;

    case T_EState:
      // TODO(gc): fix this.
      // retval = equal_estate(a, b);
      break;

    case T_Integer:
    case T_Float:
    case T_String:
      retval = equal_value(a, b);
      break;

    case T_List: {
      List* la = (List*)a;
      List* lb = (List*)b;
      List* l;

      // Try to reject by length check before we grovel through all the elements...
      if (length(a) != length(b)) {
        return false;
      }

      FOR_EACH(l, la) {
        if (!equal(LFIRST(l), LFIRST(lb))) {
          return false;
        }

        lb = LNEXT(lb);
      }
      retval = true;
    } break;

    case T_Query:
      retval = equal_query(a, b);
      break;

    case T_RangeTblEntry:
      retval = equal_range_tbl_entry(a, b);
      break;

    case T_SortClause:
      retval = equal_sort_clause(a, b);
      break;

    case T_GroupClause:
      // GroupClause is equivalent to SortClause.
      retval = equal_sort_clause(a, b);
      break;

    case T_TargetEntry:
      retval = equal_target_entry(a, b);
      break;

    case T_CaseExpr:
      retval = equal_case_expr(a, b);
      break;

    case T_CaseWhen:
      retval = equal_case_when(a, b);
      break;

    default:
      elog(NOTICE, "%s: don't know whether nodes of type %d are equal.", __func__, NODE_TAG(a));
      break;
  }

  return retval;
}

// Compares two lists of integers.
static bool equali(List* a, List* b) {
  List* l;

  FOR_EACH(l, a) {
    if (b == NIL) {
      return false;
    }

    if (LFIRSTI(l) != LFIRSTI(b)) {
      return false;
    }

    b = LNEXT(b);
  }

  if (b != NIL) {
    return false;
  }

  return true;
}