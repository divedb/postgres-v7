// =========================================================================
//
// elog.c
//  error logger
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/error/elog.c,v 1.57 2000/04/15 19:13:08 tgl Exp $
//
// =========================================================================

#include <stdarg.h>
#include <stdio.h>

void elog(int lev, const char* fmt, ...) {
  static char buf[1024];

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  buf[n] = '\0';
  printf("%s\n", buf);
}