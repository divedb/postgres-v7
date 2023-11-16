add_library(enbl enbl.c)
add_library(globals globals.c)

add_library(init INTERFACE)
target_link_libraries(init INTERFACE enbl globals)