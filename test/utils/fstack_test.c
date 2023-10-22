#include "rdbms/utils/fstack.h"

#include <CUnit/Basic.h>
#include <assert.h>
#include <stdlib.h>

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      CU_cleanup_registry();                             \
      return CU_get_error();                             \
    }                                                    \
  } while (0)

typedef struct Book {
  const char* name;
  const char* author;
  int pub_year;
  struct FixedItemData dummy;
} Book;

Book* new_book(const char* name, const char* author, int pub_year) {
  Book* book = (Book*)malloc(sizeof(*book));

  assert(book != NULL);

  book->name = name;
  book->author = author;
  book->pub_year = pub_year;

  return book;
}

void test_empty_stack() {
  FixedStack stack = (FixedStack)malloc(sizeof(*stack));

  assert(stack != NULL);

  CU_ASSERT(fixed_stack_pop(stack) == NULL);
  CU_ASSERT(fixed_stack_get_top(stack) == NULL);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("FixedStack", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  ADD_TEST_MACRO(suite, "test of empty stack", test_empty_stack);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}