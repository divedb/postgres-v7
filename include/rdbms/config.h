#ifndef RDBMS_CONFIG_H_
#define RDBMS_CONFIG_H_

// Maximum number of columns in an index and maximum number of arguments
// to a function. They must be the same value.
//
// The minimum value is 9 (btree index creation has a 9-argument function).
//
// There is no maximum value, though if you want to pass more than 32
// arguments to a function, you will have to modify
// pgsql/src/backend/utils/fmgr/fmgr.c and add additional entries
// to the 'case' statement for the additional arguments.
#define INDEX_MAX_KEYS 16

// Maximum number of arguments to a function.
//
// The minimum value is 8 (index cost estimation uses 8-argument functions).
// The maximum possible value is around 600 (limited by index tuple size in
// pg_proc's index; BLCKSZ larger than 8K would allow more).  Values larger
// than needed will waste memory and processing time, but do not directly
// cost disk space.
//
// Changing this does not require an initdb, but it does require a full
// backend recompile (including any user-defined C functions).
#define FUNC_MAX_ARGS INDEX_MAX_KEYS
#define BLCKSZ        8192

#endif  // RDBMS_CONFIG_H_