#define MEMORY_CONTEXT_CHECKING
#define HAVE_ALLOC_INFO

#include "../template.h"
#include "rdbms/utils/memutils.h"

static void test_alloc_and_free() {
  memory_context_init();

  int sizes[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6};
  void* pointers[LENGTH_OF(sizes)];

  for (int i = 0; i < LENGTH_OF(sizes); i++) {
    pointers[i] = palloc(sizes[i]);
  }

  memory_context_stats(TopMemoryContext);

  for (int i = 0; i < LENGTH_OF(sizes); i++) {
    pfree(pointers[i]);
  }
}

static void register_test() {
  TEST("Test alloc and free.", test_alloc_and_free);
  // TEST("Test alloc and free.", test_alloc_and_free);
}

MAIN("Memory Context")
