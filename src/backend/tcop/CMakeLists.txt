add_library(pg postgres.c)

add_library(tcop INTERFACE)
target_link_libraries(tcop INTERFACE pg)