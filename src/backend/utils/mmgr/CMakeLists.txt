add_library(mmgr INTERFACE)

add_library(oset oset.c)
add_library(aset aset.c)

target_link_libraries(mmgr INTERFACE oset aset)