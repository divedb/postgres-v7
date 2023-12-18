//===----------------------------------------------------------------------===//
//
// bootstrap.c
//  Routines to support running postgres in 'bootstrap' mode
//  bootstrap mode is used to create the initial template database.
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/bootstrap/bootstrap.c,v 1.81 2000/04/12 17:14:54 momjian Exp $
//
//===----------------------------------------------------------------------===//

#include "rdbms/bootstrap/bootstrap.h"

#include <unistd.h>

#include "rdbms/access/htup.h"
#include "rdbms/access/tupdesc.h"
#include "rdbms/catalog/pg_attribute.h"
#include "rdbms/catalog/pg_type.h"
#include "rdbms/miscadmin.h"
#include "rdbms/nodes/execnodes.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/exc.h"
#include "rdbms/utils/fmgroids.h"

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
// the particular string. The hash table is chained. One of the fields
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

// Basic information associated with each type. This is used before
// pg_type is created.
//
// XXX several of these input/output functions do catalog scans
// (e.g., F_REGPROCIN scans pg_proc). This obviously creates some
// order dependencies in the catalog creation process.
typedef struct TypeInfo {
  char name[NAME_DATA_LEN];
  Oid oid;
  Oid elem;
  int16 len;
  Oid in_proc;
  Oid out_proc;
} TypeInfo;

static struct TypeInfo ProcID[] = {
    {"bool", BOOLOID, 0, 1, F_BOOLIN, F_BOOLOUT},
    {"bytea", BYTEAOID, 0, -1, F_BYTEAIN, F_BYTEAOUT},
    {"char", CHAROID, 0, 1, F_CHARIN, F_CHAROUT},
    {"name", NAMEOID, 0, NAME_DATA_LEN, F_NAMEIN, F_NAMEOUT},
    {"int2", INT2OID, 0, 2, F_INT2IN, F_INT2OUT},
    {"int2vector", INT2VECTOROID, 0, INDEX_MAX_KEYS * 2, F_INT2VECTORIN, F_INT2VECTOROUT},
    {"int4", INT4OID, 0, 4, F_INT4IN, F_INT4OUT},
    {"regproc", REGPROCOID, 0, 4, F_REGPROCIN, F_REGPROCOUT},
    {"text", TEXTOID, 0, -1, F_TEXTIN, F_TEXTOUT},
    {"oid", OIDOID, 0, 4, F_OIDIN, F_OIDOUT},
    {"tid", TIDOID, 0, 6, F_TIDIN, F_TIDOUT},
    {"xid", XIDOID, 0, 4, F_XIDIN, F_XIDOUT},
    {"cid", CIDOID, 0, 4, F_CIDIN, F_CIDOUT},
    {"oidvector", 30, 0, INDEX_MAX_KEYS * 4, F_OIDVECTORIN, F_OIDVECTOROUT},
    {"smgr", 210, 0, 2, F_SMGRIN, F_SMGROUT},
    {"_int4", 1007, INT4OID, -1, F_ARRAY_IN, F_ARRAY_OUT},
    {"_aclitem", 1034, 1033, -1, F_ARRAY_IN, F_ARRAY_OUT}};

static int NTypes = sizeof(ProcID) / sizeof(struct TypeInfo);

typedef struct TypeMap {
  Oid am_oid;
  FormData_pg_type am_type;
} TypeMap;

static struct TypeMap** Type = NULL;
static struct TypeMap* Ap = NULL;
static int Warnings = 0;
static char Blanks[MAX_ATTR];
static char* CurRelName;           // Current relation name.
static MemoryContext NoGC = NULL;  // Special no-gc mem context.
static Datum Values[MAX_ATTR];     // Corresponding attribute values.

Form_pg_attribute AttrTypes[MAX_ATTR];  // Points to attribute info.
int NumAttr;                            // Number of attributes for cur. rel.
int DebugMode;

extern int optind;
extern char* optarg;

// At bootstrap time, we first declare all the indices to be built, and
// then build them.  The IndexList structure stores enough information
// to allow us to build the indices after they've been declared.
typedef struct IndexList {
  char* il_heap;
  char* il_ind;
  IndexInfo* il_info;
  struct IndexList* il_next;
} IndexList;

static IndexList* ILHead = NULL;

// The main loop for handling the backend in bootstrap mode
// the bootstrap mode is used to initialize the template database
// the bootstrap backend doesn't speak SQL, but instead expects
// commands in a special bootstrap language.
//
// The arguments passed in to BootstrapMain are the run-time arguments
// without the argument '-boot', the caller is required to have
// removed -boot from the run-time args
int bootstrap_main(int argc, char* argv[]) {
  int i;
  char* dbname;
  int flag;
  int xlog_op = BS_XLOG_NOP;
  char* optential_data_dir = NULL;

  // Initialize globals.
  MyProcPid = getpid();

  // Fire up essential subsystems: error and memory management
  //
  // If we are running under the postmaster, this is done already.
  if (!IsUnderPostmaster) {
    enable_exception_handling(true);
    memory_context_init();
  }

  // Process command arguments.
  // Set defaults, to be overriden by explicit options below.
  Quiet = false;
  Noversion = false;
  dbname = NULL;
}

void err_out() {
  Warnings++;
  clean_up();
}

// Assumes that 'oid' will not be zero.
void insert_one_tuple(Oid oid) {
  HeapTuple tuple;
  TupleDesc tup_desc;

  int i;

  // TODO(gc): 为什么根据oid不能得到NumAttr
  if (DebugMode) {
    printf("%s: oid %u, %d attrs\n", __func__, oid, NumAttr);
    fflush(stdout);
  }

  tup_desc = create_tuple_desc(NumAttr, AttrTypes);
}