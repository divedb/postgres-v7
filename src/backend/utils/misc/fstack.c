#include "rdbms/utils/fstack.h"

#include <assert.h>
#include <stdbool.h>

#define FIXED_ITEM_IS_VALID(item)              POINTER_IS_VALID(item)
#define FIXED_STACK_GET_ITEM_BASE(stack, item) ((Pointer)((char*)(item) - (stack)->offset))
#define FIXED_STACK_GET_ITEM(stack, pointer)   ((FixedItem)((char*)(pointer) + (stack)->offset))
#define FIXED_STACK_IS_VALID(stack)            ((bool)POINTER_IS_VALID(stack))

void fixed_stack_init(FixedStack stack, Offset offset) {
  assert(POINTER_IS_VALID(stack));

  stack->top = NULL;
  stack->offset = offset;
}

Pointer fixed_stack_pop(FixedStack stack) {
  Pointer pointer;

  assert(FIXED_STACK_IS_VALID(stack));

  if (!POINTER_IS_VALID(stack->top)) {
    return NULL;
  }

  pointer = FIXED_STACK_GET_ITEM_BASE(stack, stack->top);
  stack->top = stack->top->next;

  return pointer;
}

void fixed_stack_push(FixedStack stack, Pointer pointer) {
  FixedItem item = FIXED_STACK_GET_ITEM(stack, pointer);

  assert(FIXED_STACK_IS_VALID(stack));
  assert(POINTER_IS_VALID(pointer));

  item->next = stack->top;
  stack->top = item;
}

Pointer fixed_stack_get_top(FixedStack stack) {
  assert(FIXED_STACK_IS_VALID(stack));

  if (!POINTER_IS_VALID(stack->top)) {
    return NULL;
  }

  return FIXED_STACK_GET_ITEM_BASE(stack, stack->top);
}

static bool fixed_stack_contains(FixedStack stack, Pointer pointer) {
  FixedItem next;
  FixedItem item;

  assert(FIXED_STACK_IS_VALID(stack));
  assert(POINTER_IS_VALID(pointer));

  item = FIXED_STACK_GET_ITEM(stack, pointer);

  for (next = stack->top; FIXED_ITEM_IS_VALID(next); next = next->next) {
    if (next == item) {
      return true;
    }
  }

  return false;
}

Pointer fixed_stack_get_next(FixedStack stack, Pointer pointer) {
  FixedItem item;

  assert(fixed_stack_contains(stack, pointer));

  item = FIXED_STACK_GET_ITEM(stack, pointer)->next;

  if (!POINTER_IS_VALID(item)) {
    return NULL;
  }

  return FIXED_STACK_GET_ITEM_BASE(stack, item);
}