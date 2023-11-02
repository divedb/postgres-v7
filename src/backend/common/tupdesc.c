// =========================================================================
//
// tupdesc.c
//  POSTGRES tuple descriptor support code
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/access/common/tupdesc.c,v 1.62 2000/04/12 17:14:37 momjian Exp $
//
// NOTES
//  some of the executor utility code such as "ExecTypeFromTL" should be
//  moved here.
//
// =========================================================================

#include "rdbms/access/tupdesc.h"

#include "rdbms/utils/mctx.h"
#include "rdbms/utils/palloc.h"

// This function allocates and zeros a tuple descriptor structure.
TupleDesc create_template_tuple_desc(int natts) {
  uint32 size;
  TupleDesc desc;

  assert(natts >= 1);

  // Allocate enough memory for the tuple descriptor and
  // zero it as TupleDescInitEntry assumes that the descriptor
  // is filled with NULL pointers.
  size = natts * sizeof(Form_pg_attribute);
  desc = (TupleDesc)palloc(sizeof(struct tupleDesc));
  desc->attrs = (Form_pg_attribute*)palloc(size);
  desc->constr = NULL;
  MemSet(desc->attrs, 0, size);

  desc->natts = natts;

  return desc;
}

// This function allocates a new TupleDesc from Form_pg_attribute array.
TupleDesc create_tuple_desc(int natts, Form_pg_attribute* attrs) {
  TupleDesc desc;

  assert(natts >= 1);

  desc = (TupleDesc)palloc(sizeof(struct tupleDesc));
  desc->attrs = attrs;
  desc->natts = natts;
  desc->constr = NULL;

  return desc;
}

// This function creates a new TupleDesc by copying from an existing
// TupleDesc
// !!! Constraints are not copied !!!
TupleDesc create_tuple_desc_copy(TupleDesc tupdesc) {
  TupleDesc desc;
  int i;
  int size;

  desc = (TupleDesc)palloc(sizeof(struct tupleDesc));
  desc->natts = tupdesc->natts;
  size = desc->natts * sizeof(Form_pg_attribute);
  desc->attrs = (Form_pg_attribute*)palloc(size);

  for (i = 0; i < desc->natts; i++) {
    desc->attrs[i] = (Form_pg_attribute)palloc(ATTRIBUTE_TUPLE_SIZE);
    memmove(desc->attrs[i], tupdesc->attrs[i], ATTRIBUTE_TUPLE_SIZE);
    desc->attrs[i]->attnotnull = false;
    desc->attrs[i]->atthasdef = false;
  }

  desc->constr = NULL;

  return desc;
}

// This function creates a new TupleDesc by copying from an existing
// TupleDesc (with Constraints).
TupleDesc create_tuple_desc_copy_constr(TupleDesc tupdesc) {
  TupleDesc desc;
  TupleConstr* constr = tupdesc->constr;
  int i;
  int size;

  desc = (TupleDesc)palloc(sizeof(struct tupleDesc));
  desc->natts = tupdesc->natts;
  size = desc->natts * sizeof(Form_pg_attribute);
  desc->attrs = (Form_pg_attribute*)palloc(size);

  for (i = 0; i < desc->natts; i++) {
    desc->attrs[i] = (Form_pg_attribute)palloc(ATTRIBUTE_TUPLE_SIZE);
    memmove(desc->attrs[i], tupdesc->attrs[i], ATTRIBUTE_TUPLE_SIZE);
  }

  if (constr) {
    TupleConstr* cpy = (TupleConstr*)palloc(sizeof(TupleConstr));
    cpy->has_not_null = constr->has_not_null;

    if ((cpy->num_defval = constr->num_defval) > 0) {
      cpy->defval = (AttrDefault*)palloc(cpy->num_defval * sizeof(AttrDefault));
      memcpy(cpy->defval, constr->defval, cpy->num_defval * sizeof(AttrDefault));

      for (i = cpy->num_defval - 1; i >= 0; i--) {
        if (constr->defval[i].adbin) {
          cpy->defval[i].adbin = pstrdup(constr->defval[i].adbin);
        }
      }
    }

    if ((cpy->num_check = constr->num_check) > 0) {
      cpy->check = (ConstrCheck*)palloc(cpy->num_check * sizeof(ConstrCheck));
      memcpy(cpy->check, constr->check, cpy->num_check * sizeof(ConstrCheck));

      for (i = cpy->num_check - 1; i >= 0; i--) {
        if (constr->check[i].ccname) {
          cpy->check[i].ccname = pstrdup(constr->check[i].ccname);
        }

        if (constr->check[i].ccbin) {
          cpy->check[i].ccbin = pstrdup(constr->check[i].ccbin);
        }
      }
    }

    desc->constr = cpy;
  } else {
    desc->constr = NULL;
  }

  return desc;
}

void free_tuple_desc(TupleDesc tupdesc) {
  int i;

  for (i = 0; i < tupdesc->natts; i++) {
    pfree(tupdesc->attrs[i]);
  }

  pfree(tupdesc->attrs);

  if (tupdesc->constr) {
    if (tupdesc->constr->num_defval > 0) {
      AttrDefault* attrdef = tupdesc->constr->defval;

      for (i = tupdesc->constr->num_defval - 1; i >= 0; i--) {
        if (attrdef[i].adbin) {
          pfree(attrdef[i].adbin);
        }
      }

      pfree(attrdef);
    }

    if (tupdesc->constr->num_check > 0) {
      ConstrCheck* check = tupdesc->constr->check;

      for (i = tupdesc->constr->num_check - 1; i >= 0; i--) {
        if (check[i].ccname) pfree(check[i].ccname);
        if (check[i].ccbin) pfree(check[i].ccbin);
      }
      pfree(check);
    }
    pfree(tupdesc->constr);
  }

  pfre(tupdesc);
}

bool equal_tuple_descs(TupleDesc tupdesc1, TupleDesc tupdesc2) {
  int i;

  if (tupdesc1->natts != tupdesc2->natts) {
    return false;
  }

  for (i = 0; i < tupdesc1->natts; i++) {
    Form_pg_attribute attr1 = tupdesc1->attrs[i];
    Form_pg_attribute attr2 = tupdesc2->attrs[i];

    // We do not need to check every single field here, and in fact
    // some fields such as attdisbursion probably shouldn't be
    // compared.
    if (strcmp(NameStr(attr1->attname), NameStr(attr2->attname)) != 0) {
      return false;
    }

    if (attr1->atttypid != attr2->atttypid) {
      return false;
    }

    if (attr1->atttypmod != attr2->atttypmod) {
      return false;
    }

    if (attr1->attstorage != attr2->attstorage) {
      return false;
    }

    if (attr1->attnotnull != attr2->attnotnull) {
      return false;
    }
  }

  if (tupdesc1->constr != NULL) {
    TupleConstr* constr1 = tupdesc1->constr;
    TupleConstr* constr2 = tupdesc2->constr;

    if (constr2 == NULL) {
      return false;
    }

    if (constr1->num_defval != constr2->num_defval) {
      return false;
    }

    for (i = 0; i < (int)constr1->num_defval; i++) {
      AttrDefault* defval1 = constr1->defval + i;
      AttrDefault* defval2 = constr2->defval + i;

      if (defval1->adnum != defval2->adnum) {
        return false;
      }

      if (strcmp(defval1->adbin, defval2->adbin) != 0) {
        return false;
      }
    }

    if (constr1->num_check != constr2->num_check) {
      return false;
    }

    for (i = 0; i < (int)constr1->num_check; i++) {
      ConstrCheck* check1 = constr1->check + i;
      ConstrCheck* check2 = constr2->check + i;

      if (strcmp(check1->ccname, check2->ccname) != 0) {
        return false;
      }

      if (strcmp(check2->ccbin, check2->ccbin) != 0) {
        return false;
      }
    }

    if (constr1->has_not_null != constr2->has_not_null) {
      return false;
    }
  } else if (tupdesc2->constr != NULL) {
    return false;
  }

  return true;
}

// TupleDescInitEntry
//
// This function initializes a single attribute structure in
// a preallocated tuple descriptor.
bool tuple_desc_init_entry(TupleDesc desc, AttrNumber attribute_number, char* attribute_name, Oid typeid, int32 typmod,
                           int attdim, bool attisset) {}