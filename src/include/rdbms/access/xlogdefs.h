//===----------------------------------------------------------------------===//
//
// xlogdefs.h
//  Postgres transaction log manager record pointer and system startup number
//  definitions
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: bufmgr.h,v 1.37 2000/04/12 17:16:51 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_ACCESS_XLOG_DEFS_H_
#define RDBMS_ACCESS_XLOG_DEFS_H_

// Pointer to a location in the XLOG. These pointers are 64 bits wide,
// because we don't want them ever to overflow.
//
// NOTE: xrecoff == 0 is used to indicate an invalid pointer. This is OK
// because we use page headers in the XLOG, so no XLOG record can start
// right at the beginning of a file.
//
// NOTE: the "log file number" is somewhat misnamed, since the actual files
// making up the XLOG are much smaller than 4Gb. Each actual file is an
// XLogSegSize-byte "segment" of a logical log file having the indicated
// xlogid. The log file number and segment number together identify a
// physical XLOG file. Segment number and offset within the physical file
// are computed from xrecoff div and mod XLogSegSize.
typedef struct XLogRecPtr {
  uint32 xlog_id;   // Log file #, 0 based
  uint32 xrec_off;  // Byte offset of location in log file
} XLogRecPtr;

// Macros for comparing XLogRecPtrs
//
// Beware of passing expressions with side-effects to these macros,
// since the arguments may be evaluated multiple times.
#define XL_BYTE_LT(a, b) ((a).xlog_id < (b).xlog_id || ((a).xlog_id == (b).xlog_id && (a).xrec_off < (b).xrec_off))
#define XL_BYTE_LE(a, b) ((a).xlog_id < (b).xlog_id || ((a).xlog_id == (b).xlog_id && (a).xrec_off <= (b).xrec_off))
#define XL_BYTE_EQ(a, b) ((a).xlog_id == (b).xlog_id && (a).xrec_off == (b).xrec_off)

// StartUpID (SUI) - system startups counter. It's to allow removing
// pg_log after shutdown, in future.
typedef uint32 StartUpID;

#endif  // RDBMS_ACCESS_XLOG_DEFS_H_
