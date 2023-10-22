#include "rdbms/c.h"
#include "rdbms/utils/hsearch.h"

#define MOD(x, y) ((x) & ((y)-1))

static long* dyna_hash_alloc(unsigned int size);
static void dyna_hash_free(Pointer ptr);
static uint32 call_hash(HTAB* hashp, char* k);
static SEG_OFFSET seg_alloc(HTAB* hashp);
static int bucket_alloc(HTAB* hashp);
static int dir_realloc(HTAB* hashp);
static int expand_table(HTAB* hashp);
static int hdefault(HTAB* hashp);
static int init_htab(HTAB* hashp, int nelem);

typedef long*((*dhalloc_ptr)());

// memory allocation routines
//
// for postgres: all hash elements have to be in
// the global cache context.  Otherwise the postgres
// garbage collector is going to corrupt them. -wei
//
// ??? the "cache" memory context is intended to store only
//  system cache information.  The user of the hashing
//  routines should specify which context to use or we
//  should create a separate memory context for these
//  hash routines.  For now I have modified this code to
//  do the latter -cim 1/19/91

GlobalMemory DynaHashCxt = (GlobalMemory)NULL;

#define MEM_ALLOC DynaHashAlloc
#define MEM_FREE  DynaHashFree

HTAB* hash_create(int nelem, HASHCTL* info, int flags) {
  HHDR* hctl;
  HTAB* hashp;
}