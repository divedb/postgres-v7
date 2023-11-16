add_library(fstack fstack.c)
add_library(trace trace.c)

add_library(misc INTERFACE)
target_link_libraries(misc INTERFACE fstack trace)