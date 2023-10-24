// =========================================================================
//
// oset.c
//  Fixed format ordered set definitions.
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
//
// IDENTIFICATION
//  $Header: /usr/local/cvsroot/pgsql/src/backend/utils/mmgr/oset.c,v 1.17 2000/04/12 17:16:10 momjian Exp $
//
// NOTE
//  XXX This is a preliminary implementation which lacks fail=fast
//  XXX validity checking of arguments.
//
// =========================================================================
#include "rdbms/utils/oset.h"

static void ordered_elem_push_head(OrderedElem elem) {
  elem->next = elem->set->head;
  elem->prev = (OrderedElem)&elem->set->head;
  elem->next->prev = elem;
  elem->prev->next = elem;
}

static void ordered_elem_push(OrderedElem elem) { ordered_elem_push_head(elem); }

static Pointer ordered_elem_get_base(OrderedElem elem) {
  if (elem == (OrderedElem)NULL) {
    return (Pointer)NULL;
  }

  return (Pointer)((char*)(elem) - (elem)->set->offset);
}

void ordered_set_init(OrderedSet set, Offset offset) {
  set->head = (OrderedElem)&set->dummy;
  set->dummy = NULL;
  set->tail = (OrderedElem)&set->head;
  set->offset = offset;
}

Pointer ordered_set_get_head(OrderedSet set) {
  OrderedElem elem;

  elem = set->head;

  if (elem->next) {
    return ordered_elem_get_base(elem);
  }

  return NULL;
}

Pointer ordered_elem_get_predecessor(OrderedElem elem) {
  elem = elem->prev;

  if (elem->prev) {
    return ordered_elem_get_base(elem);
  }

  return NULL;
}

Pointer ordered_elem_get_successor(OrderedElem elem) {
  elem = elem->next;

  if (elem->next) {
    return ordered_elem_get_base(elem);
  }

  return NULL;
}

void ordered_elem_pop(OrderedElem elem) {
  elem->next->prev = elem->prev;
  elem->prev->next = elem->next;
  elem->next = NULL;
  elem->prev = NULL;
}

void ordered_elem_push_into(OrderedElem elem, OrderedSet set) {
  elem->set = set;
  elem->next = NULL;
  elem->prev = NULL;
  ordered_elem_push(elem);
}
