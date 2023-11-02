#ifndef RDBMS_UTILS_GLOBALS_H_
#define RDBMS_UTILS_GLOBALS_H_

#include "rdbms/libpq/libpq-be.h"
#include "rdbms/libpq/pgcomm.h"
#include "rdbms/storage/backendid.h"
#include "rdbms/utils/rel.h"

extern ProtocolVersion FrontendProtocol;
extern bool Noversion;
extern bool Quite;
extern bool QueryCancel;
extern int MyProcPid;
extern struct Port* MyProcPort;
// The PGDATA directory user says to use, or defaults to via environment
// variable.  NULL if no option given and no environment variable set.
extern char* DataDir;

// extern Relation RelDesc;  // Current relation descriptor.
// extern char OutputFileName[MAXPGPATH];

extern BackendId MyBackendId;
extern BackendTag MyBackendTag;

extern char* UserName;
extern char* DatabaseName;
extern char* DatabasePath;

extern bool MyDatabaseIdIsInitialized;
extern Oid MyDatabaseId;
extern bool TransactionInitWasProcessed;
extern bool IsUnderPostmaster;
extern int DebugLvl;
extern int DateStyle;
extern bool EuroDates;
extern bool HasCTZSet;
extern bool CDayLight;
extern int CTimeZone;
extern char CTZName[MAXTZLEN + 1];

extern char DateFormat[20];
extern char FloatFormat[20];
extern bool AllowSystemTableMods;
extern char* IndexedCatalogNames[];

#ifndef linux

extern char Ps_status_buffer[1024];

#endif

extern char* SharedSystemRelationNames[];

#endif  // RDBMS_UTILS_GLOBALS_H_
