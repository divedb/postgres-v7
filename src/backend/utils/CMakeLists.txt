add_subdirectory(misc)
add_subdirectory(mmgr)
add_subdirectory(error)
add_subdirectory(adt)

add_library(utils INTERFACE)
target_link_libraries(utils INTERFACE misc mmgr error)