function(add_utils_test)
    add_executable(${ARGV})
    target_link_libraries(${ARGV0} PRIVATE utils PkgConfig::CUNIT pthread)
endfunction()

add_utils_test(fstack_test fstack_test.c)
add_utils_test(oset_test oset_test.c)
add_utils_test(aset_test aset_test.c)
add_utils_test(trace_test trace_test.c)