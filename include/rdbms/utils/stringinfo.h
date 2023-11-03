// =========================================================================
//
// stringinfo.h
//  Declarations/definitions for "StringInfo" functions.
//
// StringInfo provides an indefinitely=extensible string data type.
// It can be used to buffer either ordinary C strings (null=terminated text)
// or arbitrary binary data.  All storage is allocated with palloc().
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: stringinfo.h,v 1.18 2000/04/12 17:16:34 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_STRING_INFO_H_
#define RDBMS_UTILS_STRING_INFO_H_

// StringInfoData holds information about an extensible string.
//    data      is the current buffer for the string (allocated with palloc).
//
//    len       is the current string length.  There is guaranteed to be
//              a terminating '\0' at data[len], although this is not very
//              useful when the string holds binary data rather than text.
//
//    maxlen    is the allocated size in bytes of 'data', i.e. the maximum
//              string size (including the terminating '\0' char) that we can
//              currently store in 'data' without having to reallocate
//              more space.  We must always have maxlen > len.
typedef struct StringInfoData {
  char* data;
  int len;
  int maxlen;
} StringInfoData;

typedef StringInfoData* StringInfo;

// There are two ways to create a StringInfo object initially:
//
// StringInfo stringptr = makeStringInfo();
//  Both the StringInfoData and the data buffer are palloc'd.
//
// StringInfoData string;
// initStringInfo(&string);
//  The data buffer is palloc'd but the StringInfoData is just local.
//  This is the easiest approach for a StringInfo object that will
//  only live as long as the current routine.
//
// To destroy a StringInfo, pfree() the data buffer, and then pfree() the
// StringInfoData if it was palloc'd.  There's no special support for this.
//
// NOTE: some routines build up a string using StringInfo, and then
// release the StringInfoData but return the data string itself to their
// caller.  At that point the data string looks like a plain palloc'd
// string.

// Create an empty 'StringInfoData' & return a pointer to it.
StringInfo make_string_info();

// Initialize a StringInfoData struct (with previously undefined contents)
// to describe an empty string.
void init_string_info(StringInfo str);

// Format text data under the control of fmt (an sprintf-like format string)
// and append it to whatever is already in str.  More space is allocated
// to str if necessary.  This is sort of like a combination of sprintf and
// strcat.
void append_string_info(StringInfo str, const char* fmt, ...);

// Append a single byte to str.
// Like appendStringInfo(str, "%c", ch) but much faster.
void append_string_info_char(StringInfo str, char ch);

// As above, but a macro for even more speed where it matters.
// Caution: str argument will be evaluated multiple times.
#define APPEND_STRING_INFO_CHAR_MACRO(str, ch)                          \
  (((str)->len + 1 >= (str)->maxlen) ? append_string_info_char(str, ch) \
                                     : (void)((str)->data[(str)->len] = (ch), (str)->data[++(str)->len] = '\0'))

// Append arbitrary binary data to a StringInfo, allocating more space
// if necessary.
void append_binary_string_info(StringInfo str, const char* data, int datalen);

#endif  // RDBMS_UTILS_STRING_INFO_H_
