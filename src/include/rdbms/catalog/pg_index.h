/*-------------------------------------------------------------------------
 *
 * pg_index.h
 *	  definition of the system "index" relation (pg_index)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_index.h,v 1.16 2001/01/24 19:43:21 momjian Exp $
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_INDEX_H
#define PG_INDEX_H

/* ----------------
 *		postgres.h contains the system type definintions and the
 *		CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *		can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *		pg_index definition.  cpp turns this into
 *		typedef struct FormData_pg_index.  The oid of the index relation
 *		is stored in indexrelid; the oid of the indexed relation is stored
 *		in indrelid.
 * ----------------
 */

/*
 * it seems that all variable length fields should go at the _end_,
 * because the system cache routines only copy the fields up to the
 * first variable length field.  so I moved indislossy, indhaskeytype,
 * and indisunique before indpred.	--djm 8/20/96
 */
CATALOG(pg_index)
{
	Oid			indexrelid;
	Oid			indrelid;
	Oid			indproc;		/* registered procedure for functional
								 * index */
	int2vector	indkey;
	oidvector	indclass;
	bool		indisclustered;
	bool		indislossy;		/* do we fetch false tuples (lossy
								 * compression)? */
	bool		indhaskeytype;	/* does key type != attribute type? */
	bool		indisunique;	/* is this a unique index? */
	bool		indisprimary;	/* is this index for primary key */
	Oid			indreference;	/* oid of index of referenced relation (ie
								 * - this index for foreign key */
	text		indpred;		/* query plan for partial index predicate */
} FormData_pg_index;

/* ----------------
 *		Form_pg_index corresponds to a pointer to a tuple with
 *		the format of pg_index relation.
 * ----------------
 */
typedef FormData_pg_index *Form_pg_index;

/* ----------------
 *		compiler constants for pg_index
 * ----------------
 */
#define Natts_pg_index					12
#define Anum_pg_index_indexrelid		1
#define Anum_pg_index_indrelid			2
#define Anum_pg_index_indproc			3
#define Anum_pg_index_indkey			4
#define Anum_pg_index_indclass			5
#define Anum_pg_index_indisclustered	6
#define Anum_pg_index_indislossy		7
#define Anum_pg_index_indhaskeytype		8
#define Anum_pg_index_indisunique		9
#define Anum_pg_index_indisprimary		10
#define Anum_pg_index_indreference		11
#define Anum_pg_index_indpred			12

#endif	 /* PG_INDEX_H */
