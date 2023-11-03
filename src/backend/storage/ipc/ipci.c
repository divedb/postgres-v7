// =========================================================================
//
// ipci.c
//  POSTGRES inter=process communication initialization code.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/storage/ipc/ipci.c,v 1.33 2000/04/12 17:15:37 momjian Exp $
//
// =========================================================================

#include "rdbms/storage/ipc.h"

// Returns a memory key given a port address.
IPCKey system_port_address_create_ipc_key(SystemPortAddress address) {
  assert(address < 32768);

  return SystemPortAddressGetIPCKey(address);
}

void create_shared_memory_and_semaphores(IPCKey key, int max_backends) {}