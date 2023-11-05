#include "rdbms/nodes/pg_list.h"

#include <CUnit/Basic.h>
#include <assert.h>
#include <stdlib.h>

#include "rdbms/utils/palloc.h"

#define ADD_TEST_MACRO(suite, description, func)         \
  do {                                                   \
    if (NULL == CU_add_test(suite, description, func)) { \
      goto CLEAN_LABEL;                                  \
    }                                                    \
  } while (0)

typedef struct Book {
  const char* name;
  const char* author;
  int pub_year;
} Book;

Book* create_book(const char* name, const char* author, int pub_year) {
  Book* book = (Book*)palloc(sizeof(*book));

  assert(book != NULL);

  book->name = pstrdup(name);
  book->author = pstrdup(author);
  book->pub_year = pub_year;

  return book;
}

void destroy_book(const Book* book) {
  assert(book != NULL);

  pfree((void*)(book->name));
  pfree((void*)(book->author));
  pfree((void*)(book));
}

void test_make_list() {
  Book* book1 = create_book("Clean Code: A Handbook of Agile Software Craftsmanship", "Robert C. Martin ", 2008);
  List* list = make_list(book1);

  CU_ASSERT(list != NULL);
  CU_ASSERT(length(list) == 1);

  destroy_book(book1);
  free_list(list);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("List", NULL, NULL);

  if (suite == NULL) {
    goto CLEAN_LABEL;
  }

  ADD_TEST_MACRO(suite, "test of make list", test_make_list);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

CLEAN_LABEL:
  CU_cleanup_registry();
  return CU_get_error();
}