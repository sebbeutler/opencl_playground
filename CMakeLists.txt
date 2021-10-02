cmake_minimum_required(VERSION 3.10.0)

project(OPENCL_PLAYGROUND C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_BUILD_TYPE Debug)

# binary folder after building
set(BIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${BIN_OUTPUT_DIR})

# source code at root
set(SOURCES
    ${PROJECT_SOURCE_DIR}/main.c
    ${PROJECT_SOURCE_DIR}/ui.h
    ${PROJECT_SOURCE_DIR}/ui.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

configure_file(${PROJECT_SOURCE_DIR}/main.cl ${BIN_OUTPUT_DIR}/main.cl COPYONLY)

add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/main.cl
        ${BIN_OUTPUT_DIR}/main.cl)

configure_file(${PROJECT_SOURCE_DIR}/pcg.cl ${BIN_OUTPUT_DIR}/pcg.cl COPYONLY)

add_custom_command(
    TARGET ${PROJECT_NAME}  POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/pcg.cl
        ${BIN_OUTPUT_DIR}/pcg.cl)

# includes
target_include_directories(${PROJECT_NAME} PRIVATE ./)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY})
target_link_libraries (${PROJECT_NAME} ${OpenCL_LIBRARY})

# Link the math library, needed on UNIX systems.
if (UNIX)
    target_link_libraries(${PROJECT_NAME} m)

    find_package(SDL2 REQUIRED)
    include_directories(${PROJECT_NAME} 
    ${CMAKE_CURRENT_LIST_DIR}/third_party/include)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})   

endif (UNIX)

# disable unsafe functions warnings
# add_compile_options(/w44101)
if (WIN32)
    include_directories(${PROJECT_NAME} 
    ${CMAKE_CURRENT_LIST_DIR}/third_party/include)
    target_link_libraries(${PROJECT_NAME} 
    ${CMAKE_CURRENT_LIST_DIR}/third_party/lib/x64/SDL2.lib
    ${CMAKE_CURRENT_LIST_DIR}/third_party/lib/x64/SDL2main.lib)

    set(CMAKE_C_FLAGS "/D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE ${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS "/D_CRT_NONSTDC_NO_DEPRECATE /D_SCL_SECURE_NO_DEPRECATE ${CMAKE_C_FLAGS}")
endif (WIN32)

# disable ZERO_CHECK project
set(CMAKE_SUPPRESS_REGENERATION true)