add_subdirectory(buffer)
add_subdirectory(ipc)

add_library(storage INTERFACE)
target_link_libraries(storage INTERFACE buffer ipc)