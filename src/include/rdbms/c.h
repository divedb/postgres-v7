//===----------------------------------------------------------------------===//
//
// c.h
//  Fundamental C definitions.  This is included by every .c file in
//  PostgreSQL (via either postgres.h or postgres_fe.h, as appropriate).
//
//  Note that the definitions here are not intended to be exposed to clients of
//  the frontend interface libraries --- so we don't worry much about polluting
//  the namespace with lots of stuff...
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: c.h,v 1.92 2001/03/22 04:00:24 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_C_H_
#define RDBMS_C_H_

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rdbms/config.h"
#include "rdbms/postgres_ext.h"

//===----------------------------------------------------------------------===//
// Section 1: hacks to cope with non-ANSI C compilers
//
// Type prefixes (const, signed, volatile, inline) are now handled in config.h.
//===----------------------------------------------------------------------===//
#define CPP_AS_STRING(identifier) #identifier
#define CPP_CONCAT(x, y)          x##y
#define DUMMY_RET                 void

//===----------------------------------------------------------------------===//
// Section 2: bool, true, false, TRUE, FALSE, NULL
//===----------------------------------------------------------------------===//
typedef bool* BoolPtr;

//===----------------------------------------------------------------------===//
// Section 3: standard system types
//===----------------------------------------------------------------------===//
typedef char* Pointer;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint8 bool8;
typedef uint16 bool16;
typedef uint32 bool32;
typedef uint8 bits8;
typedef uint16 bits16;
typedef uint32 bits32;
typedef uint8 word8;
typedef uint16 word16;
typedef uint32 word32;
typedef float float32data;
typedef double float64data;
typedef float* float32;
typedef double* float64;
typedef long int int64;
typedef unsigned long int uint64;

typedef size_t Size;
typedef unsigned int Index;
typedef signed int Offset;

typedef int16 int2;
typedef int32 int4;
typedef float float4;
typedef double float8;

// Unfortunately, both regproc and RegProcedure are used.
typedef Oid regproc;
typedef Oid RegProcedure;

typedef uint32 TransactionId;

#define INVALID_TRANSACTION_ID 0

typedef uint32 CommandId;

#define FIRST_COMMAND_ID 0

#define MAX_DIM 6

typedef struct {
  int indx[MAX_DIM];
} IntArray;

struct VarLena {
  int32 vl_len;
  char vl_dat[1];
};

#define VAR_HDR_SZ ((int32)sizeof(int32))

typedef struct VarLena Bytea;
typedef struct VarLena text;
typedef struct VarLena BpChar;  // Blank-padded char, ie SQL char(n)
typedef struct VarLena VarChar;

typedef int2 Int2Vector[INDEX_MAX_KEYS];
typedef Oid OidVector[INDEX_MAX_KEYS];

// We want NameData to have length NAMEDATALEN and int alignment,
// because that's how the data type 'name' is defined in pg_type.
// Use a union to make sure the compiler agrees.
typedef union NameData {
  char data[NAME_DATA_LEN];
  int alignment_dummy;
} NameData;

typedef NameData* Name;

#define NAME_STR(name) ((name).data)

//===----------------------------------------------------------------------===//
// Section 4: IsValid macros for system types
//===----------------------------------------------------------------------===//
#define BOOL_IS_VALID(boolean)    ((boolean) == false || (boolean) == true)
#define POINTER_IS_VALID(pointer) ((void*)(pointer) != NULL)
#define POINTER_IS_ALIGNED(pointer, type) \
  (((long)(pointer) % (sizeof(type))) == 0)
#define OID_IS_VALID(oid)         ((oid) != INVALID_OID)
#define REG_PROCEDURE_IS_VALID(p) OID_IS_VALID(p)

//===----------------------------------------------------------------------===//
// Section 5: offsetof, lengthof, endof, alignment
//===----------------------------------------------------------------------===//
#define OFFSET_OF(type, field) ((long)&((type*)0)->field)
#define LENGTH_OF(array)       (sizeof(array) / sizeof((array)[0]))
#define END_OF(array)          (&array[LENGTH_OF(array)])

// Alignment macros: align a length or address appropriately for a given type.
//
// There used to be some incredibly crufty platform-dependent hackery here,
// but now we rely on the configure script to get the info for us. Much nicer.
//
// NOTE: TYPEALIGN will not work if ALIGNVAL is not a power of 2.
// That case seems extremely unlikely to occur in practice, however.
#define TYPE_ALIGN(alignment, size) \
  (((long)(size) + (alignment - 1)) & ~(alignment - 1))

#define SHORT_ALIGN(size)  TYPE_ALIGN(_Alignof(short), size)
#define INT_ALIGN(size)    TYPE_ALIGN(_Alignof(int), size)
#define LONG_ALIGN(size)   TYPE_ALIGN(_Alignof(long), size)
#define DOUBLE_ALIGN(size) TYPE_ALIGN(_Alignof(double), size)
#define MAX_ALIGN(size)    TYPE_ALIGN(_Alignof(max_align_t), size)

//===----------------------------------------------------------------------===//
// Section 6: widely useful macros
//===----------------------------------------------------------------------===//
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ABS(x)    ((x) >= 0 ? (x) : -(x))

// Like standard library function strncpy(), except that result string
// is guaranteed to be null-terminated --- that is, at most N-1 bytes
// of the source string will be kept.
// Also, the macro returns no result (too hard to do that without
// evaluating the arguments multiple times, which seems worse).
//
// BTW: when you need to copy a non-null-terminated string (like a text
// datum) and add a null, do not do it with StrNCpy(..., len+1).  That
// might seem to work, but it fetches one byte more than there is in the
// text object.  One fine day you'll have a SIGSEGV because there isn't
// another byte before the end of memory. Don't laugh, we've had real
// live bug reports from real live users over exactly this mistake.
// Do it honestly with "memcpy(dst,src,len); dst[len] = '\0';", instead.
#define STR_N_CPY(dst, src, len)  \
  do {                            \
    char* _dst = (dst);           \
    Size _len = (len);            \
                                  \
    if (_len > 0) {               \
      strncpy(_dst, (src), _len); \
      _dst[_len - 1] = '\0';      \
    }                             \
  } while (0)

// Get a bit mask of the bits set in non-int32 aligned addresses.
#define INT_ALIGN_MASK (sizeof(int32) - 1)

// Exactly the same as standard library function memset(), but considerably
// faster for zeroing small word-aligned structures (such as parsetree nodes).
// This has to be a macro because the main point is to avoid function-call
// overhead.
//
// We got the 64 number by testing this against the stock memset() on
// BSD/OS 3.0. Larger values were slower. bjm 1997/09/11
//
// I think the crossover point could be a good deal higher for
// most platforms, actually.  tgl 2000-03-19
#define MEMSET(start, val, len)                      \
  do {                                               \
    int32* _start = (int32*)(start);                 \
    int _val = (val);                                \
    Size _len = (len);                               \
                                                     \
    if ((((long)_start) & INT_ALIGN_MASK) == 0 &&    \
        (_len & INT_ALIGN_MASK) == 0 && _val == 0 && \
        _len <= MEMSET_LOOP_LIMIT) {                 \
      int32* _stop = (int32*)((char*)_start + _len); \
      while (_start < _stop) *_start++ = 0;          \
    } else                                           \
      memset((char*)_start, _val, _len);             \
  } while (0)

#define MEMSET_LOOP_LIMIT 64

//===----------------------------------------------------------------------===//
// Section 7: random stuff
//===----------------------------------------------------------------------===//
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

//===----------------------------------------------------------------------===//
// Section 8: system-specific hacks
//
// This should be limited to things that absolutely have to be
// included in every source file. The port-specific header file
// is usually a better place for this sort of thing.
//===----------------------------------------------------------------------===//
#define PG_BINARY   0
#define PG_BINARY_R "r"
#define PG_BINARY_W "w"

// These are for things that are one way on Unix and another on NT.
#define NULL_DEV "/dev/null"
#define SEP_CHAR '/'

#endif  // RDBMS_C_H_
