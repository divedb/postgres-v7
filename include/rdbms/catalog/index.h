/*-------------------------------------------------------------------------
 *
 * index.h
 *	  prototypes for index.c.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: index.h,v 1.33 2001/03/22 04:00:35 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef INDEX_H
#define INDEX_H

#include "access/itup.h"
#include "nodes/execnodes.h"

extern Form_pg_am AccessMethodObjectIdGetForm(Oid accessMethodObjectId,
							MemoryContext resultCxt);

extern void UpdateIndexPredicate(Oid indexoid, Node *oldPred, Node *predicate);

extern void InitIndexStrategy(int numatts,
				  Relation indexRelation,
				  Oid accessMethodObjectId);

extern void index_create(char *heapRelationName,
			 char *indexRelationName,
			 IndexInfo *indexInfo,
			 Oid accessMethodObjectId,
			 Oid *classObjectId,
			 bool islossy,
			 bool primary,
			 bool allow_system_table_mods);

extern void index_drop(Oid indexId);

extern IndexInfo *BuildIndexInfo(HeapTuple indexTuple);

extern void FormIndexDatum(IndexInfo *indexInfo,
			   HeapTuple heapTuple,
			   TupleDesc heapDescriptor,
			   MemoryContext resultCxt,
			   Datum *datum,
			   char *nullv);

extern void UpdateStats(Oid relid, long reltuples);
extern bool IndexesAreActive(Oid relid, bool comfirmCommitted);
extern void setRelhasindex(Oid relid, bool hasindex);

#ifndef OLD_FILE_NAMING
extern void setNewRelfilenode(Relation relation);

#endif	 /* OLD_FILE_NAMING */
extern bool SetReindexProcessing(bool processing);
extern bool IsReindexProcessing(void);

extern void index_build(Relation heapRelation, Relation indexRelation,
			IndexInfo *indexInfo, Node *oldPred);

extern bool reindex_index(Oid indexId, bool force, bool inplace);
extern bool activate_indexes_of_a_table(Oid relid, bool activate);
extern bool reindex_relation(Oid relid, bool force);

#endif	 /* INDEX_H */