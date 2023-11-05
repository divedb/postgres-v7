function(add_nodes_test)
    add_executable(${ARGV})
    target_link_libraries(${ARGV0} PRIVATE nodes PkgConfig::CUNIT pthread)
endfunction()

add_nodes_test(pg_list_test pg_list_test.c)