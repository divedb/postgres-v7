//===----------------------------------------------------------------------===//
//
// tupmacs.h
//  Tuple macros used by both index tuples and heap tuples.
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: tupmacs.h,v 1.17 2001/03/22 04:00:31 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_ACCESS_TUP_MACS_H_
#define RDBMS_ACCESS_TUP_MACS_H_

#include "rdbms/utils/memutils.h"

// Check to see if the ATT'th bit of an array of 8-bit bytes is set.
#define ATT_IS_NULL(att, bits) (!((bits)[(att) >> 3] & (1 << ((att)&0x07))))

// Given a Form_pg_attribute and a pointer into a tuple's data area,
// return the correct value or pointer.
//
// We return a Datum value in all cases.  If the attribute has "byval" false,
// we return the same pointer into the tuple data area that we're passed.
// Otherwise, we return the correct number of bytes fetched from the data
// area and extended to Datum form.
//
// On machines where Datum is 8 bytes, we support fetching 8-byte byval
// attributes; otherwise, only 1, 2, and 4-byte values are supported.
//
// Note that T must already be properly aligned for this to work correctly.
#define FETCH_ATT(a, t) FETCH_ATT_AUX(t, (a)->attbyval, (a)->attlen)

#if SIZEOF_DATUM == 8

#define FETCH_ATT_AUX(T, attbyval, attlen)                              \
  ((attbyval) ? ((attlen) == (int)sizeof(Datum)                         \
                     ? *((Datum*)(T))                                   \
                     : ((attlen) == (int)sizeof(int32)                  \
                            ? INT32_GET_DATUM(*((int32*)(T)))           \
                            : ((attlen) == (int)sizeof(int16)           \
                                   ? INT16_GET_DATUM(*((int16*)(T)))    \
                                   : (ASSERT_MACRO((attlen) == 1),      \
                                      CHAR_GET_DATUM(*((char*)(T))))))) \
              : POINTER_GET_DATUM((char*)(T)))

#endif

// att_align aligns the given offset as needed for a datum of length attlen
// and alignment requirement attalign.	In practice we don't need the length.
// The attalign cases are tested in what is hopefully something like their
// frequency of occurrence.
#define ATT_ALIGN(cur_offset, attlen, attalign)                          \
  (((attalign) == 'i')                                                   \
       ? INT_ALIGN(cur_offset)                                           \
       : (((attalign) == 'c')                                            \
              ? ((long)(cur_offset))                                     \
              : (((attalign) == 'd') ? DOUBLE_ALIGN(cur_offset)          \
                                     : (ASSERT_MACRO((attalign) == 's'), \
                                        SHORT_ALIGN(cur_offset)))))

// att_addlength increments the given offset by the length of the attribute.
// attval is only accessed if we are dealing with a varlena attribute.
#define ATT_ADD_LENGTH(cur_offset, attlen, attval) \
  (((attlen) != -1) ? ((cur_offset) + (attlen))    \
                    : ((cur_offset) + VARATT_SIZE(DATUM_GET_POINTER(attval))))

/*
 * store_att_byval is a partial inverse of fetch_att: store a given Datum
 * value into a tuple data area at the specified address.  However, it only
 * handles the byval case, because in typical usage the caller needs to
 * distinguish by-val and by-ref cases anyway, and so a do-it-all macro
 * wouldn't be convenient.
 */
#if SIZEOF_DATUM == 8

#define STORE_ATT_BY_VAL(T, newdatum, attlen)                       \
  do {                                                              \
    switch (attlen) {                                               \
      case sizeof(char):                                            \
        *(char*)(T) = DATUM_GET_CHAR(newdatum);                     \
        break;                                                      \
      case sizeof(int16):                                           \
        *(int16*)(T) = DATUM_GET_INT_16(newdatum);                  \
        break;                                                      \
      case sizeof(int32):                                           \
        *(int32*)(T) = DATUM_GET_INT32(newdatum);                   \
        break;                                                      \
      case sizeof(Datum):                                           \
        *(Datum*)(T) = (newdatum);                                  \
        break;                                                      \
      default:                                                      \
        elog(ERROR, "store_att_byval: unsupported byval length %d", \
             (int)(attlen));                                        \
        break;                                                      \
    }                                                               \
  } while (0)
#endif

#endif  // RDBMS_ACCESS_TUP_MACS_H_
