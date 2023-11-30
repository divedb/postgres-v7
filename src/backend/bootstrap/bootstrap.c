/*-------------------------------------------------------------------------
 *
 * bootstrap.c
 *	  routines to support running postgres in 'bootstrap' mode
 *	bootstrap mode is used to create the initial template database
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /usr/local/cvsroot/pgsql/src/backend/bootstrap/bootstrap.c,v 1.81 2000/04/12 17:14:54 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "rdbms/bootstrap/bootstrap.h"

#include "rdbms/access/htup.h"
#include "rdbms/access/tupdesc.h"
#include "rdbms/postgres.h"

#define ALLOC(t, c) (t*)calloc((unsigned)(c), sizeof(t))

extern void base_init();
extern void startup_xlog();
extern void shutdown_xlog();
extern void bootstrap_xlog();

extern char XLogDir[];
extern char ControlFilePath[];

extern int int_yyparse();

static HashNode* add_str(char* str, int len, int mde_ref);
static Form_pg_attribute allocate_attribute();
static bool bootstrap_already_seen(Oid oid);
static int comp_hash(char* str, int len);
static HashNode* find_str(char* str, int len, HashNode* mde_ref);
static Oid get_type(char* type);
static void clean_up();

// Global variables.
//
// In the lexical analyzer, we need to get the reference number quickly from
// the string, and the string from the reference number.  Thus we have
// as our data structure a hash table, where the hashing key taken from
// the particular string.  The hash table is chained.  One of the fields
// of the hash table node is an index into the array of character pointers.
// The unique index number that every string is assigned is simply the
// position of its string pointer in the array of string pointers.
#define STR_TABLE_SIZE  10000
#define HASH_TABLE_SIZE 503

// Hash function numbers.
#define NUM      23
#define NUM_SQR  529
#define NUM_CUBE 12167

char* StrTable[STR_TABLE_SIZE];
HashNode* HashTable[HASH_TABLE_SIZE];

static int StrTableEnd = -1;  // Tells us last occupied string space.

// Basic information associated with each type.  This is used before
// pg_type is created.
//
// XXX several of these input/output functions do catalog scans
// (e.g., F_REGPROCIN scans pg_proc). This obviously creates some
// order dependencies in the catalog creation process.
typedef struct TypInfo {
  char name[NAME_DATA_LEN];
  Oid oid;
  Oid elem;
  int16 len;
  Oid in_proc;
  Oid out_proc;
} TypInfo;

// Assumes that 'oid' will not be zero.
void insert_one_tuple(Oid object_id) {
  HeapTuple tuple;
  TupleDesc tup_desc;

  int i;

  if (DebugMode) {
    printf("%s: oid %u, %d attrs\n", __func__, object_id, numattr);
    fflush(stdout);
  }
}