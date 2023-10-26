// =========================================================================
//
// dynahash.c
//  dynamic hashing
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/hash/dynahash.c,v 1.31 2000/04/12 17:16:00 momjian Exp $
//
// =========================================================================

// Dynamic hashing, after CACM April 1988 pp 446-457, by Per-Ake Larson.
// Coded into C, with minor code improvements, and with hsearch(3) interface,
// by ejp@ausmelb.oz, Jul 26, 1988: 13:16;
// also, hcreate/hdestroy routines added to simulate hsearch(3).
//
// These routines simulate hsearch(3) and family, with the important
// difference that the hash table is dynamic - can grow indefinitely
// beyond its original size (as supplied to hcreate()).
//
// Performance appears to be comparable to that of hsearch(3).
// The 'source-code' options referred to in hsearch(3)'s 'man' page
// are not implemented; otherwise functionality is identical.
//
// Compilation controls:
// DEBUG controls some informative traces, mainly for debugging.
// HASH_STATISTICS causes HashAccesses and HashCollisions to be maintained;
// when combined with HASH_DEBUG, these are displayed by hdestroy().
//
// Problems & fixes to ejp@ausmelb.oz. WARNING: relies on pre-processor
// concatenation property, in probably unnecessary code 'optimisation'.
//
// Modified margo@postgres.berkeley.edu February 1990
// added multiple table interface
// Modified by sullivan@postgres.berkeley.edu April 1990
// changed ctl structure for shared memory

#include "rdbms/c.h"
#include "rdbms/utils/hashfn.h"
#include "rdbms/utils/hsearch.h"
#include "rdbms/utils/mctx.h"
#include "rdbms/utils/memutils.h"

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

static long* dyna_hash_alloc(unsigned int size) {
  if (!DynaHashCxt) {
    DynaHashCxt = create_global_memory("DynaHash");
  }

  return (long*)memory_context_alloc((MemoryContext)DynaHashCxt, size);
}

static void dyna_hash_free(Pointer ptr) { memory_context_free((MemoryContext)DynaHashCxt, ptr); }

static uint32 call_hash(HTAB* hashp, char* k) {
  HHDR* hctl = hashp->hctl;
  long hash_val;
  long bucket;

  hash_val = hashp->hash(k, (int)hctl->keysize);
  bucket = hash_val & hctl->high_mask;

  if (bucket > hctl->max_bucket) {
    bucket = bucket & hctl->low_mask;
  }

  return bucket;
}

#define MEM_ALLOC dyna_hash_alloc
#define MEM_FREE  dyna_hash_free

// Pointer access macros.  Shared memory implementation cannot
// store pointers in the hash table data structures because
// pointer values will be different in different address spaces.
// these macros convert offsets to pointers and pointers to offsets.
// Shared memory need not be contiguous, but all addresses must be
// calculated relative to some offset (segbase).
#define GET_SEG(hp, seg_num)        (SEGMENT)(((unsigned long)(hp)->segbase) + (hp)->dir[seg_num])
#define GET_BUCKET(hp, bucket_offs) (ELEMENT*)(((unsigned long)(hp)->segbase) + bucket_offs)
#define MAKE_HASHOFFSET(hp, ptr)    (((unsigned long)ptr) - ((unsigned long)(hp)->segbase))

static long HashAccesses;
static long HashCollisions;
static long HashExpansions;

HTAB* hash_create(int nelem, HASHCTL* info, int flags) {
  HHDR* hctl;
  HTAB* hashp;

  hashp = (HTAB*)MEM_ALLOC(sizeof(HTAB));
  MemSet(hashp, 0, sizeof(HTAB));

  if (flags & HASH_FUNCTION) {
    hashp->hash = info->hash;
  } else {
    hashp->hash = string_hash;
  }

  if (flags & HASH_SHARED_MEM) {
    // ctl structure is preallocated for shared memory tables. Note
    // that HASH_DIRSIZE had better be set as well.
    hashp->hctl = (HHDR*)info->hctl;
    hashp->segbase = (char*)info->segbase;
    hashp->alloc = info->alloc;
    hashp->dir = (SEG_OFFSET*)info->dir;

    // Hash table already exists, we're just attaching to it.
    if (flags & HASH_ATTACH) {
      return hashp;
    }
  } else {
    hashp->hctl = NULL;
    hashp->alloc = (dhalloc_ptr)MEM_ALLOC;
    hashp->dir = NULL;
    hashp->segbase = NULL;
  }

  if (!hashp->hctl) {
    hashp->hctl = (HHDR*)hashp->alloc(sizeof(HHDR));

    // TODO(gc): need to log this information.
    if (!hashp->hctl) {
      return NULL;
    }
  }

  if (!hdefault(hashp)) {
    return NULL;
  }

  hctl = hashp->hctl;
  hctl->accesses = 0;
  hctl->collisions = 0;

  if (flags & HASH_SEGMENT) {
    hctl->ssize = info->ssize;
    hctl->sshift = my_log2(info->ssize);

    assert(hctl->ssize == (1L << hctl->sshift));
  }

  if (flags & HASH_FFACTOR) {
    hctl->ffactor = info->ffactor;
  }

  // SHM hash tables have fixed directory size passed by the caller.
  if (flags & HASH_DIRSIZE) {
    hctl->max_dsize = info->max_dsize;
    hctl->dsize = info->dsize;
  }

  // Hash table now allocates space for key and data but you have to say
  // how much space to allocate.
  if (flags & HASH_ELEM) {
    hctl->keysize = info->keysize;
    hctl->datasize = info->datasize;
  }

  if (flags & HASH_ALLOC) {
    hashp->alloc = info->alloc;
  }

  if (init_htab(hashp, nelem)) {
    hash_destroy(hashp);

    return NULL;
  }

  return hashp;
}

// XXX this sure looks thoroughly broken to me --- tgl 2/99.
// It's freeing every entry individually --- but they weren't
// allocated individually, see bucket_alloc!!  Why doesn't it crash?
// ANSWER: it probably does crash, but is never invoked in normal
// operations...
// TODO(gc): double check this.
void hash_destroy(HTAB* hashp) {
  if (hashp == NULL) {
    return;
  }

  SEG_OFFSET seg_num;
  SEGMENT segp;
  int nsegs = hashp->hctl->nsegs;
  int j;
  BUCKET_INDEX* elp;
  BUCKET_INDEX p;
  BUCKET_INDEX q;
  ELEMENT* curr;

  // Can't destroy a shared memory hash table.
  assert(!hashp->segbase);
  // Allocation method must be one we know how to free, too。
  assert(hashp->alloc == (dhalloc_ptr)MEM_ALLOC);

  hash_stats("destroy", hashp);

  for (seg_num = 0; nsegs > 0; nsegs--, seg_num++) {
    segp = GET_SEG(hashp, seg_num);

    for (j = 0, elp = segp; j < hashp->hctl->ssize; j++, elp++) {
      for (p = *elp; p != INVALID_INDEX; p = q) {
        curr = GET_BUCKET(hashp, p);
        q = curr->next;
        MEM_FREE((char*)curr);
      }
    }

    MEM_FREE((char*)segp);
  }

  MEM_FREE((char*)hashp->dir);
  MEM_FREE((char*)hashp->hctl);
  MEM_FREE((char*)hashp);
}

void hash_stats(char* where, HTAB* hashp) {
#if HASH_STATISTICS

  fprintf(stderr, "%s: this HTAB -- accesses %ld collisions %ld\n", where, hashp->hctl->accesses,
          hashp->hctl->collisions);

  fprintf(stderr, "hash_stats: keys %ld keysize %ld maxp %d segmentcount %d\n", hashp->hctl->nkeys,
          hashp->hctl->keysize, hashp->hctl->max_bucket, hashp->hctl->nsegs);
  fprintf(stderr, "%s: total accesses %ld total collisions %ld\n", where, hash_accesses, hash_collisions);
  fprintf(stderr, "hash_stats: total expansions %ld\n", hash_expansions);

#endif
}

// hash_search -- look up key in table and perform action
//
// action is one of HASH_FIND/HASH_ENTER/HASH_REMOVE
//
// RETURNS: NULL if table is corrupted, a pointer to the element
//  found/removed/entered if applicable, TRUE otherwise.
//  foundPtr is TRUE if we found an element in the table
//  (FALSE if we entered one).
long* hash_search(HTAB* hashp, char* key_ptr, HASHACTION action, bool* found_ptr) {
  assert(PointerIsValid(hashp) && PointerIsValid(key_ptr));
  assert((action == HASH_FIND) || (action == HASH_REMOVE) || (action == HASH_ENTER) || (action == HASH_FIND_SAVE) ||
         (action == HASH_REMOVE_SAVED));

  uint32 bucket;
  long segment_num;
  long segment_ndx;
  SEGMENT segp;
  ELEMENT* curr;
  HHDR* hctl;
  BUCKET_INDEX curr_index;
  BUCKET_INDEX* prev_index_ptr;
  char* dest_addr;

  static struct State {
    ELEMENT* curr_elem;
    BUCKET_INDEX curr_index;
    BUCKET_INDEX* prev_index;
  } save_state;

  hctl = hashp->hctl;

#if HASH_STATISTICS
  hash_accesses++;
  hashp->hctl->accesses++;
#endif

  if (action == HASH_REMOVE_SAVED) {
  } else {
    bucket = call_hash(hashp, key_ptr);
    segment_num = bucket >> hctl->sshift;
    segment_ndx = MOD(bucket, hctl->ssize);
    segp = GET_SEG(hashp, segment_num);

    assert(segp != NULL);

    prev_index_ptr = &segp[segment_ndx];
    curr_index = *prev_index_ptr;

    // Follow collision chain.
    for (curr = NULL; curr_index != INVALID_INDEX;) {
      // Coerce bucket index into a pointer.
      curr = GET_BUCKET(hashp, curr_index);

      // TODO(gc): it looks weired, curr->key is type of long.
      if (!memcmp((char*)&(curr->key), key_ptr, hctl->keysize)) {
        break;
      }

      prev_index_ptr = &(curr->next);
      curr_index = *prev_index_ptr;

#if HASH_STATISTICS
      hash_collisions++;
      hashp->hctl->collisions++;
#endif
    }
  }

  // If we found an entry or if we weren't trying to insert, we're done now.
  *found_ptr = curr_index != INVALID_INDEX;

  switch (action) {
    case HASH_ENTER:
      if (curr_index != INVALID_INDEX) {
        return &(curr->key);
      }

      break;

    case HASH_REMOVE:
    case HASH_REMOVE_SAVED:
      if (curr_index != INVALID_INDEX) {
        assert(hctl->nkeys > 0);
        hctl->nkeys--;

        // Remove record from hash bucket's chain.
        *prev_index_ptr = curr->next;
        // Add the record to the freelist for this table.
        curr->next = hctl->free_bucket_index;
        hctl->free_bucket_index = curr_index;

        // Better hope the caller is synchronizing access to this
        // element, because someone else is going to reuse it the
        // next time something is added to the table.
        return &(curr->key);
      }

      return (long*)true;

    case HASH_FIND:
      if (curr_index != INVALID_INDEX) {
        return &(curr->key);
      }

      return (long*)true;

    case HASH_FIND_SAVE:
      if (curr_index != INVALID_INDEX) {
        save_state.curr_elem = curr;
        save_state.prev_index = prev_index_ptr;
        save_state.curr_index = curr_index;

        return &(curr->key);
      }

      return (long*)true;

    default:
      return NULL;
  }

  //  If we got here, then we didn't find the element and we have to
  // Insert it into the hash table.
  assert(curr_index == INVALID_INDEX);

  // Get the next free bucket.
  curr_index = hctl->free_bucket_index;

  if (curr_index == INVALID_INDEX) {
    if (!bucket_alloc(hashp)) {
      return NULL;
    }

    curr_index = hctl->free_bucket_index;
  }

  assert(curr_index == INVALID_INDEX);

  curr = GET_BUCKET(hashp, curr_index);
  hctl->free_bucket_index = curr->next;

  // Link into chain.
  *prev_index_ptr = curr_index;

  // Copy key into record.
  dest_addr = (char*)&(curr->key);
  memmove(dest_addr, key_ptr, hctl->keysize);
  curr->next = INVALID_INDEX;

  if (++hctl->nkeys / (hctl->max_bucket + 1) > hctl->ffactor) {
    // NOTE: failure to expand table is not a fatal error, it just
    // means we have to run at higher fill factor than we wanted.
    expand_table(hashp);
  }

  return &(curr->key);
}

// hash_seq
//
//  sequentially search through hash table and return
//  all the elements one by one, return NULL on error and
//  return TRUE in the end.
long* hash_seq(HTAB* hashp) {
  static long S_CurBucket = 0;
  static BUCKET_INDEX S_CurIndex;
  ELEMENT* cur_elem;
  long segment_num;
  long segment_ndx;
  SEGMENT segp;
  HHDR* hctl;

  if (hashp == NULL) {
    S_CurBucket = 0;
    S_CurIndex = INVALID_INDEX;

    return NULL;
  }

  hctl = hashp->hctl;

  while (S_CurBucket <= hctl->max_bucket) {
    if (S_CurIndex != INVALID_INDEX) {
      cur_elem = GET_BUCKET(hashp, S_CurIndex);
      S_CurIndex = cur_elem->next;

      if (S_CurIndex == INVALID_INDEX) {
        ++S_CurBucket;
      }

      return &(cur_elem->key);
    }

    // Initialize the search within this bucket.
    segment_num = S_CurBucket >> hctl->sshift;
    segment_ndx = MOD(S_CurBucket, hctl->ssize);

    // First find the right segment in the table directory.
    segp = GET_SEG(hashp, segment_num);

    if (segp == NULL) {
      return NULL;
    }

    // Now find the right index into the segment for the first item in
    // this bucket's chain.  if the bucket is not empty (its entry in
    // the dir is valid), we know this must correspond to a valid
    // element and not a freed element because it came out of the
    // directory of valid stuff.  if there are elements in the bucket
    // chains that point to the freelist we're in big trouble.
    S_CurIndex = segp[segment_ndx];

    if (S_CurIndex == INVALID_INDEX) {
      ++S_CurBucket;
    }
  }

  return (long*)true;
}

long hash_estimate_size(long num_entries, long keysize, long datasize) {
  long size = 0;
  long nbuckets;
  long nsegments;
  long ndir_entries;
  long nrecord_allocs;
  long record_size;

  // Estimate number of buckets wanted.
  nbuckets = 1L << my_log2((num_entries - 1) / DEF_FFACTOR + 1);
  // # of segments needed for nBuckets.
  nsegments = 1L << my_log2((nbuckets - 1) / DEF_SEGSIZE + 1);
  // Directory entries.
  ndir_entries = DEF_DIRSIZE;
  while (ndir_entries < nsegments) {
    // dir_alloc doubles dsize at each call
    ndir_entries <<= 1;
  }

  // Dixed control info.
  size += MAXALIGN(sizeof(HHDR));  // But not HTAB, per above.
  // Directory.
  size += MAXALIGN(ndir_entries * sizeof(SEG_OFFSET));
  // Segments.
  size += nsegments * MAXALIGN(DEF_SEGSIZE * sizeof(BUCKET_INDEX));
  // Records --- allocated in groups of BUCKET_ALLOC_INCR.
  record_size = sizeof(BUCKET_INDEX) + keysize + datasize;
  record_size = MAXALIGN(record_size);
  nrecord_allocs = (num_entries - 1) / BUCKET_ALLOC_INCR + 1;
  size += nrecord_allocs * BUCKET_ALLOC_INCR * record_size;

  return size;
}

// Select an appropriate directory size for a hashtable with the given
// maximum number of entries.
// This is only needed for hashtables in shared memory, whose directories
// cannot be expanded dynamically.
// NB: assumes that all hash structure parameters have default values!
//
// XXX this had better agree with the behavior of init_htab()...
long hash_select_dirsize(long num_entries) {
  long nbuckets;
  long nsegments;
  long ndir_entries;

  nbuckets = 1L << my_log2((num_entries - 1) / DEF_FFACTOR + 1);
  nsegments = 1L << my_log2((nbuckets - 1) / DEF_SEGSIZE + 1);
  ndir_entries = DEF_DIRSIZE;

  while (ndir_entries < nsegments) {
    ndir_entries <<= 1;
  }

  return ndir_entries;
}

static SEG_OFFSET seg_alloc(HTAB* hashp) {
  SEGMENT segp;
  SEG_OFFSET seg_offset;

  segp = (SEGMENT)hashp->alloc(sizeof(BUCKET_INDEX) * hashp->hctl->ssize);

  if (!segp) {
    return NULL;
  }

  MemSet((char*)segp, 0, (long)sizeof(BUCKET_INDEX) * hashp->hctl->ssize);
  seg_offset = MAKE_HASHOFFSET(hashp, segp);

  return seg_offset;
}

static int bucket_alloc(HTAB* hashp) {
  int i;
  ELEMENT* tmp_bucket;
  long bucket_size;
  BUCKET_INDEX tmp_index;
  BUCKET_INDEX last_index;

  // Each bucket has a BUCKET_INDEX header plus user data.
  bucket_size = sizeof(BUCKET_INDEX) + hashp->hctl->keysize + hashp->hctl->datasize;
  // Make sure its aligned correctly.
  bucket_size = MAXALIGN(bucket_size);
  tmp_bucket = (ELEMENT*)hashp->alloc((unsigned long)BUCKET_ALLOC_INCR * bucket_size);

  if (!tmp_bucket) {
    return 0;
  }

  // `tmp_index` is the shmem offset into the first bucket of the array.
  tmp_index = MAKE_HASHOFFSET(hashp, tmp_bucket);
  // Set the freebucket list to point to the first bucket.
  last_index = hashp->hctl->free_bucket_index;
  hashp->hctl->free_bucket_index = tmp_index;

  // Initialize each bucket to point to the one behind it.
  // NOTE: loop sets last bucket incorrectly; we fix below.
  for (i = 0; i < BUCKET_ALLOC_INCR; i++) {
    tmp_bucket = GET_BUCKET(hashp, tmp_index);
    tmp_index += bucket_size;
    tmp_bucket->next = tmp_index;
  }

  // The last bucket points to the old freelist head (which is probably
  // invalid or we wouldn't be here).
  tmp_bucket->next = last_index;

  return 1;
}

static int dir_realloc(HTAB* hashp) {
  char* p;
  char* old_p;
  long new_dsize;
  long old_dirsize;
  long new_dirsize;

  if (hashp->hctl->max_dsize != NO_MAX_DSIZE) {
    return 0;
  }

  /* Reallocate directory */
  new_dsize = hashp->hctl->dsize << 1;
  old_dirsize = hashp->hctl->dsize * sizeof(SEG_OFFSET);
  new_dirsize = new_dsize * sizeof(SEG_OFFSET);

  old_p = (char*)hashp->dir;
  p = (char*)hashp->alloc((unsigned long)new_dirsize);

  if (p != NULL) {
    memmove(p, old_p, old_dirsize);
    MemSet(p + old_dirsize, 0, new_dirsize - old_dirsize);
    MEM_FREE((char*)old_p);
    hashp->dir = (SEG_OFFSET*)p;
    hashp->hctl->dsize = new_dsize;
    return 1;
  }

  return 0;
}

// Expand the table by adding one more hash bucket.
static int expand_table(HTAB* hashp) {
  HHDR* hctl;
  SEGMENT old_seg;
  SEGMENT new_seg;
  long old_bucket;
  long new_bucket;
  long new_segnum;
  long new_segndx;
  long old_segnum;
  long old_segndx;
  ELEMENT* chain;
  BUCKET_INDEX* old;
  BUCKET_INDEX* newbi;
  BUCKET_INDEX* chain_index;
  BUCKET_INDEX* next_index;

#ifdef HASH_STATISTICS
  hash_expansions++;
#endif

  hctl = hashp->hctl;
  new_bucket = hctl->max_bucket + 1;
  new_segnum = new_bucket >> hctl->sshift;
  new_segndx = MOD(new_bucket, hctl->ssize);

  if (new_segnum >= hctl->nsegs) {
    // Allocate new segment if necessary -- could fail if dir full.
    if (new_segnum >= hctl->dsize) {
      if (!dir_realloc(hashp)) {
        return 0;
      }
    }

    if (!(hashp->dir[new_segnum] = seg_alloc(hashp))) {
      return 0;
    }

    hctl->nsegs++;
  }

  // OK. We created a new bucket.
  hctl->max_bucket++;

  // Before changing masks, find old bucket corresponding to same hash
  // values; values in that bucket may need to be relocated to new
  // bucket. Note that new_bucket is certainly larger than low_mask at
  // this point, so we can skip the first step of the regular hash mask
  // calc.
  old_bucket = (new_bucket & hctl->low_mask);

  // If we crossed a power of 2, readjust masks.
  if (new_bucket > hctl->high_mask) {
    hctl->low_mask = hctl->high_mask;
    hctl->high_mask = new_bucket | hctl->low_mask;
  }

  // Relocate records to the new bucket.	NOTE: because of the way the
  // hash masking is done in call_hash, only one old bucket can need to
  // be split at this point.	With a different way of reducing the hash
  // value, that might not be true!
  old_segnum = old_bucket >> hctl->sshift;
  old_segndx = MOD(old_bucket, hctl->ssize);

  old_seg = GET_SEG(hashp, old_segnum);
  new_seg = GET_SEG(hashp, new_segnum);

  old = &old_seg[old_segndx];
  newbi = &new_seg[new_segndx];

  for (chain_index = *old; chain_index != INVALID_INDEX; chain_index = next_index) {
    chain = GET_BUCKET(hashp, chain_index);
    next_index = chain->next;
    if ((long)call_hash(hashp, (char*)&(chain->key)) == old_bucket) {
      *old = chain_index;
      old = &chain->next;
    } else {
      *newbi = chain_index;
      newbi = &chain->next;
    }
  }

  // Don't forget to terminate the rebuilt hash chains.
  *old = INVALID_INDEX;
  *newbi = INVALID_INDEX;

  return 1;
}

// Set default HHDR parameters.
static int hdefault(HTAB* hashp) {
  HHDR* hctl;

  MemSet(hashp->hctl, 0, sizeof(HHDR));

  hctl = hashp->hctl;
  hctl->ssize = DEF_SEGSIZE;
  hctl->sshift = DEF_SEGSIZE_SHIFT;
  hctl->dsize = DEF_DIRSIZE;
  hctl->ffactor = DEF_FFACTOR;
  hctl->nkeys = 0;
  hctl->nsegs = 0;
  // Default memory allocation for hash buckets.
  hctl->keysize = sizeof(char*);
  hctl->datasize = sizeof(char*);

  // Table has no fixed maximum size.
  hctl->max_dsize = NO_MAX_DSIZE;

  // Garbage collection for HASH_REMOVE.
  hctl->free_bucket_index = INVALID_INDEX;

  return 1;
}

static int init_htab(HTAB* hashp, int nelem) {
  SEG_OFFSET* segp;

  int nbuckets;
  int nsegs;
  HHDR* hctl;

  hctl = hashp->hctl;

  // Divide number of elements by the fill factor to determine a desired
  // number of buckets.  Allocate space for the next greater power of
  // two number of buckets.
  nelem = (nelem - 1) / hctl->ffactor + 1;

  nbuckets = 1 << my_log2(nelem);

  hctl->max_bucket = hctl->low_mask = nbuckets - 1;
  hctl->high_mask = (nbuckets << 1) - 1;

  // Figure number of directory segments needed, round up to a power of 2.
  nsegs = (nbuckets - 1) / hctl->ssize + 1;
  nsegs = 1 << my_log2(nsegs);

  // Make sure directory is big enough. If pre-allocated directory is
  // too small, choke (caller screwed up).
  if (nsegs > hctl->dsize) {
    if (!(hashp->dir)) {
      hctl->dsize = nsegs;
    } else {
      return -1;
    }
  }

  // Allocate a directory.
  if (!(hashp->dir)) {
    hashp->dir = (SEG_OFFSET*)hashp->alloc(hctl->dsize * sizeof(SEG_OFFSET));

    if (!hashp->dir) {
      return -1;
    }
  }

  // Allocate initial segments.
  for (segp = hashp->dir; hctl->nsegs < nsegs; hctl->nsegs++, segp++) {
    *segp = seg_alloc(hashp);

    if (*segp == (SEG_OFFSET)0) {
      return -1;
    }
  }

#if HASH_DEBUG
  fprintf(stderr, "%s\n%s%x\n%s%d\n%s%d\n%s%d\n%s%d\n%s%d\n%s%x\n%s%x\n%s%d\n%s%d\n", "init_htab:", "TABLE POINTER   ",
          hashp, "DIRECTORY SIZE  ", hctl->dsize, "SEGMENT SIZE    ", hctl->ssize, "SEGMENT SHIFT   ", hctl->sshift,
          "FILL FACTOR     ", hctl->ffactor, "MAX BUCKET      ", hctl->max_bucket, "HIGH MASK       ", hctl->high_mask,
          "LOW  MASK       ", hctl->low_mask, "NSEGS           ", hctl->nsegs, "NKEYS           ", hctl->nkeys);
#endif

  return 0;
}

int my_log2(long num) {
  int i;
  long limit;

  for (i = 0, limit = 1; limit < num; i++, limit <<= 1)
    ;
  return i;
}