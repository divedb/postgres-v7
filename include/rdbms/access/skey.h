// =========================================================================
//
// skey.h
//  POSTGRES scan key definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: skey.h,v 1.13 2000/01/26 05:57:51 momjian Exp $
//
//
// Note:
//  Needs more accessor/assignment routines.
// =========================================================================

#ifndef RDBMS_ACCESS_SKEY_H_
#define RDBMS_ACCESS_SKEY_H_

#include "rdbms/access/attnum.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/fmgr.h"

typedef struct ScanKeyData {
  bits16 sk_flags;            // Flags.
  AttrNumber sk_attno;        // Domain number.
  RegProcedure sk_procedure;  // Procedure OID.
  FmgrInfo sk_func;
  int32 sk_nargs;
  Datum sk_argument;  // Data to compare.
} ScanKeyData;

typedef ScanKeyData* ScanKey;

#define SK_ISNULL  0x1
#define SK_UNARY   0x2
#define SK_NEGATE  0x4
#define SK_COMMUTE 0x8

#define SCAN_UNMARKED           0x01
#define SCAN_UNCHECKED_PREVIOUS 0x02
#define SCAN_UNCHECKED_NEXT     0x04

// Prototypes for functions in access/common/scankey.c.
void scan_key_entry_set_illegal(ScanKey entry);
void scan_key_entry_initialize(ScanKey entry, bits16 flags, AttrNumber attr_num, RegProcedure procedure,
                               Datum argument);

#endif  // RDBMS_ACCESS_SKEY_H_
