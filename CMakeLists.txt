cmake_minimum_required(VERSION 3.28.3)

project(creolynator VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(SOURCES
    src/utils.cpp
    src/error.cpp
    src/b_lexer.cpp
    src/i_lexer.cpp
    src/migr.cpp
    src/migr_structural.cpp
    src/migr_semantic.cpp
    src/main.cpp
)

set(HEADERS
    include/globals.h
    include/utils.h
    include/error.h
    include/b_lexer.h
    include/i_lexer.h
    include/migr.h
    include/migr_structural.h
    include/migr_semantic.h
    include/serialization_engine.h
)

add_library(RapidJSON INTERFACE)
target_include_directories(RapidJSON INTERFACE
    ${CMAKE_SOURCE_DIR}/external/rapidjson
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>:-O3 -Wall>
)

# header only rapidjson library
target_link_libraries(${PROJECT_NAME} PRIVATE RapidJSON)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin"
)

install(TARGETS ${PROJECT_NAME} DESTINATION build/bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using GCC or Clang compiler")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using MSVC compiler")
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
