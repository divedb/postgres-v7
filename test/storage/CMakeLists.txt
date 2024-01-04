function(add_storage_test target)
    add_executable(${target} "${target}.c")
    target_link_libraries(${target} PRIVATE postgres PkgConfig::CUNIT pthread)
endfunction()

add_storage_test(ipc_test fd_test)