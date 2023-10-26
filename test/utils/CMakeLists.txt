add_executable(fstack_test fstack_test.c)
target_link_libraries(fstack_test PRIVATE utils cunit)

add_executable(oset_test oset_test.c)
target_link_libraries(oset_test PRIVATE utils cunit)