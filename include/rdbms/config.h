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
#define FUNC_MAX_ARGS  INDEX_MAX_KEYS

#endif  // RDBMS_CONFIG_H_