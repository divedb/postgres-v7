//===----------------------------------------------------------------------===//
//
// copy.h
//  Definitions for using the POSTGRES copy command.
//
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: copy.h,v 1.11 2000/04/12 17:16:32 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_COMMANDS_COPY_H_
#define RDBMS_COMMANDS_COPY_H_

#include <stdbool.h>

extern int LineNo;

void do_copy(char* relation_name, bool binary, bool oids, bool from, bool pipe, char* filename, char* delimiter,
             char* null_print);

#endif  // RDBMS_COMMANDS_COPY_H_
