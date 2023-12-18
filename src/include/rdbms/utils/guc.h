//===----------------------------------------------------------------------===//
// guc.h
//
// External declarations pertaining to backend/utils/misc/guc.c and
// backend/utils/misc/guc-file.l
//
// $Header: /home/projects/pgsql/cvsroot/pgsql/src/include/utils/guc.h,v 1.6 2001/03/22 04:01:12 momjian Exp $
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_GUC_H_
#define RDBMS_UTILS_GUC_H_

#include <stdbool.h>

// Certain options can only be set at certain times. The rules are
// like this:
//
// POSTMASTER options can only be set when the postmaster starts,
// either from the configuration file or the command line.
//
// SIGHUP options can only be set at postmaster startup or by changing
// the configuration file and sending the HUP signal to the postmaster
// or a backend process. (Notice that the signal receipt will not be
// evaluated immediately. The postmaster and the backend block at a
// certain point in their main loop. It's safer to wait than to read a
// file asynchronously.)
//
// BACKEND options can only be set at postmaster startup or with the
// PGOPTIONS variable from the client when the connection is
// initiated. Note that you cannot change this kind of option using
// the SIGHUP mechanism, that would defeat the purpose of this being
// fixed for a given backend once started.
//
// SUSET options can be set at postmaster startup, with the SIGHUP
// mechanism, or from SQL if you're a superuser. These options cannot
// be set using the PGOPTIONS mechanism, because there is not check as
// to who does this.
//
// USERSET options can be set by anyone any time.
typedef enum { PGC_POSTMASTER, PGC_SIGHUP, PGC_BACKEND, PGC_SUSET, PGC_USERSET } GucContext;

void set_configOption(const char* name, const char* value, GucContext context);
const char* get_config_option(const char* name);
void process_config_file(GucContext context);
void reset_all_options();
void parse_long_option(const char* string, char** name, char** value);
bool set_config_option(const char* name, const char* value, GucContext context, bool do_it);

extern bool DebugPrintQuery;
extern bool DebugPrintPlan;
extern bool DebugPrintParse;
extern bool DebugPrintRewritten;
extern bool DebugPrettyPrint;

extern bool ShowParserStats;
extern bool ShowPlannerStats;
extern bool ShowExecutorStats;
extern bool ShowQueryStats;
extern bool ShowBtreeBuildStats;

extern bool SQLInheritance;

#endif  // RDBMS_UTILS_GUC_H_
