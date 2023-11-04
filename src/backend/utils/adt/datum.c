// =========================================================================
//
// datum.c
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/adt/datum.c,v 1.17 2000/01/26 05:57:13 momjian Exp $
//
// =========================================================================

// In the implementation of the next routines we assume the following:
//
// A) if a type is "byVal" then all the information is stored in the
// Datum itself (i.e. no pointers involved!). In this case the
// length of the type is always greater than zero and less than
// "sizeof(Datum)"
//
// B) if a type is not "byVal" and it has a fixed length, then
// the "Datum" always contain a pointer to a stream of bytes.
// The number of significant bytes are always equal to the length of the
// type.
//
// C) if a type is not "byVal" and is of variable length (i.e. it has
// length == =1) then "Datum" always points to a "struct varlena".
// This varlena structure has information about the actual length of this
// particular instance of the type and about its value.

#include "rdbms/utils/datum.h"

#include "rdbms/postgres.h"
#include "rdbms/utils/elog.h"

// Find the "real" size of a datum, given the datum value,
// its type, whether it is a "by value", and its length.
//
// To cut a long story short, usually the real size is equal to the
// type length, with the exception of variable length types which have
// a length equal to -1. In this case, we have to look at the value of
// the datum itself (which is a pointer to a 'varlena' struct) to find
// its size.
Size datum_get_size(Datum value, Oid type, bool by_val, Size len) {
  struct varlena* s;
  Size size = 0;

  if (by_val) {
    if (len <= sizeof(Datum)) {
      size = len;
    } else {
      elog(ERROR, "%s: error type = %ld, by val with len = %d.", (long)type, len);
    }
  } else {
    if (len == -1) {
      // Variable length type Look at the varlena struct for its
      // real length...
      s = (struct varlena*)DATUM_GET_POINTER(value);
      if (!POINTER_IS_VALID(s)) {
        elog(ERROR, "%s: invalid datum pointer.", __func__);
      }
      size = (Size)VARSIZE(s);
    } else {
      // Fixed length type.
      size = len;
    }
  }

  return size;
}

// Make a copy of a datum
//
// If the type of the datum is not passed by value (i.e. "byVal=false")
// then we assume that the datum contains a pointer and we copy all the
// bytes pointed by this pointer
Datum datum_copy(Datum value, Oid type, bool by_val, Size len) {
  Size real_size;
  Datum res;
  char* s;

  if (by_val) {
    res = value;
  } else {
    if (value == 0) {
      return NULL;
    }

    real_size = DATUM_GET_SIZE(value, type, by_val, len);

    // The value is a pointer. Allocate enough space and copy the
    // pointed data.
    s = (char*)palloc(real_size);

    if (s == NULL) {
      elog(ERROR, "%s: out of memory.", __func__);
    }

    memmove(s, DATUM_GET_POINTER(value), real_size);
    res = (Datum)s;
  }

  return res;
}

// Free the space occupied by a datum CREATED BY "datumCopy"
//
// NOTE: DO NOT USE THIS ROUTINE with datums returned by amgetattr() etc.
// ONLY datums created by "datumCopy" can be freed!
void datum_free(Datum value, Oid type, bool by_val, Size len) {
  Size real_size;
  Pointer s;

  real_size = DATUM_GET_SIZE(value, type, by_val, len);

  if (!by_val) {
    // Free the space palloced by "datumCopy()".
    s = DATUM_GET_POINTER(value);
    pfree(s);
  }
}

// Return true if two datums are equal, false otherwise
//
// NOTE: XXX!
// We just compare the bytes of the two values, one by one.
// This routine will return false if there are 2 different
// representations of the same value (something along the lines
// of say the representation of zero in one's complement arithmetic).
bool datum_is_equal(Datum value1, Datum value2, Oid type, bool by_val, Size len) {
  Size size1;
  Size size2;
  char* s1;
  char* s2;

  if (by_val) {
    // Just compare the two datums. NOTE: just comparing "len" bytes
    // will not do the work, because we do not know how these bytes
    // are aligned inside the "Datum".
    if (value1 == value2) {
      return true;
    } else {
      return false;
    }
  } else {
    // byVal = false Compare the bytes pointed by the pointers stored
    // in the datums.
    size1 = DATUM_GET_SIZE(value1, type, by_val, len);
    size2 = DATUM_GET_SIZE(value2, type, by_val, len);

    if (size1 != size2) {
      return false;
    }

    s1 = (char*)DATUM_GET_POINTER(value1);
    s2 = (char*)DATUM_GET_POINTER(value2);

    if (!memcmp(s1, s2, size1)) {
      return true;
    } else {
      return false;
    }
  }
}