#ifndef RDBMS_C_H_
#define RDBMS_C_H_

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================================================
// Section 1: bool, true, false, TRUE, FALSE, NULL
// ================================================

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef bool* BoolPtr;

// ================================================
// Section 2: non-ansi C definitions
// ================================================

#define CPP_AS_STRING(identifier) #identifier
#define CPP_CONCAT(x, y)          x##y

// ================================================
// Section 3: standard system types
// ================================================

typedef char* Pointer;

// intN
//  Signed integer, EXACTLY N BITS IN SIZE,
//  used for numerical computations and the
//  frontend/backend protocol.
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

// uintN
//  Unsigned integer, EXACTLY N BITS IN SIZE,
//  used for numerical computations and the
//  frontend/backend protocol.
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

// floatN
//  Floating point number, AT LEAST N BITS IN SIZE,
//  used for numerical computations.
//
//  Since sizeof(floatN) may be > sizeof(char *), always pass
//  floatN by reference.
typedef float float32data;
typedef double float64data;
typedef float* float32;
typedef double* float64;

// boolN
//  Boolean value, AT LEAST N BITS IN SIZE.
typedef uint8 bool8;
typedef uint16 bool16;
typedef uint32 bool32;

// bitsN
//  Unit of bitwise operation, AT LEAST N BITS IN SIZE.
typedef uint8 bits8;
typedef uint16 bits16;
typedef uint32 bits32;

// wordN
//  Unit of storage, AT LEAST N BITS IN SIZE,
//  used to fetch/store data.
typedef uint8 word8;
typedef uint16 word16;
typedef uint32 word32;

// Size
//  Size of any memory resident object, as returned by sizeof.
typedef size_t Size;

// Index
//  Index into any memory resident array.
// Note:
//  Indices are non negative.
typedef unsigned int Index;

#define MAXDIM 6
typedef struct {
  int indx[MAXDIM];
} IntArray;

// Offset
//  Offset into any memory resident array.
//
// Note:
//  This differs from an Index in that an Index is always
//  non negative, whereas Offset may be negative.
typedef signed int Offset;

// ================================================
// Section 4: datum type + support macros
// ================================================

// datum.h
//  POSTGRES abstract data type datum representation definitions.
//
// Note:
//
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

typedef unsigned long Datum;
typedef Datum* DatumPtr;

#define GET_1_BYTE(datum)  (((Datum)(datum)) & 0x000000ff)
#define GET_2_BYTES(datum) (((Datum)(datum)) & 0x0000ffff)
#define GET_4_BYTES(datum) (((Datum)(datum)) & 0xffffffff)
#define SET_1_BYTE(value)  (((Datum)(value)) & 0x000000ff)
#define SET_2_BYTES(value) (((Datum)(value)) & 0x0000ffff)
#define SET_4_BYTES(value) (((Datum)(value)) & 0xffffffff)

#define DATUM_GET_BOOL(x) ((bool)(((Datum)(x)) != 0))
#define BOOL_GET_DATUM(x) ((Datum)((x) ? 1 : 0))

// Returns character value of a datum.
#define DATUM_GET_CHAR(x) ((char)GET_1_BYTE(x))

// Returns datum representation for a character.
#define CHAR_GET_DATUM(x) ((Datum)SET_1_BYTE(x))

// Returns datum representation for an 8-bit integer.
#define INT8_GET_DATUM(x) ((Datum)SET_1_BYTE(x))

// Returns 8-bit unsigned integer value of a datum.
#define DATUM_GET_UINT8(x) ((uint8)GET_1_BYTE(x))

// Returns datum representation for an 8-bit unsigned integer.
#define UINT8_GET_DATUM(x) ((Datum)SET_1_BYTE(x))

// Returns 16-bit integer value of a datum.fj_always_done
#define DATUM_GET_INT16(x) ((int16)GET_2_BYTES(x))

// Returns datum representation for a 16-bit integer.
#define INT16_GET_DATUM(x) ((Datum)SET_2_BYTES(x))

// Returns 16-bit unsigned integer value of a datum.
#define DATUM_GET_UINT16(x) ((uint16)GET_2_BYTES(x))

// Returns datum representation for a 16-bit unsigned integer.
#define UINT16_GET_DATUM(x) ((Datum)SET_2_BYTES(x))

// Returns 32-bit integer value of a datum.
#define DATUM_GET_INT32(x) ((int32)GET_4_BYTES(x))

// Returns datum representation for a 32-bit integer.
#define INT32_GET_DATUM(x) ((Datum)SET_4_BYTES(x))

// Returns 32-bit unsigned integer value of a datum.
#define DATUM_GET_UINT32(x) ((uint32)GET_4_BYTES(x))

// Returns datum representation for a 32-bit unsigned integer.
#define UINT32_GET_DATUM(x) ((Datum)SET_4_BYTES(x))

// Returns object identifier value of a datum.
#define DATUM_GET_OBJECT_ID(x) ((Oid)GET_4_BYTES(x))

// Returns datum representation for an object identifier.
#define OBJECT_ID_GET_DATUM(x) ((Datum)SET_4_BYTES(x))

// Returns pointer value of a datum.
#define DATUM_GET_POINTER(x) ((Pointer)(x))

// Returns datum representation for a pointer.
#define POINTER_GET_DATUM(x) ((Datum)(x))

#define DATUM_GET_CSTRING(x) ((char*)DATUM_GET_POINTER(x))
#define CSTRING_GET_DATUM(x) POINTER_GET_DATUM(x)

// Returns name value of a datum.
#define DATUM_GET_NAME(x) ((Name)DATUM_GET_POINTER((Datum)(x)))

// Returns datum representation for a name.
#define NAME_GET_DATUM(x) POINTER_GET_DATUM((Pointer)(x))

#define DATUM_GET_INT64(x)  (*((int64*)DATUM_GET_POINTER(x)))
#define DATUM_GET_FLOAT4(x) (*(float4*)DATUM_GET_POINTER(x))
#define DATUM_GET_FLOAT8(x) (*(float8*)DATUM_GET_POINTER(x))

// Returns 32-bit floating point value of a datum.
// This is really a pointer, of course.
#define DATUM_GET_FLOAT32(x) ((float32)DATUM_GET_POINTER(x))

// Returns datum representation for a 32-bit floating point number.
// This is really a pointer, of course.
#define FLOAT32_GET_DATUM(x) POINTER_GET_DATUM((Pointer)(x))

// Returns 64-bit floating point value of a datum.
// This is really a pointer, of course.
#define DATUM_GET_FLOAT64(x) ((float64)DATUM_GET_POINTER(x))

// Returns datum representation for a 64-bit floating point number.
// This is really a pointer, of course.
#define FLOAT64_GET_DATUM(x) POINTER_GET_DATUM((Pointer)(x))

// ================================================
// Section 5: IsValid macros for system types
// ================================================

#define BOOL_IS_VALID(boolean)            ((boolean) == false || (boolean) == true)
#define POINTER_IS_VALID(pointer)         ((void*)(pointer) != NULL)
#define POINTER_IS_ALIGNED(pointer, type) (((long)(pointer) % (sizeof(type))) == 0)

// ================================================
// Section 6: offsetof, lengthof, endof
// ================================================

#ifndef offsetof
#define offsetof(type, field) ((long)&((type*)0)->field)
#endif

// Number of elements in an array.
#define LENGTH_OF(array) (sizeof(array) / sizeof((array)[0]))

// Address of the element one past the last in an array.
#define END_OF(array) (&array[LENGTH_OF(array)])

// Alignment macros: align a length or address appropriately for a given type.
//
// There used to be some incredibly crufty platform-dependent hackery here,
// but now we rely on the configure script to get the info for us. Much nicer.
//
// NOTE: TYPEALIGN will not work if ALIGNVAL is not a power of 2.
// That case seems extremely unlikely to occur in practice, however.
#define TYPE_ALIGN(alignment, size) (((long)(size) + (alignment - 1)) & ~(alignment - 1))

#define SHORT_ALIGN(LEN)  TYPE_ALIGN(_Alignof(short), LEN)
#define INT_ALIGN(LEN)    TYPE_ALIGN(_Alignof(int), LEN)
#define LONG_ALIGN(LEN)   TYPE_ALIGN(_Alignof(long), LEN)
#define DOUBLE_ALIGN(LEN) TYPE_ALIGN(_Alignof(double), LEN)
#define MAX_ALIGN(LEN)    TYPE_ALIGN(_Alignof(max_align_t), LEN)

// ================================================
// Section 7: exception handling definitions
//            Assert, Trap, etc macros
// ================================================

// Exception Handling definitions.
typedef char* ExcMessage;
typedef struct Exception {
  ExcMessage message;
} Exception;

// USE_ASSERT_CHECKING, if defined, turns on all the assertions.
// - plai  9/5/90
//
// It should _NOT_ be defined in releases or in benchmark copies

// Trap
//  Generates an exception if the given condition is true.

// Get a bit mask of the bits set in non-int32 aligned addresses.
#define INT_ALIGN_MASK (sizeof(int32) - 1)

// MemSet
//   Exactly the same as standard library function memset(), but considerably
//   faster for zeroing small word-aligned structures (such as parsetree nodes).
//   This has to be a macro because the main point is to avoid function-call
//   overhead.
//
// We got the 64 number by testing this against the stock memset() on
// BSD/OS 3.0. Larger values were slower. bjm 1997/09/11
//
// I think the crossover point could be a good deal higher for
// most platforms, actually.  tgl 2000-03-19
#define MEMSET(start, val, len)                                                                \
  do {                                                                                         \
    int32* _start = (int32*)(start);                                                           \
    int _val = (val);                                                                          \
    Size _len = (len);                                                                         \
                                                                                               \
    if ((((long)_start) & INT_ALIGN_MASK) == 0 && (_len & INT_ALIGN_MASK) == 0 && _val == 0 && \
        _len <= MEMSET_LOOP_LIMIT) {                                                           \
      int32* _stop = (int32*)((char*)_start + _len);                                           \
      while (_start < _stop) *_start++ = 0;                                                    \
    } else                                                                                     \
      memset((char*)_start, _val, _len);                                                       \
  } while (0)

#define MEMSET_LOOP_LIMIT 64

// ================================================
// Section 11: system-specific hacks
//
//  This should be limited to things that absolutely have to be
// included in every source file. The changes should be factored
// into a separate file so that changes to one port don't require
// changes to c.h (and everyone recompiling their whole system).
// ================================================

// These are for things that are one way on Unix and another on NT.
#define NULL_DEV "/dev/null"
#define SEP_CHAR '/'

#endif  // RDBMS_C_H_
