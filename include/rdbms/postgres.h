//===----------------------------------------------------------------------===//
// postgres.h
//  definition of (and support for) postgres system types.
//  this file is included by almost every .c in the system
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1995, Regents of the University of California
//
// $Id: postgres.h,v 1.38 2000/04/12 17:16:24 momjian Exp $
//===----------------------------------------------------------------------===//

// This file will eventually contain the definitions for the
// following (and perhaps other) system types:
//
//  int2    int4    float4    float8
//  Oid     regproc RegProcedure
//  aclitem
//  struct VarLena
//  int2vector    oidvector
//  bytea    text
//  NameData   Name
//
//  TABLE OF CONTENTS
//    1) simple type definitions
//    2) VarLena and array types
//    3) TransactionId and CommandId
//    4) genbki macros used by catalog/pg_xxx.h files
//    5) random stuff

#ifndef RDBMS_POSTGRES_H_
#define RDBMS_POSTGRES_H_

#include "rdbms/c.h"
#include "rdbms/config.h"
#include "rdbms/postgres_ext.h"

// ================================================
// Section 1: simple type definitions
// ================================================

typedef int16 int2;
typedef int32 int4;
typedef float float4;
typedef double float8;

typedef int4 aclitem;

#define INVALID_OID             0
#define OID_IS_VALID(object_id) ((bool)((object_id) != INVALID_OID))

// Unfortunately, both regproc and RegProcedure are used
typedef Oid regproc;
typedef Oid RegProcedure;

// Pointer to func returning (char *)
typedef char*((*func_ptr)());

#define REG_PROCEDURE_IS_VALID(p) OID_IS_VALID(p)

// ================================================
// Section 2: variable length and array types
// ================================================

struct VarLena {
  int32 vl_len;
  char vl_dat[1];
};

#define VARSIZE(ptr) (((struct VarLena*)(ptr))->vl_len)
#define VARDATA(ptr) (((struct VarLena*)(ptr))->vl_dat)
#define VARHDRSZ     ((int32)sizeof(int32))

typedef struct VarLena Bytea;
typedef struct VarLena Text;

typedef int2 Int2Vector[INDEX_MAX_KEYS];
typedef Oid OidVector[INDEX_MAX_KEYS];

// We want NameData to have length NAMEDATALEN and int alignment,
// because that's how the data type 'name' is defined in pg_type.
// Use a union to make sure the compiler agrees.
// TODO(gc): does alignment_dummy makes sense here?
typedef union NameData {
  char data[NAME_DATA_LEN];
  int alignment_dummy;
} NameData;

typedef NameData* Name;

#define NAME_STR(name) ((name).data)

// ================================================
// Section 3: TransactionId and CommandId
// ================================================

typedef uint32 TransactionId;
#define INVALID_TRANSACTION_ID 0

typedef uint32 CommandId;
#define FIRST_COMMAND_ID 0

// ================================================
// Section 4: genbki macros used by the
//            catalog/pg_xxx.h files
// ================================================

#define CATALOG(x) typedef struct CPP_CONCAT(FormData_, x)

#define DATA(x)                 extern int errno
#define DESCR(x)                extern int errno
#define DECLARE_INDEX(x)        extern int errno
#define DECLARE_UNIQUE_INDEX(x) extern int errno

#define BUILD_INDICES
#define BOOTSTRAP

#define BKI_BEGIN
#define BKI_END

// ================================================
// Section 5: random stuff
//            CSIGNBIT, STATUS...
// ================================================

// msb for int/unsigned
#define ISIGNBIT (0x80000000)
#define WSIGNBIT (0x8000)
#define CSIGNBIT (0x80)

#define STATUS_OK           (0)
#define STATUS_ERROR        (-1)
#define STATUS_NOT_FOUND    (-2)
#define STATUS_INVALID      (-3)
#define STATUS_UNCATALOGUED (-4)
#define STATUS_REPLACED     (-5)
#define STATUS_NOT_DONE     (-6)
#define STATUS_BAD_PACKET   (-7)
#define STATUS_FOUND        (1)

#endif  // RDBMS_POSTGRES_H_
