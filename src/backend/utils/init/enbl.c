#include "rdbms/utils/enbl.h"

#include <assert.h>

#include "rdbms/c.h"

// BypassEnable
//  False iff enable/disable processing is required given on and "*countP."
//
// Note:
//  As a side-effect, *countP is modified.	It should be 0 initially.
//
// Exceptions:
//  BadState if called with pointer to value 0 and false.
//  BadArg if "countP" is invalid pointer.
//  BadArg if on is invalid.
bool bypass_enable(int* enable_count_in_outp, bool on) {
  assert(POINTER_IS_VALID(enable_count_in_outp));

  if (on) {
    *enable_count_in_outp += 1;

    return *enable_count_in_outp >= 2;
  }

  assert(*enable_count_in_outp >= 1);

  *enable_count_in_outp -= 1;

  return *enable_count_in_outp >= 1;
}