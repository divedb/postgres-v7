//===----------------------------------------------------------------------===//
//
// dest.h
//  Whenever the backend executes a query, the results
//  have to go someplace - either to the standard output,
//  to a local portal buffer or to a remote portal buffer.
//
//  - stdout is the destination only when we are running a
//    backend without a postmaster and are returning results
//    back to the user.
//
//  - a local portal buffer is the destination when a backend
//    executes a user-defined function which calls PQexec() or
//    PQfn(). In this case, the results are collected into a
//    PortalBuffer which the user's function may diddle with.
//
//  - a remote portal buffer is the destination when we are
//    running a backend with a frontend and the frontend executes
//    PQexec() or PQfn().  In this case, the results are sent
//    to the frontend via the pq_ functions.
//
//  - None is the destination when the system executes
//    a query internally.  This is not used now but it may be
//    useful for the parallel optimiser/executor.
//
// dest.c defines three functions that implement destination management:
//
// BeginCommand: initialize the destination.
// DestToFunction: return a pointer to a struct of destination-specific
// receiver functions.
// EndCommand: clean up the destination when output is complete.
//
// The DestReceiver object returned by DestToFunction may be a statically
// allocated object (for destination types that require no local state)
// or can be a palloc'd object that has DestReceiver as its first field
// and contains additional fields (see printtup.c for an example). These
// additional fields are then accessible to the DestReceiver functions
// by casting the DestReceiver* pointer passed to them.
// The palloc'd object is pfree'd by the DestReceiver's cleanup function.
//
// XXX FIXME: the initialization and cleanup code that currently appears
// in-line in BeginCommand and EndCommand probably should be moved out
// to routines associated with each destination receiver type.
//
// Portions Copyright (c) 1996-2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: dest.h,v 1.23 2000/01/26 05:58:35 momjian Exp $
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_TCOP_DEST_H_
#define RDBMS_TCOP_DEST_H_

#include "rdbms/access/htup.h"
#include "rdbms/access/tupdesc.h"

// CommandDest is a simplistic means of identifying the desired
// destination. Someday this will probably need to be improved.
typedef enum {
  ENUM_NONE,             // Results are discarded.
  ENUM_DEBUG,            // Results go to debugging output.
  ENUM_LOCAL,            // Results go in local portal buffer.
  ENUM_REMOTE,           // Results sent to frontend process.
  ENUM_REMOTE_INTERNAL,  // Results sent to frontend process in internal (binary) form.
  ENUM_SPI               // Results sent to SPI manager.
} CommandDest;

// DestReceiver is a base type for destination-specific local state.
// In the simplest cases, there is no state info, just the function
// pointers that the executor must call.
typedef struct DestReceiver DestReceiver;

struct DestReceiver {
  void (*receive_tuple)(HeapTuple tuple, TupleDesc type_info,
                        DestReceiver* self);               // Called for each tuple to be output.
  void (*setup)(DestReceiver* self, TupleDesc type_info);  // Initialization and teardown.
  void (*cleanup)(DestReceiver* self);

  // Private fields might appear beyond this point.
};

// The primary destination management functions.
void begin_command(char* pname, int operation, TupleDesc att_info, bool is_into_rel, bool is_into_portal, char* tag,
                   CommandDest dest);
DestReceiver* dest_to_function(CommandDest dest);
void end_command(char* command_tag, CommandDest dest);

// Additional functions that go with destination management, more or less.
void send_copy_begin();
void receive_copy_begin();
void null_command(CommandDest dest);
void ready_for_query(CommandDest dest);
void udpate_command_info(int operation, Oid last_oid, uint32 tuples);

#endif  // RDBMS_TCOP_DEST_H_
