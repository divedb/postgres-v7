#ifndef RDBMS_UTILS_OSET_H_
#define RDBMS_UTILS_OSET_H_

#include "rdbms/c.h"

typedef struct OrderedElemData OrderedElemData;
typedef OrderedElemData* OrderedElem;

typedef struct OrderedSetData OrderedSetData;
typedef OrderedSetData* OrderedSet;

struct OrderedElemData {
  OrderedElem next;  // Next elem or &this->set->dummy
  OrderedElem prev;  // Previous elem or &this->set->head
  OrderedSet set;    // Parent set
};

struct OrderedSetData {
  OrderedElem head;   // First elem or &this->dummy
  OrderedElem dummy;  // (hack) Terminator == NULL
  OrderedElem tail;   // Last elem or &this->head
  Offset offset;      // Offset from struct base to elem this could be signed short int!
};

void ordered_set_init(OrderedSet set, Offset offset);
Pointer ordered_set_get_head(OrderedSet set);
Pointer ordered_elem_get_predecessor(OrderedElem elem);
Pointer ordered_elem_get_successor(OrderedElem elem);
void ordered_elem_pop(OrderedElem elem);
void ordered_elem_push_into(OrderedElem elem, OrderedSet set);

#endif  // RDBMS_UTILS_OSET_H_
