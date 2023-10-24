add_executable(slock_test slock_test.c)
target_link_libraries(slock_test PRIVATE storage cunit)