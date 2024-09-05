cmake_minimum_required(VERSION 3.10)

project(test_process_bridge)

add_subdirectory("${CMAKE_SOURCE_DIR}/.." PB_build)

add_executable(${PROJECT_NAME}_test test.c)
add_executable(${PROJECT_NAME}_child child.c)

set_target_properties(${PROJECT_NAME}_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
set_target_properties(${PROJECT_NAME}_child PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(${PROJECT_NAME}_test process_bridge)
target_link_libraries(${PROJECT_NAME}_child process_bridge)
