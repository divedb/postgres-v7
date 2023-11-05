// =========================================================================
//
// pg_list.h
//  POSTGRES generic list package
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: pg_list.h,v 1.17 2000/04/12 17:16:40 momjian Exp $
//
// =========================================================================

#ifndef RDBMS_NODES_PG_LIST_H_
#define RDBMS_NODES_PG_LIST_H_

#include "rdbms/nodes/nodes.h"

// Value node.
//
// The same Value struct is used for three node types: T_Integer,
// T_Float, and T_String.  Integral values are actually represented
// by a machine integer, but both floats and strings are represented
// as strings.	Using T_Float as the node type simply indicates that
// the contents of the string look like a valid numeric literal.
//
// (Before Postgres 7.0, we used a double to represent T_Float,
// but that creates loss-of-precision problems when the value is
// ultimately destined to be converted to NUMERIC.	Since Value nodes
// are only used in the parsing process, not for runtime data, it's
// better to use the more general representation.)
//
// Note that an integer-looking string will get lexed as T_Float if
// the value is too large to fit in a 'long'.
typedef struct Value {
  NodeTag type;  // Tag appropriately (eg. T_String).
  union ValUnion {
    long ival;  // Machine integer.
    char* str;  // String.
  } val;
} Value;

#define INT_VAL(v)   (((Value*)(v))->val.ival)
#define FLOAT_VAL(v) atof(((Value*)(v))->val.str)
#define STR_VAL(v)   (((Value*)(v))->val.str)

typedef struct List {
  NodeTag type;
  union {
    void* ptr_value;
    int int_value;
  } elem;
  struct List* next;
} List;

#define NIL ((List*)NULL)

// Anything that doesn't end in 'i' is assumed to be referring to the
// pointer version of the list (where it makes a difference).
#define LFIRST(l)  ((l)->elem.ptr_value)
#define LNEXT(l)   ((l)->next)
#define LSECOND(l) LFIRST(LNEXT(l))
#define LFIRSTI(l) ((l)->elem.int_value)

// A convenience macro which loops through the list.
#define FOR_EACH(_elt_, _list_) for (_elt_ = (_list_); _elt_ != NIL; _elt_ = LNEXT(_elt_))

int length(List* list);
List* nconc(List* list1, List* list2);
List* lcons(void* datum, List* list);
List* lconsi(int datum, List* list);
bool member(void* datum, List* list);
bool int_member(int datum, List* list);
Value* make_integer(long i);
Value* make_float(char* numeric_str);
Value* make_string(char* str);
List* make_list(void* elem, ...);
List* lappend(List* list, void* datum);
List* lappendi(List* list, int datum);
List* lremove(void* elem, List* list);
List* lisp_remove(void* elem, List* list);
List* ltruncate(int n, List* list);

void* nth(int n, List* l);
int nthi(int n, List* l);
void set_nth(List* l, int n, void* elem);

List* set_difference(List* list1, List* list2);
List* set_differencei(List* list1, List* list2);
List* lisp_union(List* list1, List* list2);
List* lisp_unioni(List* list1, List* list2);

bool same_seti(List* list1, List* list2);
bool non_overlap_setsi(List* list1, List* list2);
bool is_subseti(List* list1, List* list2);

void free_list(List* list);

#endif  // RDBMS_NODES_PG_LIST_H_
