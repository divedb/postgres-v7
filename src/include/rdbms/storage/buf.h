//===----------------------------------------------------------------------===//
//
// buf.h
//  Basic buffer manager data types.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: buf.h,v 1.7 2000/01/26 05:58:32 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_STORAGE_BUF_H_
#define RDBMS_STORAGE_BUF_H_

#define INVALID_BUFFER (0)
#define UNKOWN_BUFFER  (-99999)

typedef long Buffer;

#define BUFFER_IS_INVALID(buffer) ((buffer) == INVALID_BUFFER)
#define BUFFER_IS_UNKOWN(buffer)  ((buffer) == UNKOWN_BUFFER)
#define BUFFER_IS_LOCAL(buffer)   ((buffer) < 0)

// If NO_BUFFERISVALID is defined, all error checking using BufferIsValid()
// This should be set only if one is sure there will be no errors.
//  - plai 9/10/90
#undef NO_BUFFER_IS_VALID

#endif  // RDBMS_STORAGE_BUF_H_
