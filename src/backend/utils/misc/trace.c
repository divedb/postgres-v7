#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef USE_SYSLOG
#include <syslog.h>
#endif

#include "rdbms/utils/trace.h"

// We could support trace messages of indefinite length, as elog() does,
// but it's probably not worth the trouble.  Instead limit trace message
// length to this.
#define TRACE_MSG_MAX_LEN 4096

#ifdef USE_SYSLOG
// Global option to control the use of syslog(3) for logging:
//
//  0 stdout/stderr only
//  1 stdout/stderr + syslog
//  2 syslog only
#define UseSyslog       pg_options[OPT_SYSLOG]
#define PG_LOG_FACILITY LOG_LOCAL0
#define PG_LOG_IDENT    "postgres"
#else
#define UseSyslog 0
#endif

// Trace option names, must match the constants in trace_opts[].
static char* OptNames[] = {
    "all",  // 0=trace some, 1=trace all, -1=trace none.
    "verbose",
    "query",
    "plan",
    "parse",
    "rewritten",
    "pretty_plan",
    "pretty_parse",
    "pretty_rewritten",
    "parserstats",
    "plannerstats",
    "executorstats",
    "shortlocks",  // Currently unused but needed, see lock.c.
    "locks",
    "userlocks",
    "spinlocks",
    "notify",
    "malloc",
    "palloc",
    "lock_debug_oidmin",
    "lock_debug_relid",
    "lock_read_priority",  // Lock priority, see lock.c.
    "deadlock_timeout",    // Deadlock timeout, see proc.c.
    "nofsync",             // Turn fsync off.
    "syslog",              // Use syslog for error messages.
    "hostlookup",          // Enable hostname lookup in ps_status.
    "showportnumber",      // Show port number in ps_status.
};

// Array of trace flags which can be set or reset independently.
int PgOptions[NUM_PG_OPTIONS] = {0};

// Print a timestamp and a message to stdout if the trace flag
// indexed by the flag value is set.
int tprintf(int flag, const char* fmt, ...) {
  va_list ap;
  char line[TRACE_MSG_MAX_LEN + TIMESTAMP_SIZE + 1];

#ifdef USE_SYSLOG
  int log_level;
#endif

  if ((flag == TRACE_ALL) || (PgOptions[TRACE_ALL] > 0)) {
    // Unconditional trace or trace all option set.
  } else if (PgOptions[TRACE_ALL] == 0) {
    if ((flag < 0) || (flag >= NUM_PG_OPTIONS) || (!PgOptions[flag])) {
      return 0;
    }
  } else if (PgOptions[TRACE_ALL] < 0) {
    return 0;
  }

#ifdef ELOG_TIMESTAMPS
  strcpy(line, tprintf_timestamp());
#endif

  va_start(ap, fmt);
  vsnprintf(line + TIMESTAMP_SIZE, TRACE_MSG_MAX_LEN, fmt, ap);
  va_end(ap);

#ifdef USE_SYSLOG
  log_level = ((flag == TRACE_ALL) ? LOG_INFO : LOG_DEBUG);
  write_syslog(log_level, line + TIMESTAMP_SIZE);
#endif

  if (UseSyslog <= 1) {
    puts(line);
    fflush(stdout);
  }

  return 1;
}
