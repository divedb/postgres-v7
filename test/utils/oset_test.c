#include "rdbms/utils/oset.h"

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

typedef struct Person {
  int age;
  const char* name;
  OrderedElemData dummy;
} Person;

Person* create_person(const char* name, int age) {
  Person* person = (Person*)malloc(sizeof(*person));

  assert(person != NULL);

  person->name = name;
  person->age = age;

  return person;
}

void destroy_person(Person* person) { free(person); }

void test_init() {
  OrderedSetData set_data;
  Offset offset = offsetof(Person, dummy);

  ordered_set_init(&set_data, offset);
}

void test_basic_push_and_pop() {
  OrderedSetData set_data;
  OrderedSet oset = &set_data;
  Offset offset = offsetof(Person, dummy);

  ordered_set_init(oset, offset);

  // Empty order set.
  CU_ASSERT(ordered_set_get_head(oset) == NULL);

  Person* person1 = create_person("A", 18);
  Person* person2 = create_person("B", 20);

  ordered_elem_push_into(&(person1->dummy), oset);
  CU_ASSERT(ordered_set_get_head(oset) == (Pointer)person1);

  ordered_elem_push_into(&(person2->dummy), oset);
  CU_ASSERT(ordered_set_get_head(oset) == (Pointer)person2);

  ordered_elem_pop(&(person2->dummy));
  CU_ASSERT(ordered_set_get_head(oset) == (Pointer)person1);

  ordered_elem_pop(&(person1->dummy));
  CU_ASSERT(ordered_set_get_head(oset) == NULL);

  destroy_person(person2);
  destroy_person(person1);
}

void test_predecessor_and_successor() {
  OrderedSetData set_data;
  OrderedSet oset = &set_data;
  Offset offset = offsetof(Person, dummy);

  ordered_set_init(oset, offset);

  // Empty order set.
  CU_ASSERT(ordered_set_get_head(oset) == NULL);

  Person* person1 = create_person("A", 18);
  Person* person2 = create_person("B", 20);

  ordered_elem_push_into(&(person1->dummy), oset);
  CU_ASSERT(ordered_elem_get_predecessor(&(person1->dummy)) == NULL);
  CU_ASSERT(ordered_elem_get_successor(&(person1->dummy)) == NULL);

  ordered_elem_push_into(&(person2->dummy), oset);
  CU_ASSERT(ordered_elem_get_predecessor(&(person1->dummy)) == (Pointer)person2);
  CU_ASSERT(ordered_elem_get_successor(&(person1->dummy)) == NULL);
  CU_ASSERT(ordered_elem_get_predecessor(&(person2->dummy)) == NULL);
  CU_ASSERT(ordered_elem_get_successor(&(person2->dummy)) == (Pointer)person1);

  destroy_person(person2);
  destroy_person(person1);
}

int main() {
  CU_pSuite suite = NULL;

  if (CUE_SUCCESS != CU_initialize_registry()) {
    return CU_get_error();
  }

  suite = CU_add_suite("OrderedSet", NULL, NULL);

  if (suite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  ADD_TEST_MACRO(suite, "test of init ordered set", test_init);
  ADD_TEST_MACRO(suite, "test of basic push and pop", test_basic_push_and_pop);
  ADD_TEST_MACRO(suite, "test predecessor and successor", test_predecessor_and_successor);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}