// =========================================================================
//
// datum.h
//  POSTGRES abstract data type datum representation definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: datum.h,v 1.10 2000/01/26 05:58:37 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_DATUM_H_
#define RDBMS_UTILS_DATUM_H_

#include "rdbms/postgres.h"

// SOME NOT VERY PORTABLE ROUTINES ???
//
// In the implementation of the next routines we assume the following:
//
// A) if a type is "byVal" then all the information is stored in the
// Datum itself (i.e. no pointers involved!). In this case the
// length of the type is always greater than zero and less than
// "sizeof(Datum)"
//
// B) if a type is not "byVal" and it has a fixed length, then
// the "Datum" always contain a pointer to a stream of bytes.
// The number of significant bytes are always equal to the length of thr
// type.
//
// C) if a type is not "byVal" and is of variable length (i.e. it has
// length == -1) then "Datum" always points to a "struct varlena".
// This varlena structure has information about the actual length of this
// particular instance of the type and about its value.

// Dind the "real" length of a datum.
Size datum_get_size(Datum value, Oid type, bool by_val, Size len);

// Make a copy of a datum.
Datum datum_copy(Datum value, Oid type, bool by_val, Size len);

// Free space that *might* have been palloced by "datumCopy".
void datum_free(Datum value, Oid type, bool by_val, Size len);

// Return true if thwo datums are equal, false otherwise.
// XXX : See comments in the code for restrictions!
bool datum_is_equal(Datum value1, Datum value2, Oid type, bool by_val, Size len);

#endif  // RDBMS_UTILS_DATUM_H_
