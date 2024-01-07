//===----------------------------------------------------------------------===//
// postgres.h
//  Primary include file for PostgreSQL server .c files
//
// This should be the first file included by PostgreSQL backend modules.
// Client-side code should include postgres_fe.h instead.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1995, Regents of the University of California
//
// $Id: postgres.h,v 1.48 2001/03/23 18:26:01 tgl Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_POSTGRES_H_
#define RDBMS_POSTGRES_H_

#include "rdbms/c.h"

//===----------------------------------------------------------------------===//
// Section 1: variable-length datatypes (TOAST support)
//===----------------------------------------------------------------------===//
// Struct varattrib is the header of a varlena object that may have been
// TOASTed.
#define TUPLE_TOASTER_ACTIVE

typedef struct VarAttrib {
  int32 va_header;  // External/compressed storage
  // Flags and item size
  union {
    struct {
      int32 va_rawsize;  // Plain data size.
      char va_data[1];   // Compressed data.
    } va_compressed;     // Compressed stored attribute.

    struct {
      int32 va_rawsize;   // Plain data size.
      int32 va_extsize;   // External saved size.
      Oid va_valueid;     // Unique identifier of value.
      Oid va_toastrelid;  // RelID where to find chunks.
      Oid va_toastidxid;  // Main tables row Oid.
      Oid va_rowid;       // Referencing row Oid.
      int16 va_attno;     // Main tables attno.
    } va_external;        // External stored attribute.

    char va_data[1];  // Plain stored attribute.
  } va_content;
} VarAttrib;

#define VARATT_FLAG_EXTERNAL   0x80000000
#define VARATT_FLAG_COMPRESSED 0x40000000
#define VARATT_MASK_FLAGS      0xc0000000
#define VARATT_MASK_SIZE       0x3fffffff

#define VARATT_SIZEP(ptr) (((VarAttrib*)(ptr))->va_header)
#define VARATT_SIZE(ptr)  (VARATT_SIZEP(ptr) & VARATT_MASK_SIZE)
#define VARATT_DATA(ptr)  (((VarAttrib*)(ptr))->va_content.va_data)
#define VARATT_CDATA(ptr) \
  (((VarAttrib*)(ptr))->va_content.va_compressed.va_data)

#define VARSIZE(ptr) VARATT_SIZE(ptr)
#define VARDATA(ptr) VARATT_DATA(ptr)

#define VARATT_IS_EXTENDED(ptr) ((VARATT_SIZEP(ptr) & VARATT_MASK_FLAGS) != 0)
#define VARATT_IS_EXTERNAL(ptr) \
  ((VARATT_SIZEP(ptr) & VARATT_FLAG_EXTERNAL) != 0)
#define VARATT_IS_COMPRESSED(ptr) \
  ((VARATT_SIZEP(ptr) & VARATT_FLAG_COMPRESSED) != 0)

//===----------------------------------------------------------------------===//
// Section 2: datum type + support macros
//===----------------------------------------------------------------------===//
// Port Notes:
//  Postgres makes the following assumption about machines:
//
//  sizeof(Datum) == sizeof(long) >= sizeof(void *) >= 4
//
//  Postgres also assumes that
//
//  sizeof(char) == 1
//
//  and that
//
//  sizeof(short) == 2
//
//  If your machine meets these requirements, Datums should also be checked
//  to see if the positioning is correct.
typedef unsigned long Datum;  // XXX sizeof(long) >= sizeof(void *)
typedef Datum* DatumPtr;

#define GET_1_BYTE(datum)  (((Datum)(datum)) & 0x000000ff)
#define GET_2_BYTES(datum) (((Datum)(datum)) & 0x0000ffff)
#define GET_4_BYTES(datum) (((Datum)(datum)) & 0xffffffff)
#define SET_1_BYTE(value)  (((Datum)(value)) & 0x000000ff)
#define SET_2_BYTES(value) (((Datum)(value)) & 0x0000ffff)
#define SET_4_BYTES(value) (((Datum)(value)) & 0xffffffff)

#define DATUM_GET_BOOL(x)        ((bool)(((Datum)(x)) != 0))
#define BOOL_GET_DATUM(x)        ((Datum)((x) ? 1 : 0))
#define DATUM_GET_CHAR(x)        ((char)GET_1_BYTE(x))
#define CHAR_GET_DATUM(x)        ((Datum)SET_1_BYTE(x))
#define INT8_GET_DATUM(x)        ((Datum)SET_1_BYTE(x))
#define DATUM_GET_UINT8(x)       ((uint8)GET_1_BYTE(x))
#define UINT8_GET_DATUM(x)       ((Datum)SET_1_BYTE(x))
#define DATUM_GET_INT16(x)       ((int16)GET_2_BYTES(x))
#define INT16_GET_DATUM(x)       ((Datum)SET_2_BYTES(x))
#define DATUM_GET_UINT16(x)      ((uint16)GET_2_BYTES(x))
#define UINT16_GET_DATUM(x)      ((Datum)SET_2_BYTES(x))
#define DATUM_GET_INT32(x)       ((int32)GET_4_BYTES(x))
#define INT32_GET_DATUM(x)       ((Datum)SET_4_BYTES(x))
#define DATUM_GET_UINT32(x)      ((uint32)GET_4_BYTES(x))
#define UINT32_GET_DATUM(x)      ((Datum)SET_4_BYTES(x))
#define DATUM_GET_OBJECT_ID(x)   ((Oid)GET_4_BYTES(x))
#define OBJECT_ID_GET_DATUM(x)   ((Datum)SET_4_BYTES(x))
#define DATUM_GET_POINTER(x)     ((Pointer)(x))
#define POINTER_GET_DATUM(x)     ((Datum)(x))
#define DATUM_GET_CSTRING(x)     ((char*)DATUM_GET_POINTER(x))
#define CSTRING_GET_DATUM(x)     POINTER_GET_DATUM(x)
#define DATUM_GET_NAME(x)        ((Name)DATUM_GET_POINTER((Datum)(x)))
#define NAME_GET_DATUM(x)        POINTER_GET_DATUM((Pointer)(x))
#define DATUM_GET_INT64(x)       (*((int64*)DATUM_GET_POINTER(x)))
#define DATUM_GET_FLOAT4(x)      (*(float4*)DATUM_GET_POINTER(x))
#define DATUM_GET_FLOAT8(x)      (*(float8*)DATUM_GET_POINTER(x))
#define DATUM_GET_FLOAT32(x)     ((float32)DATUM_GET_POINTER(x))
#define FLOAT32_GET_DATUM(x)     POINTER_GET_DATUM((Pointer)(x))
#define DATUM_GET_FLOAT64(x)     ((float64)DATUM_GET_POINTER(x))
#define FLOAT64_GET_DATUM(x)     POINTER_GET_DATUM((Pointer)(x))
#define INT64_GET_DATUM_FAST(x)  POINTER_GET_DATUM(&(x))
#define FLOAT4_GET_DATUM_FAST(x) POINTER_GET_DATUM(&(x))
#define FLOAT8_GET_DATUM_FAST(x) POINTER_GET_DATUM(&(x))

//===----------------------------------------------------------------------===//
// Section 3: exception handling definitions
//            Assert, Trap, etc macros
//===----------------------------------------------------------------------===//
typedef char* ExcMessage;
typedef struct Exception {
  ExcMessage message;
} Exception;

extern Exception FailedAssertion;
extern Exception BadArg;
extern Exception BadState;

extern bool AssertEnabled;

// Generates an exception if the given condition is true.
#define TRAP(condition, exception)                                  \
  do {                                                              \
    if ((AssertEnabled) && (condition))                             \
      exceptional_condition(CPP_AS_STRING(condition), &(exception), \
                            (char*)NULL, __FILE__, __LINE__);       \
  } while (0)

// TrapMacro is the same as Trap but it's intended for use in macros:
//
//  #define foo(x) (AssertM(x != 0) && bar(x))
//
// Isn't CPP fun?
#define TRAP_MACRO(condition, exception)                                 \
  ((bool)((!AssertEnabled) || !(condition) ||                            \
          (exceptional_condition(CPP_AS_STRING(condition), &(exception), \
                                 (char*)NULL, __FILE__, __LINE__))))

// TODO(gc): fix assert
#ifndef USE_ASSERT_CHECKING
#define ASSERT(condition)       assert(condition)
#define ASSERT_MACRO(condition) ((void)true)
#define ASSERT_ARG(condition)
#define ASSERT_STATE(condition)
#define AssertEnabled 0
#else
#define ASSERT(condition) TRAP(!(condition), FailedAssertion)
#define ASSERT_MACRO(condition) \
  ((void)TRAP_MACRO(!(condition), FailedAssertion))
#define ASSERT_ARG(condition)   TRAP(!(condition), BadArg)
#define ASSERT_STATE(condition) TRAP(!(condition), BadState)
#endif

// Generates an exception with a message if the given condition is true.
#define LOG_TRAP(condition, exception, print_args)                         \
  do {                                                                     \
    if ((AssertEnabled) && (condition))                                    \
      exceptional_condition(CPP_AS_STRING(condition), &(exception),        \
                            vararg_format print_args, __FILE__, __LINE__); \
  } while (0)

// This is the same as LogTrap but it's intended for use in macros:
//
//  #define foo(x) (LogAssertMacro(x != 0, "yow!") && bar(x))
#define LOG_TRAP_MACRO(condition, exception, print_args)                 \
  ((bool)((!AssertEnabled) || !(condition) ||                            \
          (exceptional_condition(CPP_AS_STRING(condition), &(exception), \
                                 vararg_format print_args, __FILE__,     \
                                 __LINE__))))

extern int exceptional_condition(char* condition_name, Exception* exceptionp,
                                 char* details, char* filename,
                                 int line_number);
extern char* vararg_format(const char* fmt, ...);

#ifndef USE_ASSERT_CHECKING

#define LOG_ASSERT(condition, print_args)
#define LOG_ASSERT_MACRO(condition, print_args) true
#define LOG_ASSERT_ARG(condition, print_args)
#define LOG_ASSERT_STATE(condition, print_args)

#else

#define LOG_ASSERT(condition, print_args) \
  LOG_TRAP_(!(condition), FailedAssertion, print_args)
#define LOG_ASSERT_MACRO(condition, print_args) \
  LOG_TRAP_MACRO(!(condition), FailedAssertion, print_args)
#define LOG_ASSERT_ARG(condition, print_args) \
  LOG_TRAP_(!(condition), BadArg, print_args)
#define LOG_ASSERT_STATE(condition, print_args) \
  LOG_TRAP_(!(condition), BadState, print_args)

#ifdef ASSERT_CHECKING_TEST
extern int assert_test(int val);
#endif

#endif

//===----------------------------------------------------------------------===//
// Section 4: genbki macros used by catalog/pg_xxx.h files
//===----------------------------------------------------------------------===//
#define CATALOG(x) typedef struct CPP_CONCAT(FormData_, x)

#define DATA(x)                 extern int errno
#define DESCR(x)                extern int errno
#define DECLARE_INDEX(x)        extern int errno
#define DECLARE_UNIQUE_INDEX(x) extern int errno

#define BUILD_INDICES
#define BOOTSTRAP

#define BKI_BEGIN
#define BKI_END

typedef int4 aclitem;  // PHONY definition for catalog use only.

#endif  // RDBMS_POSTGRES_H_
