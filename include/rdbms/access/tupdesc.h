// =========================================================================
//
// tupdesc.h
//  POSTGRES tuple descriptor definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: tupdesc.h,v 1.28 2000/04/12 17:16:26 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_ACCESS_TUP_DESC_H_
#define RDBMS_ACCESS_TUP_DESC_H_

#include "rdbms/access/attnum.h"
#include "rdbms/catalog/pg_attribute.h"

typedef struct attrDefault {
  AttrNumber adnum;
  char* adbin;  // nodeToString representation of expr.
} AttrDefault;

typedef struct ConstrCheck {
  char* ccname;
  char* ccbin;  // nodeToString representation of expr.
} ConstrCheck;

// This structure contains constraints of a tuple.
typedef struct TupleConstr {
  AttrDefault* defval;  // array.
  ConstrCheck* check;   // array.
  uint16 num_defval;
  uint16 num_check;
  bool has_not_null;
} TupleConstr;

// This structure contains all information (i.e. from Classes
// pg_attribute, pg_attrdef, pg_relcheck) for a tuple.
typedef struct TupleDescData {
  int natts;                 // Number of attributes in the tuple.
  Form_pg_attribute* attrs;  // attrs[N] is a pointer to the description of Attribute Number N + 1.
  TupleConstr* constr;
} TupleDescData;

typedef TupleDescData* TupleDesc;

TupleDesc create_template_tuple_desc(int natts);
TupleDesc create_tuple_desc(int natts, Form_pg_attribute* attrs);
TupleDesc create_tuple_desc_copy(TupleDesc tupdesc);
TupleDesc create_tuple_desc_copy_constr(TupleDesc tupdesc);
void free_tuple_desc(TupleDesc tupdesc);
bool equal_tuple_descs(TupleDesc tupdesc1, TupleDesc tupdesc2);
bool tuple_desc_init_entry(TupleDesc desc, AttrNumber attribute_number, char* attribute_name, Oid typeid, int32 typmod,
                           int attdim, bool attisset);

#endif  // RDBMS_ACCESS_TUP_DESC_H_
