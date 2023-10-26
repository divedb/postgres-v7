add_library(mmgr INTERFACE)

add_library(oset oset.c)
target_link_libraries(mmgr INTERFACE oset)