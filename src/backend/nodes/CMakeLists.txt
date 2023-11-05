add_library(nodes list.c equalfuncs.c nodes.c)
target_link_libraries(nodes utils adt)