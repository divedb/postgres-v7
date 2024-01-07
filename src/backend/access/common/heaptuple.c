//===----------------------------------------------------------------------===//
//
// heaptuple.c
//  This file contains heap tuple accessor and mutator routines, as well
//  as various tuple utilities.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/access/common/heaptuple.c,v 1.71
//  2001/03/22 06:16:06 momjian Exp $
//
// NOTES
//  The old interface functions have been converted to macros
//  and moved to heapam.h
//
//===----------------------------------------------------------------------===//
#include "rdbms/access/heapam.h"
#include "rdbms/catalog/pg_type.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"
#include "rdbms/utils/memutils.h"

extern MemoryContext CurrentMemoryContext;

Size compute_data_size(TupleDesc tuple_desc, Datum* value, char* nulls) {
  uint32 data_length = 0;
  int i;
  int number_of_attributes = tuple_desc->natts;
  Form_pg_attribute* att = tuple_desc->attrs;

  for (i = 0; i < number_of_attributes; i++) {
    if (nulls[i] != ' ') {
      continue;
    }

    data_length = ATT_ALIGN(data_length, att[i]->attlen, att[i]->attalign);
    data_length = ATT_ADD_LENGTH(data_length, att[i]->attlen, value[i]);
  }

  return data_length;
}

void data_fill(char* data, TupleDesc tuple_desc, Datum* value, char* nulls,
               uint16* infomask, bits8* bit) {
  bits8* bitp = NULL;
  int bitmask = 0;
  uint32 data_length;
  int i;
  int number_of_attributes = tuple_desc->natts;
  Form_pg_attribute* att = tuple_desc->attrs;

  if (bit != NULL) {
    bitp = &bit[-1];
    bitmask = CSIGNBIT;
  }

  *infomask &= HEAP_XACT_MASK;

  for (i = 0; i < number_of_attributes; i++) {
    if (bit != NULL) {
      if (bitmask != CSIGNBIT) {
        bitmask <<= 1;
      } else {
        bitp += 1;
        *bitp = 0x0;
        bitmask = 1;
      }

      if (nulls[i] == 'n') {
        *infomask |= HEAP_HASNULL;
        continue;
      }

      *bitp |= bitmask;
    }

    // XXX we are aligning the pointer itself, not the offset.
    data = (char*)ATT_ALIGN((long)data, att[i]->attlen, att[i]->attalign);

    if (att[i]->attbyval) {
      // Pass-by-value.
      STORE_ATT_BY_VAL(data, value[i], att[i]->attlen);
    } else if (att[i]->attlen == -1) {
      // varlena.
      *infomask |= HEAP_HASVARLENA;
      if (VARATT_IS_EXTERNAL(value[i])) *infomask |= HEAP_HASEXTERNAL;
      if (VARATT_IS_COMPRESSED(value[i])) *infomask |= HEAP_HASCOMPRESSED;
      data_length = VARATT_SIZE(DatumGetPointer(value[i]));
      memcpy(data, DatumGetPointer(value[i]), data_length);
    } else {
      // Fixed-length pass-by-reference.
      ASSERT(att[i]->attlen >= 0);
      memcpy(data, DATUM_GET_POINTER(value[i]), (size_t)(att[i]->attlen));
    }

    data = (char*)ATT_ADD_LENGTH((long)data, att[i]->attlen, value[i]);
  }
}

int heap_att_is_null(HeapTuple tup, int attnum) {
  if (attnum > (int)tup->t_data->t_natts) {
    return 1;
  }

  if (HEAP_TUPLE_NO_NULLS(tup)) {
    return 0;
  }

  if (attnum > 0) {
    return ATT_IS_NULL(attnum - 1, tup->t_data->t_bits);
  } else {
    switch (attnum) {
      case TABLE_OID_ATTRIBUTE_NUMBER:
      case SELF_ITEM_POINTER_ATTRIBUTE_NUMBER:
      case OBJECT_ID_ATTRIBUTE_NUMBER:
      case MIN_TRANSACTION_ID_ATTRIBUTE_NUMBER:
      case MIN_COMMAND_ID_ATTRIBUTE_NUMBER:
      case MAX_TRANSACTION_ID_ATTRIBUTE_NUMBER:
      case MAX_COMMAND_ID_ATTRIBUTE_NUMBER:
        break;

      case 0:
        elog(ERROR, "%s: zero attnum disallowed", __func__);

      default:
        elog(ERROR, "%s: undefined negative attnum", __func__);
    }
  }

  return 0;
}

// This only gets called from fastgetattr() macro, in cases where
// we can't use a cacheoffset and the value is not null.
//
// This caches attribute offsets in the attribute descriptor.
//
// An alternate way to speed things up would be to cache offsets
// with the tuple, but that seems more difficult unless you take
// the storage hit of actually putting those offsets into the
// tuple you send to disk.  Yuck.
//
// This scheme will be slightly slower than that, but should
// perform well for queries which hit large #'s of tuples.  After
// you cache the offsets once, examining all the other tuples using
// the same attribute descriptor will go much quicker. -cim 5/4/91
Datum no_cache_get_attr(HeapTuple tuple, int attnum, TupleDesc tuple_desc,
                        bool* isnull) {
  HeapTupleHeader tup = tuple->t_data;
  Form_pg_attribute* att = tuple_desc->attrs;
  char* tp;                 // Ptr to att in tuple
  bits8* bp = tup->t_bits;  // Ptr to null bitmask in tuple
  bool slow = false;        // Do we have to walk nulls?

  (void)isnull;  // not used

#ifdef IN_MACRO
  // This is handled in the macro
  ASSERT(attnum > 0);

  if (isnull) {
    *isnull = false;
  }
#endif

  attnum--;

  // Three cases:
  //
  // 1: No nulls and no variable length attributes.
  // 2: Has a null or a varlena AFTER att.
  // 3: Has nulls or varlenas BEFORE att.
  if (HEAP_TUPLE_NO_NULLS(tuple)) {
#ifdef IN_MACRO
    // This is handled in the macro.
    if (att[attnum]->attcacheoff != -1) {
      return FETCH_ATT(att[attnum],
                       (char*)tup + tup->t_hoff + att[attnum]->attcacheoff);
    }
#endif
  } else {
    // There's a null somewhere in the tuple

    // Check to see if desired att is null

#ifdef IN_MACRO
    // This is handled in the macro
    if (ATT_IS_NULL(attnum, bp)) {
      if (isnull) {
        *isnull = true;
      }
      return (Datum)NULL;
    }
#endif

    // Now check to see if any preceding bits are null...
    {
      int byte = attnum >> 3;
      int finalbit = attnum & 0x07;

      // Check for nulls "before" final bit of last byte.
      if ((~bp[byte]) & ((1 << finalbit) - 1))
        slow = true;
      else {
        // Check for nulls in any "earlier" bytes.
        int i;

        for (i = 0; i < byte; i++) {
          if (bp[i] != 0xFF) {
            slow = true;
            break;
          }
        }
      }
    }
  }

  tp = (char*)tup + tup->t_hoff;

  // Now check for any non-fixed length attrs before our attribute
  if (!slow) {
    if (att[attnum]->attcacheoff != -1) {
      return fetchatt(att[attnum], tp + att[attnum]->attcacheoff);
    } else if (!HeapTupleAllFixed(tuple)) {
      int j;

      // In for(), we test <= and not < because we want to see if we
      // can go past it in initializing offsets.
      for (j = 0; j <= attnum; j++) {
        if (att[j]->attlen <= 0) {
          slow = true;
          break;
        }
      }
    }
  }

  // If slow is false, and we got here, we know that we have a tuple
  // with no nulls or varlenas before the target attribute. If possible,
  // we also want to initialize the remainder of the attribute cached
  // offset values.
  if (!slow) {
    int j = 1;
    long off;

    // Need to set cache for some atts
    att[0]->attcacheoff = 0;

    while (j < attnum && att[j]->attcacheoff > 0) j++;

    off = att[j - 1]->attcacheoff + att[j - 1]->attlen;

    for (; j <= attnum ||
           // Can we compute more?  We will probably need them.
           (j < tup->t_natts && att[j]->attcacheoff == -1 &&
            (HEAP_TUPLE_NO_NULLS(tuple) || !ATT_IS_NULL(j, bp)) &&
            (HEAP_TUPLE_ALL_FIXED(tuple) || att[j]->attlen > 0));
         j++) {
      // TODO(gc): Fix me when going to a machine with more than a four-byte
      // word!
      off = ATT_ALIGN(off, att[j]->attlen, att[j]->attalign);

      att[j]->attcacheoff = off;

      off = ATT_ADD_LENGTH(off, att[j]->attlen, tp + off);
    }

    return FETCH_ATT(att[attnum], tp + att[attnum]->attcacheoff);
  } else {
    bool usecache = true;
    int off = 0;
    int i;

    // Now we know that we have to walk the tuple CAREFULLY.
    //
    // Note - This loop is a little tricky.  On iteration i we first set
    // the offset for attribute i and figure out how much the offset
    // should be incremented.  Finally, we need to align the offset
    // based on the size of attribute i+1 (for which the offset has
    // been computed). -mer 12 Dec 1991
    for (i = 0; i < attnum; i++) {
      if (!HEAP_TUPLE_NO_NULLS(tuple)) {
        if (ATT_IS_NULL(i, bp)) {
          usecache = false;
          continue;
        }
      }

      // If we know the next offset, we can skip the rest.
      if (usecache && att[i]->attcacheoff != -1)
        off = att[i]->attcacheoff;
      else {
        off = ATT_ALIGN(off, att[i]->attlen, att[i]->attalign);

        if (usecache) {
          att[i]->attcacheoff = off;
        }
      }

      off = ATT_ADD_LENGTH(off, att[i]->attlen, tp + off);

      if (usecache && att[i]->attlen == -1) {
        usecache = false;
      }
    }

    off = ATT_ALIGN(off, att[attnum]->attlen, att[attnum]->attalign);

    return FETCH_ATT(att[attnum], tp + off);
  }
}

HeapTuple heap_copy_tuple(HeapTuple tuple) {
  HeapTuple new_tuple;

  if (!HEAP_TUPLE_IS_VALID(tuple) || tuple->t_data == NULL) {
    return NULL;
  }

  new_tuple = (HeapTuple)palloc(HEAP_TUPLE_SIZE + tuple->t_len);
  new_tuple->t_len = tuple->t_len;
  new_tuple->t_self = tuple->t_self;
  new_tuple->t_table_oid = tuple->t_table_oid;
  new_tuple->t_data_mcxt = CurrentMemoryContext;
  new_tuple->t_data = (HeapTupleHeader)((char*)new_tuple + HEAP_TUPLE_SIZE);
  memcpy((char*)new_tuple->t_data, (char*)tuple->t_data, tuple->t_len);

  return new_tuple;
}

void heap_copy_tuple_with_tuple(HeapTuple src, HeapTuple dest) {
  if (!HEAP_TUPLE_IS_VALID(src) || src->t_data == NULL) {
    dest->t_data = NULL;
    return;
  }

  dest->t_len = src->t_len;
  dest->t_self = src->t_self;
  dest->t_table_oid = src->t_table_oid;
  dest->t_data_mcxt = CurrentMemoryContext;
  dest->t_data = (HeapTupleHeader)palloc(src->t_len);
  memcpy((char*)dest->t_data, (char*)src->t_data, src->t_len);
}

// Constructs a tuple from the given *value and *null arrays
//
// Old comments
// Handles alignment by aligning 2 byte attributes on short boundries
// and 3 or 4 byte attributes on long word boundries on a vax; and
// aligning non-byte attributes on short boundries on a sun.  Does
// not properly align fixed length arrays of 1 or 2 byte types (yet).
//
// Null attributes are indicated by a 'n' in the appropriate byte
// of the *null. Non-null attributes are indicated by a ' ' (space).
//
// Fix me. (Figure that must keep context if debug--allow give oid.)
// Assumes in order.
HeapTuple heap_form_tuple(TupleDesc tuple_desc, Datum* value, char* nulls) {
  HeapTuple tuple;     // Return tuple
  HeapTupleHeader td;  // Tuple data
  int bitmap_len;
  unsigned long len;
  int hoff;
  bool has_null = false;
  int i;
  int number_of_attributes = tuple_desc->natts;

  if (number_of_attributes > MAX_HEAP_ATTRIBUTE_NUMBER) {
    elog(ERROR, "heap_formtuple: numberOfAttributes of %d > %d",
         number_of_attributes, MAX_HEAP_ATTRIBUTE_NUMBER);
  }

  len = offsetof(HeapTupleHeaderData, t_bits);

  for (i = 0; i < number_of_attributes; i++) {
    if (nulls[i] != ' ') {
      has_null = true;
      break;
    }
  }

  if (has_null) {
    bitmap_len = BIT_MAP_LEN(number_of_attributes);
    len += bitmap_len;
  }

  hoff = len = MAX_ALIGN(len);  // Be conservative here.

  len += ComputeDataSize(tuple_desc, value, nulls);

  tuple = (HeapTuple)palloc(HEAP_TUPLE_SIZE + len);
  tuple->t_data_mcxt = CurrentMemoryContext;
  td = tuple->t_data = (HeapTupleHeader)((char*)tuple + HEAP_TUPLE_SIZE);

  MemSet((char*)td, 0, len);

  tuple->t_len = len;
  ItemPointerSetInvalid(&(tuple->t_self));
  tuple->t_table_oid = INVALID_OID;
  td->t_natts = number_of_attributes;
  td->t_hoff = hoff;

  DataFill((char*)td + td->t_hoff, tuple_desc, value, nulls, &td->t_info_mask,
           (has_null ? td->t_bits : NULL));

  td->t_info_mask |= HEAP_XMAX_INVALID;

  return tuple;
}

HeapTuple heap_modify_tuple(HeapTuple tuple, Relation relation,
                            Datum* repl_value, char* repl_null, char* repl) {
  int attoff;
  int number_of_attributes;
  Datum* value;
  char* nulls;
  bool isNull;
  HeapTuple new_tuple;
  uint8 infomask;

  // Sanity checks.
  ASSERT(HEAP_TUPLE_IS_VALID(tuple));
  ASSERT(RELATION_IS_VALID(relation));
  ASSERT(POINTER_IS_VALID(repl_value));
  ASSERT(POINTER_IS_VALID(repl_null));
  ASSERT(POINTER_IS_VALID(repl));

  number_of_attributes = RELATION_GET_FORM(relation)->relnatts;

  // Allocate and fill *value and *nulls arrays from either the tuple or
  // the repl information, as appropriate.
  value = (Datum*)palloc(number_of_attributes * sizeof *value);
  nulls = (char*)palloc(number_of_attributes * sizeof *nulls);

  for (attoff = 0; attoff < number_of_attributes; attoff += 1) {
    if (repl[attoff] == ' ') {
      value[attoff] = heap_getattr(tuple, AttrOffsetGetAttrNumber(attoff),
                                   RelationGetDescr(relation), &isNull);
      nulls[attoff] = (isNull) ? 'n' : ' ';

    } else if (repl[attoff] != 'r')
      elog(ERROR, "heap_modifytuple: repl is \\%3d", repl[attoff]);
    else {  // == 'r'
      value[attoff] = repl_value[attoff];
      nulls[attoff] = repl_null[attoff];
    }
  }

  // Create a new tuple from the *values and *nulls arrays.
  new_tuple = heap_formtuple(RelationGetDescr(relation), value, nulls);

  // copy the header except for t_len, t_natts, t_hoff, t_bits,
  // t_infomask
  infomask = new_tuple->t_data->t_info_mask;
  memmove(
      (char*)&new_tuple->t_data->t_oid,  // XXX
      (char*)&tuple->t_data->t_oid,
      ((char*)&tuple->t_data->t_hoff - (char*)&tuple->t_data->t_oid));  // XXX
  new_tuple->t_data->t_info_mask = infomask;
  new_tuple->t_data->t_natts = number_of_attributes;
  new_tuple->t_self = tuple->t_self;
  new_tuple->t_table_oid = tuple->t_table_oid;

  return new_tuple;
}

void heap_free_tuple(HeapTuple tuple) {
  if (tuple->t_data != NULL) {
    if (tuple->t_data_mcxt != NULL &&
        (char*)(tuple->t_data) != ((char*)tuple + HEAP_TUPLE_SIZE)) {
      pfree(tuple->t_data);
    }
  }

  pfree(tuple);
}

HeapTuple heap_add_header(uint32 natts, int struct_len, char* structure) {
  HeapTuple tuple;
  HeapTupleHeader td;  // tuple data
  unsigned long len;
  int hoff;

  ASSERT_ARG(natts > 0);

  len = offsetof(HeapTupleHeaderData, t_bits);

  hoff = len = MAXALIGN(len);  // Be conservative
  len += struct_len;
  tuple = (HeapTuple)palloc(HEAP_TUPLE_SIZE + len);
  tuple->t_data_mcxt = CurrentMemoryContext;
  td = tuple->t_data = (HeapTupleHeader)((char*)tuple + HEAP_TUPLE_SIZE);

  tuple->t_len = len;
  ITEM_POINTER_SET_INVALID(&(tuple->t_self));
  tuple->t_table_oid = INVALID_OID;

  MemSet((char*)td, 0, len);

  td->t_hoff = hoff;
  td->t_natts = natts;
  td->t_info_mask = 0;
  td->t_info_mask |= HEAP_XMAX_INVALID;

  if (struct_len > 0) {
    memcpy((char*)td + hoff, structure, (size_t)struct_len);
  }

  return tuple;
}