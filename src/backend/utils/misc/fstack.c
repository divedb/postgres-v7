#include "rdbms/utils/fstack.h"

#include <assert.h>
#include <stdbool.h>

#define FixedItemIsValid(item)             PointerIsValid(item)
#define FixedStackGetItemBase(stack, item) ((Pointer)((char*)(item) - (stack)->offset))
#define FixedStackGetItem(stack, pointer)  ((FixedItem)((char*)(pointer) + (stack)->offset))
#define FixedStackIsValid(stack)           ((bool)PointerIsValid(stack))

void fixed_stack_init(FixedStack stack, Offset offset) {
  assert(PointerIsValid(stack));

  stack->top = NULL;
  stack->offset = offset;
}

Pointer fixed_stack_pop(FixedStack stack) {
  Pointer pointer;

  assert(FixedStackIsValid(stack));

  if (!PointerIsValid(stack->top)) {
    return NULL;
  }

  pointer = FixedStackGetItemBase(stack, stack->top);
  stack->top = stack->top->next;

  return pointer;
}

void fixed_stack_push(FixedStack stack, Pointer pointer) {
  FixedItem item = FixedStackGetItem(stack, pointer);

  assert(FixedStackIsValid(stack));
  assert(PointerIsValid(pointer));

  item->next = stack->top;
  stack->top = item;
}

Pointer fixed_stack_get_top(FixedStack stack) {
  assert(FixedStackIsValid(stack));

  if (!PointerIsValid(stack->top)) {
    return NULL;
  }

  return FixedStackGetItemBase(stack, stack->top);
}

static bool fixed_stack_contains(FixedStack stack, Pointer pointer) {
  FixedItem next;
  FixedItem item;

  assert(FixedStackIsValid(stack));
  assert(PointerIsValid(pointer));

  item = FixedStackGetItem(stack, pointer);

  for (next = stack->top; FixedItemIsValid(next); next = next->next) {
    if (next == item) {
      return true;
    }
  }

  return false;
}

Pointer fixed_stack_get_next(FixedStack stack, Pointer pointer) {
  FixedItem item;

  assert(fixed_stack_contains(stack, pointer));

  item = FixedStackGetItem(stack, pointer)->next;

  if (!PointerIsValid(item)) {
    return NULL;
  }

  return FixedStackGetItemBase(stack, item);
}