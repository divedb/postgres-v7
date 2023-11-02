// =========================================================================
//
// globals.c
//  global variable declarations
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/init/globals.c,v 1.43 2000/05/05 03:09:43 tgl Exp $
//
// NOTES
//  Globals used all over the place should be declared here and not
//  in other modules.
//
// =========================================================================

#include "rdbms/utils/globals.h"

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/catalog/catname.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/backendid.h"

ProtocolVersion FrontendProtocol = PG_PROTOCOL_LATEST;

bool Noversion = false;
bool Quiet = false;
bool QueryCancel = false;

int MyProcPid;
struct Port* MyProcPort;
long MyCancelKey;

// The PGDATA directory user says to use, or defaults to via environment
// variable.  NULL if no option given and no environment variable set
char* DataDir = NULL;

// Relation RelDesc; /* current relation descriptor///

// char OutputFileName[MAXPGPATH] = "";

BackendId MyBackendId;
BackendTag MyBackendTag;

char* UserName = NULL;
char* DatabaseName = NULL;
char* DatabasePath = NULL;

bool MyDatabaseIdIsInitialized = false;
Oid MyDatabaseId = InvalidOid;
bool TransactionInitWasProcessed = false;

bool IsUnderPostmaster = false;

int DebugLvl = 0;

int DateStyle = USE_ISO_DATES;
bool EuroDates = false;
bool HasCTZSet = false;
bool CDayLight = false;
int CTimeZone = 0;
char CTZName[MAXTZLEN + 1] = "";

char DateFormat[20] = "%d-%m-%Y";  // mjl: sizes! or better malloc? XXX.
char FloatFormat[20] = "%f";

bool allowSystemTableMods = false;
int SortMem = 512;

char* IndexedCatalogNames[] = {AttributeRelationName, ProcedureRelationName, TypeRelationName, RelationRelationName, 0};

// ps status buffer
#ifndef linux
char Ps_status_buffer[1024];

#endif

// we just do a linear search now so there's no requirement that the list
// be ordered.	The list is so small it shouldn't make much difference.
// make sure the list is null-terminated
//				- jolly 8/19/95
//
// OLD COMMENT
//		WARNING  WARNING  WARNING  WARNING	WARNING  WARNING
//
//		keep SharedSystemRelationNames[] in SORTED order!  A binary search
//		is done on it in catalog.c!
//
//		XXX this is a serious hack which should be fixed -cim 1/26/90
char* SharedSystemRelationNames[] = {DatabaseRelationName, GroupRelationName,  GroupNameIndex,       GroupSysidIndex,
                                     LogRelationName,      ShadowRelationName, VariableRelationName, 0};