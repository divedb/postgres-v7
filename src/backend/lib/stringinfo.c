//===----------------------------------------------------------------------===//
//
// stringinfo.c
//
// StringInfo provides an indefinitely=extensible string data type.
// It can be used to buffer either ordinary C strings (null=terminated text)
// or arbitrary binary data.  All storage is allocated with palloc().
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//  $Id: stringinfo.c,v 1.25 2000/04/12 17:15:11 momjian Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/lib/stringinfo.h"

#include <stdarg.h>

#include "rdbms/utils/elog.h"
#include "rdbms/utils/palloc.h"

StringInfo make_string_info() {
  StringInfo res;

  res = (StringInfo)palloc(sizeof(StringInfoData));

  if (res == NULL) {
    elog(ERROR, "%s: out of memory", __func__);
  }

  init_string_info(res);

  return res;
}

void init_string_info(StringInfo str) {
  int size = 256;

  str->data = (char*)palloc(size);

  if (str->data == NULL) {
    elog(ERROR, "%s: out of memory (%d bytes requested)", size);
  }

  str->maxlen = size;
  str->len = 0;
  str->data[0] = '\0';
}

// Internal routine: make sure there is enough space for 'needed' more bytes
// ('needed' does not include the terminating null).
static void enlarge_string_info(StringInfo str, int needed) {
  int newlen;

  needed += str->len + 1;

  if (needed <= str->maxlen) {
    return;
  }

  // We don't want to allocate just a little more space with each
  // append; for efficiency, double the buffer size each time it
  // overflows. Actually, we might need to more than double it if
  // 'needed' is big...
  newlen = 2 * str->maxlen;

  while (needed > newlen) {
    newlen = 2 * newlen;
  }

  str->data = (char*)repalloc(str->data, newlen);

  if (str->data == NULL) {
    elog(ERROR, "%s: out of memory (%d bytes requested)", newlen);
  }

  str->maxlen = newlen;
}

void append_string_info(StringInfo str, const char* fmt, ...) {
  va_list args;
  int avail;
  int nprinted;

  assert(str != NULL);

  for (;;) {
    // Try to format the given string into the available space;
    // but if there's hardly any space, don't bother trying,
    // just fall through to enlarge the buffer first.
    avail = str->maxlen - str->len - 1;

    if (avail > 16) {
      va_start(args, fmt);
      nprinted = vsnprintf(str->data + str->len, avail, fmt, args);
      va_end(args);

      // Note: some versions of vsnprintf return the number of chars
      // actually stored, but at least one returns -1 on failure. Be
      // conservative about believing whether the print worked.
      if (nprinted >= 0 && nprinted < avail - 1) {
        // Success. Note nprinted does not include trailing null.
        str->len += nprinted;
        break;
      }
    }

    // Double the buffer size and try again.
    enlarge_string_info(str, str->maxlen);
  }
}

void append_string_info_char(StringInfo str, char ch) {
  if (str->len + 1 >= str->maxlen) {
    enlarge_string_info(str, 1);
  }

  str->data[str->len] = ch;
  str->len++;
  str->data[str->len] = '\0';
}

void append_binary_string_info(StringInfo str, const char* data, int datalen) {
  assert(str != NULL);

  enlarge_string_info(str, datalen);
  memcpy(str->data + str->len, data, datalen);
  str->len += datalen;
  str->data[str->len] = '\0';
}