// =========================================================================
//
// elog.h
//  POSTGRES error logging definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: elog.h,v 1.16 2000/04/12 17:16:54 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_UTILS_ELOG_H_
#define RDBMS_UTILS_ELOG_H_

#define NOTICE      0     // Random info - no special action.
#define ERROR       (-1)  // User error - return to known state.
#define FATAL       1     // Fatal error - abort process.
#define REALLYFATAL 2     // Take down the other backends with me.
#define STOP        REALLYFATAL
#define DEBUG       (-2)  // Debug message.
#define LOG         DEBUG
#define NOIND       (-3)  // Debug message, don't indent as far.

#ifndef __GNUC__
void elog(int lev, const char* fmt, ...);

#else
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
void elog(int lev, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#endif

#ifndef PG_STANDALONE

int debug_file_open();

#endif

#endif  // RDBMS_UTILS_ELOG_H_
