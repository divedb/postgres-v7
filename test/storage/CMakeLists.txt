function(add_storage_test)
    add_executable(${ARGV})
    target_link_libraries(${ARGV0} PRIVATE storage PkgConfig::CUNIT pthread)
endfunction()

add_storage_test(slock_test slock_test.c)
add_storage_test(ipc_test ipc_test.c)