add_library(md md.c)
add_library(smgr INTERFACE)
target_link_libraries(smgr INTERFACE md)