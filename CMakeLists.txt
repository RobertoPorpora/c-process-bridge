cmake_minimum_required(VERSION 3.10)

project(process_bridge)

# Add the main library
add_library(
    process_bridge STATIC 
    src/PB_generic_functions.c
    src/PB_life_management.c
    src/PB_send.c
    src/PB_receive.c
)

# Include directories
target_include_directories(process_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(process_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)