add_subdirectory(storage)
add_subdirectory(utils)
# add_subdirectory(nodes)

add_library(postgres INTERFACE)
target_link_libraries(postgres INTERFACE storage)