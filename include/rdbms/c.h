#ifndef RDBMS_C_H_
#define RDBMS_C_H_

#include <errno.h>
#include <stdarg.h>
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

// ================================================
// Section 2: non-ansi C definitions
// ================================================

#define CppAsString(identifier) #identifier
#define CppConcat(x, y)         x##y

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

// Returns character value of a datum.
#define DatumGetChar(x) ((char)GET_1_BYTE(x))

// Returns datum representation for a character.
#define CharGetDatum(x) ((Datum)SET_1_BYTE(x))

// Returns datum representation for an 8-bit integer.
#define Int8GetDatum(x) ((Datum)SET_1_BYTE(x))

// Returns 8-bit unsigned integer value of a datum.
#define DatumGetUInt8(x) ((uint8)GET_1_BYTE(x))

// Returns datum representation for an 8-bit unsigned integer.
#define UInt8GetDatum(x) ((Datum)SET_1_BYTE(x))

// Returns 16-bit integer value of a datum.
#define DatumGetInt16(x) ((int16)GET_2_BYTES(x))

// Returns datum representation for a 16-bit integer.
#define Int16GetDatum(x) ((Datum)SET_2_BYTES(x))

// Returns 16-bit unsigned integer value of a datum.
#define DatumGetUInt16(x) ((uint16)GET_2_BYTES(x))

// Returns datum representation for a 16-bit unsigned integer.
#define UInt16GetDatum(x) ((Datum)SET_2_BYTES(x))

// Returns 32-bit integer value of a datum.
#define DatumGetInt32(x) ((int32)GET_4_BYTES(x))

// Returns datum representation for a 32-bit integer.
#define Int32GetDatum(x) ((Datum)SET_4_BYTES(x))

// Returns 32-bit unsigned integer value of a datum.
#define DatumGetUInt32(x) ((uint32)GET_4_BYTES(x))

// Returns datum representation for a 32-bit unsigned integer.
#define UInt32GetDatum(x) ((Datum)SET_4_BYTES(x))

// Returns object identifier value of a datum.
#define DatumGetObjectId(x) ((Oid)GET_4_BYTES(x))

// Returns datum representation for an object identifier.
#define ObjectIdGetDatum(x) ((Datum)SET_4_BYTES(x))

// Returns pointer value of a datum.
#define DatumGetPointer(x) ((Pointer)(x))

// Returns datum representation for a pointer.
#define PointerGetDatum(x) ((Datum)(x))

// Returns name value of a datum.
#define DatumGetName(x) ((Name)DatumGetPointer((Datum)(x)))

// Returns datum representation for a name.
#define NameGetDatum(x) PointerGetDatum((Pointer)(x))

// Returns 32-bit floating point value of a datum.
// This is really a pointer, of course.
#define DatumGetFloat32(x) ((float32)DatumGetPointer(x))

// Returns datum representation for a 32-bit floating point number.
// This is really a pointer, of course.
#define Float32GetDatum(x) PointerGetDatum((Pointer)(x))

// Returns 64-bit floating point value of a datum.
// This is really a pointer, of course.
#define DatumGetFloat64(x) ((float64)DatumGetPointer(x))

// Returns datum representation for a 64-bit floating point number.
// This is really a pointer, of course.
#define Float64GetDatum(x) PointerGetDatum((Pointer)(x))

// ================================================
// Section 5: IsValid macros for system types
// ================================================

#define BoolIsValid(boolean)            ((boolean) == false || (boolean) == true)
#define PointerIsValid(pointer)         ((void*)(pointer) != NULL)
#define PointerIsAligned(pointer, type) (((long)(pointer) % (sizeof(type))) == 0)

// ================================================
// Section 6: offsetof, lengthof, endof
// ================================================

#ifndef offsetof
#define offsetof(type, field) ((long)&((type*)0)->field)
#endif

// Number of elements in an array.
#define lengthof(array) (sizeof(array) / sizeof((array)[0]))

// Address of the element one past the last in an array.
#define endof(array) (&array[lengthof(array)])

#endif  // RDBMS_C_H_
