//===----------------------------------------------------------------------===//
//
// trace.h
//
//
//  Conditional trace definitions.
//
//  Massimo Dal Zotto <dz@cs.unitn.it>
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_TRACE_H_
#define RDBMS_UTILS_TRACE_H_

#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef ELOG_TIMESTAMPS

#define TIMESTAMP_SIZE 28

char* tprintf_timestamp(void);

#else

#define TIMESTAMP_SIZE 0

#endif

int tprintf(int flag, const char* fmt, ...);
int eprintf(const char* fmt, ...);
void write_syslog(int level, char* line);
void show_options();
void parse_options(char* str, bool secure);
void read_pg_options(int postgres_signal_arg);

// Trace options, used as index into pg_options.
// Must match the constants in pg_options[].
enum PgOptionEnum {
  TRACE_ALL,  // 0=trace some, 1=trace all, -1=trace none.
  TRACE_VERBOSE,
  TRACE_QUERY,
  TRACE_PLAN,
  TRACE_PARSE,
  TRACE_REWRITTEN,
  TRACE_PRETTY_PLAN,  // Indented multiline versions of trees.
  TRACE_PRETTY_PARSE,
  TRACE_PRETTY_REWRITTEN,
  TRACE_PARSERSTATS,
  TRACE_PLANNERSTATS,
  TRACE_EXECUTORSTATS,
  TRACE_SHORTLOCKS,  // Currently unused but needed, see lock.c.
  TRACE_LOCKS,
  TRACE_USERLOCKS,
  TRACE_SPINLOCKS,
  TRACE_NOTIFY,
  TRACE_MALLOC,
  TRACE_PALLOC,
  TRACE_LOCKOIDMIN,
  TRACE_LOCKRELATION,
  OPT_LOCKREADPRIORITY,  // Lock priority, see lock.c.
  OPT_DEADLOCKTIMEOUT,   // Deadlock timeout, see proc.c.
  OPT_NOFSYNC,           // Turn fsync off.
  OPT_SYSLOG,            // Use syslog for error messages.
  OPT_HOSTLOOKUP,        // Enable hostname lookup in ps_status.
  OPT_SHOWPORTNUMBER,    // Show port number in ps_status.
  NUM_PG_OPTIONS         // Must be the last item of enum.
};

extern int PgOptions[NUM_PG_OPTIONS];

#ifdef __GNUC__
#define PRINTF(args...)        tprintf1(args)
#define EPRINTF(args...)       eprintf(args)
#define TPRINTF(flag, args...) tprintf(flag, args)
#else
#define PRINTF  tprintf1
#define EPRINTF eprintf
#define TPRINTF tprintf
#endif

#endif  // RDBMS_UTILS_TRACE_H_
