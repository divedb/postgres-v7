//===----------------------------------------------------------------------===//
// postgres_ext.h
//
//  This file contains declarations of things that are visible
//  external to Postgres.  For example, the Oid type is part of a
//  structure that is passed to the front end via libpq, and is
//  accordingly referenced in libpq-fe.h.
//
//  Declarations which are specific to a particular interface should
//  go in the header file for that interface (such as libpq-fe.h).  This
//  file is only for fundamental Postgres declarations.
//
//  User-written C functions don't count as "external to Postgres."
//  Those function much as local modifications to the backend itself, and
//  use header files that are otherwise internal to Postgres to interface
//  with the backend.
//
// $Id: postgres_ext.h,v 1.4 1999/06/04 21:12:07 tgl Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_POSTGRES_EXT_H_
#define RDBMS_POSTGRES_EXT_H_

typedef unsigned int Oid;

#define INVALID_OID ((Oid)0)

#define OID_MAX UINT_MAX

// NAMEDATALEN is the max length for system identifiers (e.g. table names,
// attribute names, function names, etc.)
//
// NOTE that databases with different NAMEDATALEN's cannot interoperate!
#define NAME_DATA_LEN 32

#endif  // RDBMS_POSTGRES_EXT_H_
