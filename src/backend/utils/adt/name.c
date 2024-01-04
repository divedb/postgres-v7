//===----------------------------------------------------------------------===//
//
// name.c
//  Functions for the built-in type "name".
// name replaces char16 and is carefully implemented so that it
// is a string of length NAMEDATALEN.  DO NOT use hard-coded constants anywhere
// always use NAMEDATALEN as the symbolic constant!   - jolly 8/21/95
//
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/adt/name.c
//  v 1.31 2001/01/24 19:43:14 momjian Exp $
//
//===----------------------------------------------------------------------===//
#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/utils/builtins.h"

int name_cpy(Name n1, Name n2) {
  if (!n1 || !n2) {
    return -1;
  }

  strncpy(NAME_STR(*n1), NAME_STR(*n2), NAME_DATA_LEN);

  return 0;
}

int name_strcpy(Name name, const char* str) {
  if (!name || !str) {
    return -1;
  }

  STR_N_CPY(NAME_STR(*name), str, NAME_DATA_LEN);

  return 0;
}

int name_strcmp(Name name, const char* str) {
  if (!name && !str) {
    return 0;
  }

  if (!name) {
    // NULL < anything
    return -1;
  }

  if (!str) {
    // NULL < anything
    return 1;
  }

  return strncmp(NAME_STR(*name), str, NAME_DATA_LEN);
}