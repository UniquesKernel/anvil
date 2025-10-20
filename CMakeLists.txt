cmake_minimum_required(VERSION 3.22)

project(Anvil VERSION 0.1.0 
  LANGUAGES ASM CXX
  DESCRIPTION "A CPP libary collection"
)

option(BUILD_TESTING "Build Tests" ON)
option( "check of memory leaks" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerStandards)

enable_testing()

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Using Python 3 Interpreter: ${Python3_EXECUTABLE}")
message(STATUS "Python3 Include Dirs: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3 Libraries: ${Python3_LIBRARIES}")

if(BUILD_TESTING)
    include(CTest)
    
    if (MEMCHECK)
      find_program(VALGRIND_EXECUTABLE valgrind)
      if(VALGRIND_EXECUTABLE)
        set(MEMORYCHECK_COMMAND ${VALGRIND_EXECUTABLE})
        set(MEMORYCHECK_COMMAND_OPTIONS 
          "--leak-check=full"
          "--show-leak-kinds=definite,possible"
          "--track-origins=yes"
          "--error-exitcode=1"
          "--suppressions=${CMAKE_SOURCE_DIR}/valgrind-python.supp")
        message(STATUS "Found Valgrind: ${VALGRIND_EXECUTABLE}")
      endif()
    endif()
endif()

add_subdirectory(libs/memory)