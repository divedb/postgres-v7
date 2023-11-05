// =========================================================================
//
// plannodes.h
//  Definitions for query plan nodes
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: plannodes.h,v 1.39 2000/04/12 17:16:40 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_NODES_PLAN_NODES_H_
#define RDBMS_NODES_PLAN_NODES_H_

// Executor State types are used in the plannode structures
// so we have to include their definitions too.
//
//  Node Type                   node information used by executor
//
// control nodes
//
//  Result                      ResultState             resstate;
//  Append                      AppendState             appendstate;
//
// scan nodes
//
//  Scan ***                    CommonScanState         scanstate;
//  IndexScan                   IndexScanState          indxstate;
//
//  (*** nodes which inherit Scan also inherit scanstate)
//
// join nodes
//
//  NestLoop                    NestLoopState           nlstate;
//  MergeJoin                   MergeJoinState          mergestate;
//  HashJoin                    HashJoinState           hashjoinstate;
//
// materialize nodes
//
//  Material                    MaterialState           matstate;
//  Sort                        SortState               sortstate;
//  Unique                      UniqueState             uniquestate;
//  Hash                        HashState               hashstate;

// Node definitions.

// Plan node.
typedef struct Plan {
  NodeTag type;

  // Estimated execution costs for plan (see costsize.c for more info).
  Cost startup_cost;  // Cost expended before fetching any tuples.
  Cost total_cost;    // Total cost (assuming all tuples fetched).

  // Planner's estimate of result size (note: LIMIT, if any, is not
  // considered in setting plan_rows)
  double plan_rows;  // Number of rows plan is expected to emit
  int plan_width;    // Average row width in bytes

  EState* state;  // At execution time, state's of individual nodes point to one EState for the whole top-level plan */
  List* target_list;
  List* qual;  // Node* or List* ??
  struct Plan* left_tree;
  struct Plan* right_tree;
  List* ext_param;  // Indices of _all_ _external_ PARAM_EXEC
                    // for this plan in global
                    // es_param_exec_vals. Params from
                    // setParam from initPlan-s are not
                    // included, but their execParam-s are
                    // here!!!
  List* loc_param;  // Someones from setParam-s.
  List* chg_param;  // list of changed ones from the above.
  List* initPlan;   // Init Plan nodes (un-correlated expr subselects).
  List* subPlan;    // Other SubPlan nodes.

  // We really need in some TopPlan node to store range table and
  // resultRelation from Query there and get rid of Query itself from
  // Executor. Some other stuff like below could be put there, too.
  int n_param_exec;  // Number of them in entire query. This is
                     // to get Executor know about how many
                     // param_exec there are in query plan.
} Plan;

// These are are defined to avoid confusion problems with "left"
// and "right" and "inner" and "outer".  The convention is that
// the "left" plan is the "outer" plan and the "right" plan is
// the inner plan, but these make the code more readable.
#define INNER_PLAN(node) (((Plan*)(node))->right_tree)
#define OUTER_PLAN(node) (((Plan*)(node))->left_tree)

// Top-level nodes.

// All plan nodes "derive" from the Plan structure by having the
// Plan structure as the first field.  This ensures that everything works
// when nodes are cast to Plan's.  (node pointers are frequently cast to Plan*
// when passed around generically in the executor.

// Result node -
//  Returns tuples from outer plan that satisfy the qualifications
// TODO(gc): fix this.
// typedef struct Result {
//   Plan plan;
//   Node* res_constant_qual;
//   ResultState* res_state;
// } Result;

// Append node
// TODO(gc): fix this
// typedef struct Append {
//   Plan plan;
//   List* append_plans;
//   List* union_rtables;   // List of range tables, one for each union query.
//   Index inherit_rel_id;  // The range table has to be changed for inheritance.
//   List* inherit_rtable;
//   AppendState* append_state;
// } Append;

// /*
//  * ==========
//  * Scan nodes
//  * ==========
//  */
// typedef struct Scan
// {
// 	Plan		plan;
// 	Index		scanrelid;		/* relid is index into the range table */
// 	CommonScanState *scanstate;
// } Scan;

// /* ----------------
//  *		sequential scan node
//  * ----------------
//  */
// typedef Scan SeqScan;

/* ----------------
 *		index scan node
 * ----------------
 */
// typedef struct IndexScan {
//   Scan scan;
//   List* indxid;
//   List* indxqual;
//   List* indxqualorig;
//   ScanDirection indxorderdir;
//   IndexScanState* indxstate;
// } IndexScan;

#endif  // RDBMS_NODES_PLAN_NODES_H_
