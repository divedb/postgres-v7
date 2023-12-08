add_library(mmgr INTERFACE)

add_library(aset aset.c)
add_library(palloc palloc.c)
add_library(mctx mctx.c)

target_link_libraries(mmgr INTERFACE aset palloc mctx)