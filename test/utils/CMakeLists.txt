add_executable(fstack_test fstack_test.c)
target_link_libraries(fstack_test PRIVATE utils PkgConfig::CUNIT)

add_executable(oset_test oset_test.c)
target_link_libraries(oset_test PRIVATE utils PkgConfig::CUNIT)

add_executable(aset_test aset_test.c)
target_link_libraries(aset_test PRIVATE utils PkgConfig::CUNIT)