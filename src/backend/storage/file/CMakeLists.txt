add_library(fd fd.c)
add_library(file INTERFACE)
target_link_libraries(file INTERFACE fd)