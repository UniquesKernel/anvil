cmake_minimum_required(VERSION 3.22)

project(project VERSION 0.1.0 
  LANGUAGES ASM C
  DESCRIPTION "A template project for C libraries"
)

option(BUILD_TESTING "Build Tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerStandards)

enable_testing()

find_package(Python3 COMPONENT Interpreter REQUIRED)
message(STATUS "Using Python 3 Interpreter: ${Python3_EXECUTABLE}")

# add_subdirectory(libs/<lib_name>)

if(NOT DEFINED ENV{VIRTUAL_ENV})
    message(FATAL_ERROR "Please activate your virtual environment before running CMake. Pytest and Hypothesis are required for testing. Use 'source venv/bin/activate' to activate the virtual environment.")
endif()

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
