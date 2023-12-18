// =========================================================================
//
// backendid.h
//  POSTGRES backend id communication definitions
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: backendid.h,v 1.7 2000/01/26 05:58:32 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_STORAGE_BACKEND_ID_H_
#define RDBMS_STORAGE_BACKEND_ID_H_

#include "rdbms/c.h"

// -cim 8/17/90
typedef int16 BackendId;   // Unique currently active backend identifier.
typedef int32 BackendTag;  // Unique backend identifier.

#define InvalidBackendId  (-1)
#define InvalidBackendTag (-1)

#endif  // RDBMS_STORAGE_BACKEND_ID_H_
