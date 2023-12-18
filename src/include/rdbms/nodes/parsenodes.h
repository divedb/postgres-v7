//===----------------------------------------------------------------------===//
//
// parsenodes.h
//  Definitions for parse tree nodes
//
//
//  Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
//  Portions Copyright (c) 1994, Regents of the University of California
//
//  $Id: parsenodes.h,v 1.126 2001/03/23 04:49:56 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_NODES_PARSE_NODES_H_
#define RDBMS_NODES_PARSE_NODES_H_

#include "rdbms/nodes/primnodes.h"

// Query
//  All statments are turned into a Query tree (via transformStmt)
//  for further processing by the optimizer
//  utility statements (i.e. non-optimizable statements)
//  have the *utilityStmt field set.
//
// we need the isPortal flag because portal names can be null too; can
// get rid of it if we support CURSOR as a commandType.
typedef struct Query {
  NodeTag type;

  CmdType command_type;   // Select|insert|update|delete|utility.
  Node* utility_stmt;     // Non-null if this is a non-optimizable statement.
  int result_relation;    // Target relation (index to rtable).
  char* into;             // Portal (cursor) name.
  bool is_portal;         // Is this a retrieve into portal?.
  bool is_binary;         // Binary portal?.
  bool is_temp;           // Is 'into' a temp table?.
  bool has_aggs;          // Has aggregates in tlist or havingQual.
  bool has_sublinks;      // Has subquery SubLink.
  List* rtable;           // List of range table entries.
  List* jointree;         // Table join tree (FROM and WHERE clauses).
  List* row_marks;        // Integer list of RT indexes of relations that are selected FOR UPDATE
  List* target_list;      // Target list (of TargetEntry)
  List* group_clause;     // A list of GroupClause's
  Node* having_qual;      // Qualifications applied to groups.
  List* distinct_clause;  // A list of SortClause's
  List* sort_clause;      // A list of SortClause's
  Node* limit_offset;     // # of result tuples to skip.
  Node* limit_count;      // # of result tuples to return.
  Node* set_operations;   // Set-operation tree if this is top level of a UNION/INTERSECT/EXCEPT query

  // If the resultRelation turns out to be the parent of an inheritance
  // tree, the planner will add all the child tables to the rtable and
  // store a list of the rtindexes of all the result relations here.
  // This is done at plan time, not parse time, since we don't want to
  // commit to the exact set of child tables at parse time. This field
  // ought to go in some sort of TopPlan plan node, not in the Query.
  List* result_relations;  // Integer list of RT indexes, or NIL

  // Internal to planner.
  List* base_rel_list;   // List of base-relation RelOptInfos
  List* join_rel_list;   // List of join-relation RelOptInfos
  List* equi_key_list;   // List of lists of equijoined PathKeyItems
  List* query_pathkeys;  // Pathkeys for query_planner()'s result
} Query;

typedef enum InhOption {
  INH_NO,      // Do NOT scan child tables
  INH_YES,     // DO scan child tables
  INH_DEFAULT  // Use current SQL_inheritance option
} InhOption;

// Other Statements (no optimizations required)
//
// Some of them require a little bit of transformation (which is also
// done by transformStmt). The whole structure is then passed on to
// ProcessUtility (by-passing the optimization step) as the utilityStmt
// field in Query.

// The fields are used in different ways by the different variants of this command.
typedef struct AlterTableStmt {
  NodeTag type;
  char subtype;       // A = add, T = alter, D = drop, C = add constr, X = drop constr
  char* rel_name;     // Table to work on
  InhOption inh_opt;  // Recursively on children?
  char* name;         // Column or constraint name to act on
  Node* def;          // Definition of new column or constraint
  int behavior;       // CASCADE or RESTRICT drop behavior
} AlterTableStmt;

// Change ACL Statement.
typedef struct ChangeACLStmt {
  NodeTag type;
  List* rel_names;
  char* acl_string;
} ChangeACLStmt;

// Close Portal Statement.
typedef struct ClosePortalStmt {
  NodeTag type;
  char* portal_name;  // Name of the portal (cursor)
} ClosePortalStmt;

// Copy Statement.
typedef struct CopyStmt {
  NodeTag type;
  bool binary;       // Is a binary copy?
  char* rel_name;    // The relation to copy
  bool oids;         // Copy oid's?
  int direction;     // TO or FROM
  char* filename;    // If NULL, use stdin/stdout
  char* delimiter;   // Delimiter character, \t by default
  char* null_print;  // How to print NULLs, `\N' by default
} CopyStmt;

// Create Table Statement.
typedef struct CreateStmt {
  NodeTag type;
  bool is_temp;         // Is this a temp table?
  char* relname;        // Name of relation to create
  List* table_elts;     // Column definitions (list of ColumnDef)
  List* inh_rel_names;  // Relations to inherit from (list of T_String Values)
  List* constraints;    // Constraints (list of Constraint and FkConstraint nodes)
} CreateStmt;

// Definitions for plain (non-FOREIGN KEY) constraints in CreateStmt
//
// XXX probably these ought to be unified with FkConstraints at some point?
//
// For constraints that use expressions (CONSTR_DEFAULT, CONSTR_CHECK)
// we may have the expression in either "raw" form (an untransformed
// parse tree) or "cooked" form (the nodeToString representation of
// an executable expression tree), depending on how this Constraint
// node was created (by parsing, or by inheritance from an existing
// relation).  We should never have both in the same node!
//
// Constraint attributes (DEFERRABLE etc) are initially represented as
// separate Constraint nodes for simplicity of parsing.  analyze.c makes
// a pass through the constraints list to attach the info to the appropriate
// FkConstraint node (and, perhaps, someday to other kinds of constraints).
typedef enum ConstrType {
  CONSTR_NULL,  // Not SQL92, but a lot of people expect it
  CONSTR_NOTNULL,
  CONSTR_DEFAULT,
  CONSTR_CHECK,
  CONSTR_PRIMARY,
  CONSTR_UNIQUE,
  CONSTR_ATTR_DEFERRABLE,  // Attributes for previous constraint node
  CONSTR_ATTR_NOT_DEFERRABLE,
  CONSTR_ATTR_DEFERRED,
  CONSTR_ATTR_IMMEDIATE
} ConstrType;

typedef struct Constraint {
  NodeTag type;
  ConstrType con_type;
  char* name;         // Name, or NULL if unnamed
  Node* raw_expr;     // Expr, as untransformed parse tree
  char* cooked_expr;  // Expr, as nodeToString representation
  List* keys;         // Ident nodes naming referenced column(s)
} Constraint;

// Definitions for FOREIGN KEY constraints in CreateStmt.
#define FKCONSTR_ON_KEY_NOACTION   0x0000
#define FKCONSTR_ON_KEY_RESTRICT   0x0001
#define FKCONSTR_ON_KEY_CASCADE    0x0002
#define FKCONSTR_ON_KEY_SETNULL    0x0004
#define FKCONSTR_ON_KEY_SETDEFAULT 0x0008

#define FKCONSTR_ON_DELETE_MASK  0x000F
#define FKCONSTR_ON_DELETE_SHIFT 0

#define FKCONSTR_ON_UPDATE_MASK  0x00F0
#define FKCONSTR_ON_UPDATE_SHIFT 4

typedef struct FkConstraint {
  NodeTag type;
  char* constr_name;   // Constraint name
  char* pktable_name;  // Primary key table name
  List* fk_attrs;      // Attributes of foreign key
  List* pk_attrs;      // Corresponding attrs in PK table
  char* match_type;    // FULL or PARTIAL
  int32 actions;       // ON DELETE/UPDATE actions
  bool deferrable;     // DEFERRABLE
  bool init_deferred;  // INITIALLY DEFERRED
} FkConstraint;

// Create/Drop TRIGGER Statements.
typedef struct CreateTrigStmt {
  NodeTag type;
  char* trig_name;  // TRIGGER' name
  char* rel_name;   // Triggered relation
  char* func_name;  // Function to call (or NULL)
  List* args;       // List of (T_String) Values or NULL
  bool before;      // BEFORE/AFTER
  bool row;         // ROW/STATEMENT
  char actions[4];  // Insert, Update, Delete
  char* lang;       // NULL (which means Clanguage)
  char* text;       // AS 'text'
  List* attr;       // UPDATE OF a, b,... (NI) or NULL
  char* when;       // WHEN 'a > 10 ...' (NI) or NULL

  // The following are used for referential.
  // integrity constraint triggers.
  bool is_constraint;     // This is an RI trigger
  bool deferrable;        // [NOT] DEFERRABLE
  bool init_deferred;     // INITIALLY {DEFERRED|IMMEDIATE}
  char* constr_rel_name;  // Opposite relation
} CreateTrigStmt;

typedef struct DropTrigStmt {
  NodeTag type;
  char* trig_name;  // TRIGGER' name
  char* rel_name;   // Triggered relation.
} DropTrigStmt;

// Create/Drop PROCEDURAL LANGUAGE Statement.
typedef struct CreatePLangStmt {
  NodeTag type;
  char* pl_name;      // PL name
  char* pl_handler;   // PL call handler function
  char* pl_compiler;  // lancompiler text
  bool pl_trusted;    // PL is trusted
} CreatePLangStmt;

typedef struct DropPLangStmt {
  NodeTag type;
  char* pl_name;  // PL name
} DropPLangStmt;

// Create/Alter/Drop User Statements.
typedef struct CreateUserStmt {
  NodeTag type;
  char* user;         // PostgreSQL user login
  char* password;     // PostgreSQL user password
  int sys_id;         // PgSQL system id (-1 if don't care)
  bool create_db;     // Can the user create databases
  bool create_user;   // Can this user create users
  List* group_elts;   // The groups the user is a member of
  char* valid_until;  // The time the login is valid until
} CreateUserStmt;

typedef struct AlterUserStmt {
  NodeTag type;
  char* user;         // PostgreSQL user login
  char* password;     // PostgreSQL user password
  int create_db;      // Can the user create databases?
  int create_user;    // Can this user create users?
  char* valid_until;  // The time the login is valid until
} AlterUserStmt;

typedef struct DropUserStmt {
  NodeTag type;
  List* users;  // List of users to remove
} DropUserStmt;

// Create/Alter/Drop Group Statements.
typedef struct CreateGroupStmt {
  NodeTag type;
  char* name;        // Name of the new group
  int sys_id;        // Group id (-1 if pick default)
  List* init_users;  // List of initial users
} CreateGroupStmt;

typedef struct AlterGroupStmt {
  NodeTag type;
  char* name;        // Name of group to alter
  int action;        // +1 = add, -1 = drop user
  int sys_id;        // sysid change
  List* list_users;  // List of users to add/drop
} AlterGroupStmt;

typedef struct DropGroupStmt {
  NodeTag type;
  char* name;
} DropGroupStmt;

// Create SEQUENCE Statement.
typedef struct CreateSeqStmt {
  NodeTag type;
  char* seq_name;  // The relation to create
  List* options;
} CreateSeqStmt;

// Create Version Statement.
typedef struct VersionStmt {
  NodeTag type;
  char* rel_name;       // The new relation
  int direction;        // FORWARD | BACKWARD
  char* from_rel_name;  // Relation to create a version
  char* date;           // Date of the snapshot
} VersionStmt;

// Create {Operator|Type|Aggregate} Statement.
typedef struct DefineStmt {
  NodeTag type;
  int def_type;  // OPERATOR|P_TYPE|AGGREGATE
  char* def_name;
  List* definition;  // A list of DefElem
} DefineStmt;

#define DROP_TABLE    1
#define DROP_SEQUENCE 2
#define DROP_VIEW     3
#define DROP_INDEX    4
#define DROP_RULE     5
#define DROP_TYPE_P   6

// Drop Table Statement.
typedef struct DropStmt {
  NodeTag type;
  List* names;
  int remove_type;
} DropStmt;

// Truncate Table Statement.
typedef struct TruncateStmt {
  NodeTag type;
  char* rel_name;  // Relation to be truncated
} TruncateStmt;

// Comment On Statement.
typedef struct CommentStmt {
  NodeTag type;
  int obj_type;        // Object's type
  char* obj_name;      // Name of the object
  char* obj_property;  // Property Id (such as column)
  List* obj_list;      // Arguments for VAL objects
  char* comment;       // The comment to insert
} CommentStmt;

// Extend Index Statement.
typedef struct ExtendStmt {
  NodeTag type;
  char* idx_name;      // Name of the index
  Node* where_clause;  // Qualifications
  List* range_table;   // Range table, filled in by transformStmt()
} ExtendStmt;

// Begin Recipe Statement.
typedef struct RecipeStmt {
  NodeTag type;
  char* recipe_name;  // Name of the recipe
} RecipeStmt;

// Fetch Statement.
typedef struct FetchStmt {
  NodeTag type;
  int direction;      // FORWARD or BACKWARD
  int how_many;       // Amount to fetch ("ALL" --> 0)
  char* portal_name;  // Name of portal (cursor)
  bool is_move;       // TRUE if MOVE
} FetchStmt;

// Create Index Statement.
typedef struct IndexStmt {
  NodeTag type;
  char* idx_name;       // Name of the index
  char* rel_name;       // Name of relation to index on
  char* access_method;  // Name of acess methood (eg. btree)
  List* index_params;   // A list of IndexElem
  List* with_clause;    // A list of DefElem
  Node* where_clause;   // Qualifications
  List* range_table;    // Range table, filled in by transformStmt()
  bool unique;          // Is index unique
  bool primary;         // Is index on primary key
} IndexStmt;

// Create Function Statement.
typedef struct ProcedureStmt {
  NodeTag type;
  char* func_name;    // Name of function to create
  List* arg_types;    // List of argument types (TypeName nodes
  Node* return_type;  // The return type (as a string or a TypeName (ie.setof)
  List* with_clause;  // A list of DefElem
  List* as;           // The SQL statement or filename
  char* language;     // C or SQL, etc
} ProcedureStmt;

// Drop Aggregate Statement.
typedef struct RemoveAggrStmt {
  NodeTag type;
  char* agg_name;  // Aggregate to drop
  char* agg_type;  // TypeName for input datatype, or NULL
} RemoveAggrStmt;

// Drop Function Statement.
typedef struct RemoveFuncStmt {
  NodeTag type;
  char* func_name;  // Function to drop
  List* args;       // Types of the arguments
} RemoveFuncStmt;

// Drop Operator Statement.
typedef struct RemoveOperStmt {
  NodeTag type;
  char* op_name;  // Operator to drop
  List* args;     // Types of the arguments
} RemoveOperStmt;

// Alter Table Statement.
typedef struct RenameStmt {
  NodeTag type;
  char* rel_name;     // Relation to be altered
  InhOption inh_opt;  // Recursively alter children?
  char* column;       // If NULL, rename the relation name to the new name. Otherwise, rename this column name
  char* new_name;     // The new name
} RenameStmt;

// Create Rule Statement.
// TODO: what is do instead?
typedef struct RuleStmt {
  NodeTag type;
  char* rule_name;      // Name of the rule
  Node* where_clause;   // Qualifications
  CmdType event;        // RETRIEVE
  struct Attr* object;  // Object affected
  bool instead;         // Is a 'do instead'?
  List* actions;        // The action statements
} RuleStmt;

// Notify Statement.
typedef struct NotifyStmt {
  NodeTag type;
  char* rel_name;  // Relation to notify
} NotifyStmt;

// Listen Statement.
typedef struct ListenStmt {
  NodeTag type;
  char* rel_name;  // Relation to listen on
} ListenStmt;

// Unlisten Statement.
typedef struct UnlistenStmt {
  NodeTag type;
  char* rel_name;  // Relation to unlisten on
} UnlistenStmt;

// {Begin|Abort|End} Transaction Statement.
typedef struct TransactionStmt {
  NodeTag type;
  int command;  // BEGIN|END|ABORT
} TransactionStmt;

// Create View Statement.
typedef struct ViewStmt {
  NodeTag type;
  char* view_name;  // Name of the view
  List* aliases;    // Target column names
  Query* query;     // The SQL statement
} ViewStmt;

// Load Statement.
typedef struct LoadStmt {
  NodeTag type;
  char* filename;  // File to load
} LoadStmt;

// Createdb Statement.
typedef struct CreatedbStmt {
  NodeTag type;
  char* db_name;      // Database to create.
  char* db_path;      // Location of database.
  char* db_template;  // Template to use (NULL = default)
  int encoding;       // Default encoding (see regex/pg_wchar.h).
} CreatedbStmt;

// Dropdb Statement.
typedef struct DropdbStmt {
  NodeTag type;
  char* db_name;  // Database to drop
} DropdbStmt;

// Cluster Statement (support pbrown's cluster index implementation).
typedef struct ClusterStmt {
  NodeTag type;
  char* rel_name;    // Relation being indexed
  char* index_name;  // Original index defined
} ClusterStmt;

// Vacuum Statement.
typedef struct VacuumStmt {
  NodeTag type;
  bool verbose;   // Print status info
  bool analyze;   // Analyze data
  char* vac_rel;  // Table to vacuum
  List* va_spec;  // Columns to analyse
} VacuumStmt;

// Explain Statement.
typedef struct ExplainStmt {
  NodeTag type;
  Query* query;  // The query
  bool verbose;  // Print plan info
} ExplainStmt;

typedef struct CheckPointStmt {
  NodeTag type;
} CheckPointStmt;

// Set Statement.
typedef struct VariableSetStmt {
  NodeTag type;
  char* name;
  char* value;
} VariableSetStmt;

// Show Statement.
typedef struct VariableShowStmt {
  NodeTag type;
  char* name;
} VariableShowStmt;

// Reset Statement.
typedef struct VariableResetStmt {
  NodeTag type;
  char* name;
} VariableResetStmt;

// LOCK Statement
typedef struct LockStmt {
  NodeTag type;
  char* rel_name;  // Relation to lock.
  int mode;        // Lock mode.
} LockStmt;

// SET CONSTRAINTS Statement.
typedef struct ConstraintsSetStmt {
  NodeTag type;
  List* constraints;
  bool deferred;
} ConstraintsSetStmt;

// REINDEX Statement.
typedef struct ReindexStmt {
  NodeTag type;
  int reindex_type;  // INDEX|TABLE|DATABASE
  const char* name;  // Name to reindex
  bool force;
  bool all;
} ReindexStmt;

// Optimizable Statements.

// Insert Statement.
typedef struct InsertStmt {
  NodeTag type;
  char* rel_name;  // Relation to insert into.

  // An INSERT statement has *either* VALUES or SELECT, never both. If
  // VALUES, a targetList is supplied (empty for DEFAULT VALUES). If
  // SELECT, a complete SelectStmt (or set-operation tree) is supplied.
  List* target_list;  // The target list (of ResTarget)
  Node* select_stmt;  // The source SELECT
} InsertStmt;

// Delete Statement.
typedef struct DeleteStmt {
  NodeTag type;
  char* rel_name;      // Relation to delete from
  Node* where_clause;  // Qualifications
  InhOption inh_opt;   // Recursively act on children?
} DeleteStmt;

// Update Statement.
typedef struct UpdateStmt {
  NodeTag type;
  char* rel_name;      // Relation to update
  List* target_list;   // The target list (of ResTarget)
  Node* where_clause;  // Qualifications
  List* from_clause;   // The from clause
  InhOption inh_opt;   // Recursively act on children?
} UpdateStmt;

// A "simple" SELECT is represented in the output of gram.y by a single
// SelectStmt node.  A SELECT construct containing set operators (UNION,
// INTERSECT, EXCEPT) is represented by a tree of SelectStmt nodes, in
// which the leaf nodes are component SELECTs and the internal nodes
// represent UNION, INTERSECT, or EXCEPT operators.  Using the same node
// type for both leaf and internal nodes allows gram.y to stick ORDER BY,
// LIMIT, etc, clause values into a SELECT statement without worrying
// whether it is a simple or compound SELECT.
typedef enum SetOperation { SETOP_NONE = 0, SETOP_UNION, SETOP_INTERSECT, SETOP_EXCEPT } SetOperation;

// Select Statement.
typedef struct SelectStmt {
  NodeTag type;
  List* distinct_clause;  // NULL, list of DISTINCT ON exprs, or lcons(NIL,NIL) for all (SELECT DISTINCT)
  char* into;             // Name of table (for select into table)
  bool is_temp;           // Into is a temp table
  List* target_list;      // The target list (of ResTarget)
  List* from_clause;      // The from clause
  Node* where_clause;     // Qualifications
  List* group_clause;     // GROUP BY clauses
  Node* having_clause;    // Having conditional-expression

  // These fields are used in both "leaf" SelectStmts and upper-level
  // SelectStmts. portalname/binary may only be set at the top level.
  List* sort_clause;   // Sort clause (a list of SortGroupBy's)
  char* portal_name;   // The portal (cursor) to create
  bool binary;         // A binary (internal) portal?
  Node* limit_offset;  // # of result tuples to skip
  Node* limit_count;   // # of result tuples to return
  List* for_update;    // FOR UPDATE clause

  // These fields are used only in upper-level SelectStmts.
  SetOperation op;          // Type of set op
  bool all;                 // ALL specified?
  struct SelectStmt* larg;  // Left child
  struct SelectStmt* rarg;  // Right child
} SelectStmt;

// Set Operation node for post-analysis query trees
//
// After parse analysis, a SELECT with set operations is represented by a
// top-level Query node containing the leaf SELECTs as subqueries in its
// range table.  Its setOperations field shows the tree of set operations,
// with leaf SelectStmt nodes replaced by RangeTblRef nodes, and internal
// nodes replaced by SetOperationStmt nodes.
typedef struct SetOperationStmt {
  NodeTag type;
  SetOperation op;  // Type of set op
  bool all;         // ALL specified?
  Node* larg;       // Left child
  Node* rarg;       // Right child

  // Eventually add fields for CORRESPONDING spec here
  // Fields derived during parse analysis:
  List* col_types;  // Integer list of OIDs of output column types
} SetOperationStmt;

// Supporting data structures for Parse Trees
//
// Most of these node types appear in raw parsetrees output by the grammar,
// and get transformed to something else by the analyzer. A few of them
// are used as-is in transformed querytrees.

// TypeName - specifies a type in definitions.
typedef struct TypeName {
  NodeTag type;
  char* name;          // Name of the type
  bool timezone;       // Timezone specified?
  bool setof;          // Is a set?
  int32 type_mod;      // Type modifier
  List* array_bounds;  // Array bounds
} TypeName;

// ParamNo - specifies a parameter reference.
typedef struct ParamNo {
  NodeTag type;
  int number;          // The number of the parameter
  TypeName* typename;  // The typecast
  List* indirection;   // Array references
} ParamNo;

// A_Expr - binary expressions.
typedef struct A_Expr {
  NodeTag type;
  int oper;       // Type of operation {OP,OR,AND,NOT,ISNULL,NOTNULL}
  char* op_name;  // Name of operator/function
  Node* lexpr;    // Left argument
  Node* rexpr;    // Right argument
} A_Expr;

// Specifies an Attribute (ie. a Column); could have nested dots or
// array references.
typedef struct Attr {
  NodeTag type;
  char* rel_name;     // Name of relation (can be "*")
  ParamNo* param_no;  // Or a parameter
  List* attrs;        // Attributes (possibly nested); list of Values (strings)
  List* indirection;  // Array refs (list of A_Indices')
} Attr;

// A_Const - a constant expression.
typedef struct A_Const {
  NodeTag type;
  Value val;           // The value (with the tag)
  TypeName* typename;  // Typecast
} A_Const;

// TypeCast - a CAST expression
//
// NOTE: for mostly historical reasons, A_Const and ParamNo parsenodes contain
// room for a TypeName; we only generate a separate TypeCast node if the
// argument to be casted is neither of those kinds of nodes.  In theory either
// representation would work, but it is convenient (especially for A_Const)
// to have the target type immediately available.
typedef struct TypeCast {
  NodeTag type;
  Node* arg;           // The expression being casted
  TypeName* typename;  // The target type
} TypeCast;

// CaseExpr - a CASE expression.
typedef struct CaseExpr {
  NodeTag type;
  Oid case_type;
  Node* arg;         // Implicit equality comparison argument
  List* args;        // The arguments (list of WHEN clauses)
  Node* def_result;  // The default result (ELSE clause)
} CaseExpr;

// CaseWhen - an argument to a CASE expression.
typedef struct CaseWhen {
  NodeTag type;
  Node* expr;    // Comparison expression
  Node* result;  // Substitution result
} CaseWhen;

// ColumnDef - column definition (used in various creates)
//
// If the column has a default value, we may have the value expression
// in either "raw" form (an untransformed parse tree) or "cooked" form
// (the nodeToString representation of an executable expression tree),
// depending on how this ColumnDef node was created (by parsing, or by
// inheritance from an existing relation).	We should never have both
// in the same node!
//
// The constraints list may contain a CONSTR_DEFAULT item in a raw
// parsetree produced by gram.y, but transformCreateStmt will remove
// the item and set raw_default instead.  CONSTR_DEFAULT items
// should not appear in any subsequent processing.
typedef struct ColumnDef {
  NodeTag type;
  char* col_name;        // Name of column
  TypeName* type_name;   // Type of column
  bool is_not_null;      // Flag to NOT NULL constraint
  bool is_sequence;      // Is a sequence?
  Node* raw_default;     // Default value (untransformed parse tree)
  char* cooked_default;  // NodeToString representation
  List* constraints;     // Other constraints on column
} ColumnDef;

// An identifier (could be an attribute or a relation name). Depending
// on the context at transformStmt time, the identifier is treated as
// either a relation name (in which case, isRel will be set) or an
// attribute (in which case, it will be transformed into an Attr).
typedef struct Ident {
  NodeTag type;
  char* name;         // Its name
  List* indirection;  // Array references
  bool is_rel;        // Is a relation - filled in by transformExpr()
} Ident;

// FuncCall - a function or aggregate invocation
//
// agg_star indicates we saw a 'foo(*)' construct, while agg_distinct
// indicates we saw 'foo(DISTINCT ...)'.  In either case, the construct
// *must* be an aggregate call.  Otherwise, it might be either an
// aggregate or some other kind of function.
typedef struct FuncCall {
  NodeTag type;
  char* func_name;    // Name of function
  List* args;         // The arguments (list of exprs)
  bool agg_star;      // Argument was really '*'
  bool agg_distinct;  // Arguments were labeled DISTINCT
} FuncCall;

// A_Indices - array reference or bounds ([lidx:uidx] or [uidx]).
typedef struct A_Indices {
  NodeTag type;
  Node* lidx;  // Could be NULL
  Node* uidx;
} A_Indices;

// Result target (used in target list of pre-transformed Parse trees)
//
// In a SELECT or INSERT target list, 'name' is either NULL or
// the column name assigned to the value.  (If there is an 'AS ColumnLabel'
// clause, the grammar sets 'name' from it; otherwise 'name' is initially NULL
// and is filled in during the parse analysis phase.)
// The 'indirection' field is not used at all.
//
// In an UPDATE target list, 'name' is the name of the destination column,
// and 'indirection' stores any subscripts attached to the destination.
// That is, our representation is UPDATE table SET name [indirection] = val.
typedef struct ResTarget {
  NodeTag type;
  char* name;         // Column name or NULL
  List* indirection;  // Subscripts for destination column, or NIL
  Node* val;          // The value expression to compute or assign
} ResTarget;

// SortGroupBy - for ORDER BY clause
typedef struct SortGroupBy {
  NodeTag type;
  char* use_op;  // Operator to use
  Node* node;    // Expression
} SortGroupBy;

// RangeVar - range variable, used in FROM clauses/
typedef struct RangeVar {
  NodeTag type;
  char* rel_name;     // The relation name
  InhOption inh_opt;  // Expand rel by inheritance?
  Attr* name;         // The name to be referenced (optional)
} RangeVar;

// Subquery appearing in a FROM clause.
typedef struct RangeSubselect {
  NodeTag type;
  Node* subquery;  // The untransformed sub-select clause
  Attr* name;      // Table alias & optional column aliases
} RangeSubselect;

// IndexElem - index parameters (used in CREATE INDEX).
// For a plain index, each 'name' is an attribute name in the heap relation,
// and 'args' is NIL.  For a functional index, only one IndexElem is allowed.
// It has name = name of function and args = list of attribute names that
// are the function's arguments.
typedef struct IndexElem {
  NodeTag type;
  char* name;  // Name of index
  List* args;  // If not NULL, function index
  char* class;
} IndexElem;

// DefElem - a definition (used in definition lists in the form of defname = arg).
typedef struct DefElem {
  NodeTag type;
  char* def_name;
  Node* arg;  // A (Value *) or a (TypeName *)
} DefElem;

// Nodes for a Query tree.

// TargetEntry
//  A target  entry (used in the transformed target list)
//
// One of resdom or fjoin is not NULL. a target list is
// ((<resdom | fjoin> expr) (<resdom | fjoin> expr) ...)
typedef struct TargetEntry {
  NodeTag type;
  Resdom* res_dom;  // fjoin overload this to be a list?
  Fjoin* fjoin;
  Node* expr;
} TargetEntry;

// RangeTblEntry -
//  A range table is a List of RangeTblEntry nodes.
//
//  Some of the following are only used in one of
//  the parsing, optimizing, execution stages.
//
//  eref is the expanded table name and columns for the underlying
//  relation. Note that for outer join syntax, allowed reference names
//  could be modified as one evaluates the nested clauses (e.g.
//  "SELECT ... FROM t1 NATURAL JOIN t2 WHERE ..." forbids explicit mention
//  of a table name in any reference to the join column.
//
//  inFromCl marks those range variables that are listed in the FROM clause.
//  In SQL, the query can only refer to range variables listed in the
//  FROM clause, but POSTQUEL allows you to refer to tables not listed,
//  in which case a range table entry will be generated. We still support
//  this POSTQUEL feature, although there is some doubt whether it's
//  convenient or merely confusing.  The flag is needed since an
//  implicitly-added RTE shouldn't change the namespace for unqualified
//  column names processed later, and it also shouldn't affect the
//  expansion of '*'.
//
//  inJoinSet marks those range variables that the planner should join
//  over even if they aren't explicitly referred to in the query.  For
//  example, "SELECT COUNT(1) FROM tx" should produce the number of rows
//  in tx.  A more subtle example uses a POSTQUEL implicit RTE:
//  SELECT COUNT(1) FROM tx WHERE TRUE OR (tx.f1 = ty.f2)
//
//  Here we should get the product of the sizes of tx and ty.  However,
//  the query optimizer can simplify the WHERE clause to "TRUE", so
//  ty will no longer be referred to explicitly; without a flag forcing
//  it to be included in the join, we will get the wrong answer. So,
//  a POSTQUEL implicit RTE must be marked inJoinSet but not inFromCl.
typedef struct RangeTblEntry {
  NodeTag type;
  char* rel_name;  // Real name of the relation
  Oid rel_id;      // OID of the relation
  // Fields valid for a subquery RET (else NULL).
  Query* subquery;  // The sub-query

  // Fields valid in all RTEs.
  Attr* alias;           // User-written alias clause, if any
  Attr* eref;            // Expanded reference names
  bool inh;              // Inheritance requested?
  bool in_from_cl;       // Present in FROM clause
  bool check_for_read;   // Check rel for read access
  bool check_for_write;  // Check rel for write access
  Oid check_as_user;     // If not zero, check access as this user
} RangeTblEntry;

// SortClause
//  Representation of ORDER BY clauses
//
// tleSortGroupRef must match ressortgroupref of exactly one Resdom of the
// associated targetlist; that is the expression to be sorted (or grouped) by.
// sortop is the OID of the ordering operator.
//
// SortClauses are also used to identify Resdoms that we will do a "Unique"
// filter step on (for SELECT DISTINCT and SELECT DISTINCT ON).  The
// distinctClause list is simply a copy of the relevant members of the
// sortClause list.  Note that distinctClause can be a subset of sortClause,
// but cannot have members not present in sortClause; and the members that
// do appear must be in the same order as in sortClause.
typedef struct SortClause {
  NodeTag type;
  Index tle_sort_group_ref;  // Reference into targetlist
  Oid sort_op;               // The sort operator to use
} SortClause;

// GroupClause
//  Representation of GROUP BY clauses
//
// GroupClause is exactly like SortClause except for the nodetag value
// (it's probably not even really necessary to have two different
// nodetags...).  We have routines that operate interchangeably on both.
typedef SortClause GroupClause;

#endif  // RDBMS_NODES_PARSE_NODES_H_
