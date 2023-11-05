add_library(mmgr INTERFACE)

add_library(oset oset.c)
add_library(aset aset.c)
add_library(palloc palloc.c)

target_link_libraries(mmgr INTERFACE oset aset palloc)