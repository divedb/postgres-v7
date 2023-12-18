// =========================================================================
//
// fstack.h
//  Fixed format stack definitions.
//
//
// Portions Copyright (c) 1996=2000, PostgreSQL, Inc
// Portions Copyright (c) 1994, Regents of the University of California
//
// $Id: fstack.h,v 1.9 2000/01/26 05:58:09 momjian Exp $
//
// =========================================================================

// Note:
//  Fixed format stacks assist in the construction of FIFO stacks of
//  fixed format structures.  Structures which are to be stackable
//  should contain a FixedItemData component.  A stack is initilized
//  with the offset of the FixedItemData component of the structure
//  it will hold.  By doing so, push and pop operations are simplified
//  for the callers.  All references to stackable items are pointers
//  to the base of the structure instead of pointers to the
//  FixedItemData component.

#ifndef RDBMS_UTILS_FSTACK_H_
#define RDBMS_UTILS_FSTACK_H_

#include "rdbms/c.h"

// FixedItem
//  Fixed format stackable item chain component.
//
// Note:
//  Structures must contain one FixedItemData component per stack in
//  which it will be an item.
typedef struct FixedItemData FixedItemData;
typedef FixedItemData* FixedItem;

struct FixedItemData {
  FixedItem next;
};

// FixedStack
//  Fixed format stack.
typedef struct FixedStackData {
  FixedItem top;  // Top item on the stack or NULL.
  Offset offset;  // Offset from struct base to item.
} FixedStackData;

typedef FixedStackData* FixedStack;

// Initializes stack for structures with given fixed component offset.
// Exceptions:
//  BadArg if stack is invalid pointer.
void fixed_stack_init(FixedStack stack, Offset offset);

// Returns pointer to top structure on stack or NULL if empty stack.
// Exceptions:
//  BadArg if stack is invalid.
Pointer fixed_stack_pop(FixedStack stack);

// Places structure associated with pointer onto top of stack.
// Exceptions:
//  BadArg if stack is invalid.
//  BadArg if pointer is invalid.
void fixed_stack_push(FixedStack stack, Pointer pointer);

// Returns pointer to top structure of a stack. This item is not poped.
//
// Note:
//  This is not part of the normal stack interface. It is intended for
//  debugging use only.
//
// Exceptions:
//  BadArg if stack is invalid.
Pointer fixed_stack_get_top(FixedStack stack);

// Returns pointer to next structure after pointer of a stack.
//
// Note:
//  This is not part of the normal stack interface. It is intended for
//  debugging use only.
//
// Exceptions:
//  BadArg if stack is invalid.
//  BadArg if pointer is invalid.
//  BadArg if stack does not contain pointer.
Pointer fixed_stack_get_next(FixedStack stack, Pointer pointer);
#endif  // RDBMS_UTILS_FSTACK_H_
