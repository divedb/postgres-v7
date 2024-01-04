//===----------------------------------------------------------------------===//
//
// globals.c
//  global variable declarations
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
// $Header:
//  home/projects/pgsql/cvsroot/pgsql/src/backend/utils/init/globals.c
//  v 1.55 2001/03/22 03:59:59 momjian Exp $
//
// NOTES
//  Globals used all over the place should be declared here and not
//  in other modules.
//
//===----------------------------------------------------------------------===//
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "rdbms/catalog/catname.h"
#include "rdbms/catalog/indexing.h"
#include "rdbms/libpq/libpq_be.h"  // struct Port
#include "rdbms/libpq/pqcomm.h"
#include "rdbms/miscadmin.h"
#include "rdbms/postgres.h"
#include "rdbms/storage/backendid.h"
#include "rdbms/utils/rel.h"

ProtocolVersion FrontendProtocol = PG_PROTOCOL_LATEST;

bool Noversion = false;
bool Quiet = false;

volatile bool InterruptPending = false;
volatile bool QueryCancelPending = false;
volatile bool ProcDiePending = false;
volatile bool ImmediateInterruptOK = false;
volatile uint32 InterruptHoldoffCount = 0;
volatile uint32 CritSectionCount = 0;

int MyProcPid;
struct Port* MyProcPort;
long MyCancelKey;

// The PGDATA directory user says to use, or defaults to via environment
// variable.  NULL if no option given and no environment variable set
char* DataDir = NULL;

// TODO(gc): 这个到底是缓存什么表的描述符
Relation RelDesc;  // Current relation descriptor.

char OutputFileName[MAX_PG_PATH] = "";

BackendId MyBackendId;

char* DatabaseName = NULL;
char* DatabasePath = NULL;

Oid MyDatabaseId = INVALID_OID;

bool IsUnderPostmaster = false;

int DebugLvl = 0;

int DateStyle = USE_ISO_DATES;
bool EuroDates = false;
bool HasCTZSet = false;
bool CDayLight = false;
int CTimeZone = 0;
char CTZName[MAX_TZ_LEN + 1] = "";

char DateFormat[20] = "%d-%m-%Y";  // mjl: sizes! or better malloc? XXX.
char FloatFormat[20] = "%f";

bool EnableFsync = true;
bool AllowSystemTableMods = false;
int SortMem = 512;
int NBuffers = 16;

char* IndexedCatalogNames[] = {AttributeRelationName, ProcedureRelationName,
                               TypeRelationName, RelationRelationName, 0};

// We just do a linear search now so there's no requirement that the list
// be ordered. The list is so small it shouldn't make much difference.
// make sure the list is null-terminated
//  - jolly 8/19/95
//
// OLD COMMENT
//  WARNING WARNING WARNING WARNING WARNING WARNING
//
// Keep SharedSystemRelationNames[] in SORTED order! A binary search
// is done on it in catalog.c!
//
// XXX this is a serious hack which should be fixed -cim 1/26/90
char* SharedSystemRelationNames[] = {DatabaseRelationName, GroupRelationName,
                                     GroupNameIndex,       GroupSysidIndex,
                                     LogRelationName,      ShadowRelationName,
                                     VariableRelationName, 0};
