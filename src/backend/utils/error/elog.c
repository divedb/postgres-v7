//===----------------------------------------------------------------------===//
//
// elog.c
//  Error logger.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/error/elog.c,v 1.57 2000/04/15 19:13:08 tgl Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/utils/elog.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

#include "rdbms/commands/copy.h"
#include "rdbms/miscadmin.h"
#include "rdbms/tcop/dest.h"
#include "rdbms/utils/trace.h"

extern int errno;
extern const int sys_nerr;

extern CommandDest WhereToSendOutput;

#ifdef USE_SYSLOG
// Global option to control the use of syslog(3) for logging:
//
//  0 stdout/stderr only
//  1 stdout/stderr + syslog
//  2 syslog only
#define USE_SYS_LOG     PgOptions[OPT_SYSLOG]
#define PG_LOG_FACILITY LOG_LOCAL0
#else
#define USE_SYS_LOG 0
#endif

static int DebugFile = -1;
static int ErrFile = -1;
static int ELogDebugIndentLevel = 0;

// Primary error logging function.
//
// 'lev': error level; indicates recovery action to take, if any.
// 'fmt': a printf-style string.
// Additional arguments, if any, are formatted per %-escapes in 'fmt'.
//
// In addition to the usual %-escapes recognized by printf, "%m" in
// fmt is replaced by the error message for the current value of errno.
//
// Note: no newline is needed at the end of the fmt string, since
// elog will provide one for the output methods that need it.
//
// If 'lev' is ERROR or worse, control does not return to the caller.
// See elog.h for the error level definitions.
void elog(int lev, const char* fmt, ...) {
  static char buf[1024];
  va_list ap;

  va_start(ap, fmt);

  vsnprintf(buf, sizeof(buf), fmt, ap);

  va_end(ap);

  //   va_list ap;

  //   // The expanded format and final output message are dynamically
  //   // allocated if necessary, but not if they fit in the "reasonable
  //   // size" buffers shown here.  In extremis, we'd rather depend on
  //   // having a few hundred bytes of stack space than on malloc() still
  //   // working (since memory-clobber errors often take out malloc first).
  //   // Don't make these buffers unreasonably large though, on pain of
  //   // having to chase a bug with no error message.
  //   char fmt_fixedbuf[128];
  //   char msg_fixedbuf[256];
  //   char* fmt_buf = fmt_fixedbuf;
  //   char* msg_buf = msg_fixedbuf;

  //   // This buffer is only used if errno has a bogus value:
  //   char errorstr_buf[32];
  //   const char* errorstr;
  //   const char* prefix;
  //   const char* cp;
  //   char* bp;
  //   int indent = 0;
  //   int space_needed;

  // #ifdef USE_SYSLOG
  //   int log_level;
  // #endif

  //   // Ignore debug msgs if noplace to send.
  //   if (lev <= DEBUG && DebugFile < 0) {
  //     return;
  //   }

  //   if (lev == ERROR || lev == FATAL) {
  //     // This is probably redundant.
  //     if (IS_INIT_PROCESSING_MODE()) {
  //       lev = FATAL;
  //     }
  //   }

  //   // Choose message prefix and indent level.
  //   switch (lev) {
  //     case NOIND:
  //       indent = ELogDebugIndentLevel - 1;
  //       if (indent < 0) {
  //         indent = 0;
  //       }

  //       if (indent > 30) {
  //         indent = indent % 30;
  //       }

  //       prefix = "DEBUG:  ";
  //       break;

  //     case DEBUG:
  //       indent = ELogDebugIndentLevel;
  //       if (indent < 0) {
  //         indent = 0;
  //       }

  //       if (indent > 30) {
  //         indent = indent % 30;
  //       }

  //       prefix = "DEBUG:  ";
  //       break;

  //     case NOTICE:
  //       prefix = "NOTICE:  ";
  //       break;

  //     case ERROR:
  //       prefix = "ERROR:  ";
  //       break;

  //     default:
  //       // Temporarily use msg buf for prefix.
  //       sprintf(msg_fixedbuf, "FATAL %d:  ", lev);
  //       prefix = msg_fixedbuf;
  //       break;
  //   }

  //   // Get errno string for %m.
  //   if (errno < sys_nerr && errno >= 0) {
  //     errorstr = strerror(errno);
  //   } else {
  //     sprintf(errorstr_buf, "error %d", errno);
  //     errorstr = errorstr_buf;
  //   }

  //   // Set up the expanded format, consisting of the prefix string plus
  //   // input format, with any %m replaced by strerror() string (since
  //   // vsnprintf won't know what to do with %m).  To keep space
  //   // calculation simple, we only allow one %m.
  //   space_needed = TIMESTAMP_SIZE + strlen(prefix) + indent + (LineNo ? 24 : 0) + strlen(fmt) + strlen(errorstr) + 1;
  //   if (space_needed > (int)sizeof(fmt_fixedbuf)) {
  //     fmt_buf = (char*)malloc(space_needed);
  //     if (fmt_buf == NULL) {
  //       // We're up against it, convert to fatal out-of-memory error.
  //       fmt_buf = fmt_fixedbuf;
  //       lev = REALLYFATAL;
  //       fmt = "elog: out of memory";  // This must fit in fmt_fixedbuf!
  //     }
  //   }

  // #ifdef ELOG_TIMESTAMPS
  //   strcpy(fmt_buf, tprintf_timestamp());
  //   strcat(fmt_buf, prefix);
  // #else
  //   strcpy(fmt_buf, prefix);
  // #endif

  //   bp = fmt_buf + strlen(fmt_buf);
  //   while (indent-- > 0) {
  //     *bp++ = ' ';
  //   }

  //   // If error was in CopyFrom() print the offending line number -- dz.
  //   if (LineNo) {
  //     sprintf(bp, "copy: line %d, ", lineno);
  //     bp += strlen(bp);
  //     if (lev == ERROR || lev >= FATAL) {
  //       LineNo = 0;
  //     }
  //   }

  //   for (cp = fmt; *cp; cp++) {
  //     if (cp[0] == '%' && cp[1] != '\0') {
  //       if (cp[1] == 'm') {
  //         // XXX If there are any %'s in errorstr then vsnprintf
  //         // will do the Wrong Thing; do we need to cope? Seems
  //         // unlikely that % would appear in system errors.
  //         strcpy(bp, errorstr);

  //         // Copy the rest of fmt literally, since we can't afford
  //         // to insert another %m.
  //         strcat(bp, cp + 2);
  //         bp += strlen(bp);
  //         break;
  //       } else {
  //         // Copy % and next char --- this avoids trouble with %%m.
  //         *bp++ = *cp++;
  //         *bp++ = *cp;
  //       }
  //     } else
  //       *bp++ = *cp;
  //   }
  //   *bp = '\0';

  //   // Now generate the actual output text using vsnprintf(). Be sure to
  //   // leave space for \n added later as well as trailing null.
  //   space_needed = sizeof(msg_fixedbuf);
  //   for (;;) {
  //     int nprinted;

  //     va_start(ap, fmt);
  //     nprinted = vsnprintf(msg_buf, space_needed - 2, fmt_buf, ap);
  //     va_end(ap);

  //     // Note: some versions of vsnprintf return the number of chars
  //     // actually stored, but at least one returns -1 on failure. Be
  //     // conservative about believing whether the print worked.
  //     if (nprinted >= 0 && nprinted < space_needed - 3) break;
  //     // It didn't work, try to get a bigger buffer.
  //     if (msg_buf != msg_fixedbuf) {
  //       free(msg_buf);
  //     }

  //     space_needed *= 2;
  //     msg_buf = (char*)malloc(space_needed);
  //     if (msg_buf == NULL) {
  //       // We're up against it, convert to fatal out-of-memory error.
  //       msg_buf = msg_fixedbuf;
  //       lev = REALLYFATAL;

  // #ifdef ELOG_TIMESTAMPS
  //       strcpy(msg_buf, tprintf_timestamp());
  //       strcat(msg_buf, "FATAL:  elog: out of memory");
  // #else
  //       strcpy(msg_buf, "FATAL:  elog: out of memory");
  // #endif
  //       break;
  //     }
  //   }

  // // Message prepared; send it where it should go.
  // #ifdef USE_SYSLOG
  //   switch (lev) {
  //     case NOIND:
  //       log_level = LOG_DEBUG;
  //       break;
  //     case DEBUG:
  //       log_level = LOG_DEBUG;
  //       break;
  //     case NOTICE:
  //       log_level = LOG_NOTICE;
  //       break;
  //     case ERROR:
  //       log_level = LOG_WARNING;
  //       break;
  //     case FATAL:
  //     default:
  //       log_level = LOG_ERR;
  //       break;
  //   }
  //   write_syslog(log_level, msg_buf + TIMESTAMP_SIZE);
  // #endif

  //   // syslog doesn't want a trailing newline, but other destinations do.
  //   strcat(msg_buf, "\n");

  //   int len = strlen(msg_buf);

  //   if (DebugFile >= 0 && USE_SYS_LOG <= 1) {
  //     write(DebugFile, msg_buf, len);
  //   }

  //   // If there's an error log file other than our channel to the
  //   // front-end program, write to it first.  This is important because
  //   // there's a bug in the socket code on ultrix.  If the front end has
  //   // gone away (so the channel to it has been closed at the other end),
  //   // then writing here can cause this backend to exit without warning
  //   // that is, write() does an exit(). In this case, our only hope of
  //   // finding out what's going on is if Err_file was set to some disk
  //   // log.  This is a major pain.	(It's probably also long-dead code...
  //   // does anyone still use ultrix?)
  //   if (lev > DEBUG && ErrFile >= 0 && DebugFile != ErrFile && USE_SYS_LOG <= 1) {
  //     if (write(ErrFile, msg_buf, len) < 0) {
  //       write(open("/dev/console", O_WRONLY, 0666), msg_buf, len);
  //       lev = REALLYFATAL;
  //     }

  //     fsync(ErrFile);
  //   }

  // #ifndef PG_STANDALONE

  //   if (lev > DEBUG && WhereToSendOutput == ENUM_REMOTE) {
  //     // Send IPC message to the front-end program.
  //     char msgtype;

  //     if (lev == NOTICE) {
  //       msgtype = 'N';
  //     } else {
  //       // Abort any COPY OUT in progress when an error is detected.
  //       // This hack is necessary because of poor design of copy
  //       // protocol.
  //       pq_endcopyout(true);
  //       msgtype = 'E';
  //     }
  //     /* exclude the timestamp from msg sent to frontend */
  //     pq_puttextmessage(msgtype, msg_buf + TIMESTAMP_SIZE);

  //     /*
  //      * This flush is normally not necessary, since postgres.c will
  //      * flush out waiting data when control returns to the main loop.
  //      * But it seems best to leave it here, so that the client has some
  //      * clue what happened if the backend dies before getting back to
  //      * the main loop ... error/notice messages should not be a
  //      * performance-critical path anyway, so an extra flush won't hurt
  //      * much ...
  //      */
  //     pq_flush();
  //   }

  //   if (lev > DEBUG && WhereToSendOutput != Remote) {
  //     /*
  //      * We are running as an interactive backend, so just send the
  //      * message to stderr.
  //      */
  //     fputs(msg_buf, stderr);
  //   }

  // #endif /* !PG_STANDALONE */

  //   /* done with the message, release space */
  //   if (fmt_buf != fmt_fixedbuf) free(fmt_buf);
  //   if (msg_buf != msg_fixedbuf) free(msg_buf);

  //   /*
  //    * Perform error recovery action as specified by lev.
  //    */
  //   if (lev == ERROR || lev == FATAL) {
  //     /*
  //      * If we have not yet entered the main backend loop (ie, we are in
  //      * the postmaster or in backend startup), then go directly to
  //      * proc_exit.  The same is true if anyone tries to report an error
  //      * after proc_exit has begun to run.  (It's proc_exit's
  //      * responsibility to see that this doesn't turn into infinite
  //      * recursion!)	But in the latter case, we exit with nonzero exit
  //      * code to indicate that something's pretty wrong.
  //      */
  //     if (proc_exit_inprogress || !Warn_restart_ready) {
  //       fflush(stdout);
  //       fflush(stderr);
  //       ProcReleaseSpins(NULL); /* get rid of spinlocks we hold */
  //       ProcReleaseLocks();     /* get rid of real locks we hold */
  //       /* XXX shouldn't proc_exit be doing the above?? */
  //       proc_exit((int)proc_exit_inprogress);
  //     }

  //     /*
  //      * Guard against infinite loop from elog() during error recovery.
  //      */
  //     if (InError) elog(REALLYFATAL, "elog: error during error recovery, giving up!");
  //     InError = true;

  //     /*
  //      * Otherwise we can return to the main loop in postgres.c. In the
  //      * FATAL case, postgres.c will call proc_exit, but not till after
  //      * completing a standard transaction-abort sequence.
  //      */
  //     ProcReleaseSpins(NULL); /* get rid of spinlocks we hold */
  //     if (lev == FATAL) ExitAfterAbort = true;
  //     siglongjmp(Warn_restart, 1);
  //   }

  //   if (lev > FATAL) {
  //     /*
  //      * Serious crash time. Postmaster will observe nonzero process
  //      * exit status and kill the other backends too.
  //      *
  //      * XXX: what if we are *in* the postmaster?  proc_exit() won't kill
  //      * our children...
  //      */
  //     fflush(stdout);
  //     fflush(stderr);
  //     proc_exit(lev);
  //   }
}