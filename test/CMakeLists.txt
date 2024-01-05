function(add_one_test target)
    add_executable(${target} "${target}.c")
    target_link_libraries(${target} PRIVATE postgres PkgConfig::CUNIT pthread)
endfunction()

function (add_tests)
    foreach(test ${ARGV})
        add_one_test(${test})
    endforeach()
endfunction()

include_directories(${CUNIT_INCLUDE_DIRS})

add_subdirectory(storage)
# add_subdirectory(utils)
# add_subdirectory(nodes)
