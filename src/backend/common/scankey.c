// -------------------------------------------------------------------------
//
// scan.c
//  scan direction and key code
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/access/common/scankey.c,v 1.17 2000/01/26 05:55:53 momjian Exp $
//
// -------------------------------------------------------------------------

#include "rdbms/access/skey.h"

#define SCAN_KEY_ENTRY_IS_LEGAL(entry) (assert(POINTER_IS_VALID(entry)), ATTRIBUTE_NUMBER_IS_VALID((entry)->sk_attno))

void scan_key_entry_set_illegal(ScanKey entry) {
  assert(POINTER_IS_VALID(entry));

  entry->sk_flags = 0;
  entry->sk_attno = INVALID_ATTR_NUMBER;
  entry->sk_procedure = 0;  // Should be InvalidRegProcedure.
}

// Initializes an scan key entry.
//
// Note:
//  Assumes the scan key entry is valid.
//  Assumes the intialized scan key entry will be legal.
// TODO(gc): 各参数的含义
void scan_key_entry_initialize(ScanKey entry, bits16 flags, AttrNumber attr_num, RegProcedure procedure,
                               Datum argument) {
  assert(POINTER_IS_VALID(entry));

  entry->sk_flags = flags;
  entry->sk_attno = attr_num;
  entry->sk_procedure = procedure;
  entry->sk_argument = argument;
  fmgr_info(procedure, &entry->sk_func);
  entry->sk_nargs = entry->sk_func.fn_nargs;

  assert(SCAN_KEY_ENTRY_IS_LEGAL(entry));
}