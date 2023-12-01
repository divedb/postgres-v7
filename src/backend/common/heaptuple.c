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
//  $Header: /home/projects/pgsql/cvsroot/pgsql/src/backend/access/common/heaptuple.c,v 1.71 2001/03/22 06:16:06
//  momjian Exp $
//
// NOTES
//  The old interface functions have been converted to macros
//  and moved to heapam.h
//
//===----------------------------------------------------------------------===//

#include "rdbms/access/heapam.h"

//		constructs a tuple from the given *value and *null arrays
//
// old comments
//		Handles alignment by aligning 2 byte attributes on short boundries
//		and 3 or 4 byte attributes on long word boundries on a vax; and
//		aligning non-byte attributes on short boundries on a sun.  Does
//		not properly align fixed length arrays of 1 or 2 byte types (yet).
//
//		Null attributes are indicated by a 'n' in the appropriate byte
//		of the *null.	Non-null attributes are indicated by a ' ' (space).
//
//		Fix me.  (Figure that must keep context if debug--allow give oid.)
//		Assumes in order.
HeapTuple heap_form_tuple(TupleDesc tup_desc, Datum* value, char* nulls);