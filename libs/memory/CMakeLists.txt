# =================== Variables ==========================

set(MODULE_NAME memory)
set(MODULE_SOURCE 
    src/error.cpp
    src/memory_allocation.cpp
    src/scratch_allocator.cpp
    src/stack_allocator.cpp
    src/utility.cpp
)
set(BENCHMARK_MODULE_SOURCE
    ${MODULE_SOURCE}
    benchmarking/scratch_allocator_benchmark.cpp
)

# ================== Build Targets ========================

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE}) # Release Target
target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

add_executable(${MODULE_NAME}_benchmark ${BENCHMARK_MODULE_SOURCE}) # Benchmark executable
target_include_directories(${MODULE_NAME}_benchmark  PRIVATE ${CMAKE_SOURCE_DIR}/include)
set(BENCHMARK_OUTPUT_DIR ${CMAKE_BINARY_DIR}/benchmarks)
file(MAKE_DIRECTORY ${BENCHMARK_OUTPUT_DIR})
set_target_properties(${MODULE_NAME}_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BENCHMARK_OUTPUT_DIR}
)
add_test(NAME ${MODULE_NAME}_benchmark_run
         COMMAND ${MODULE_NAME}_benchmark --runs 100 --iters 20000 --strict)

# =================== Set Compiler Options ===================

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)
set_compiler_options(${MODULE_NAME})
set_compiler_options(${MODULE_NAME}_benchmark)
target_compile_options(memory_benchmark PRIVATE -Wno-old-style-cast -Wno-shadow -Wno-unused-result)

if(BUILD_TESTING)
    # Find Python with Development component (required for pybind11)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    
    # Find pybind11
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE pybind11_FIND_RESULT
    )
    
    if(pybind11_FIND_RESULT EQUAL 0)
        message(STATUS "Found pybind11 at: ${pybind11_DIR}")
        list(APPEND CMAKE_PREFIX_PATH ${pybind11_DIR})
        find_package(pybind11 CONFIG REQUIRED)
    else()
        message(FATAL_ERROR "pybind11 not found. Install with: pip install pybind11")
    endif()

    # Create Python module using pybind11
    pybind11_add_module(anvil_memory 
        bindings/memory_bindings.cpp 
        ${MODULE_SOURCE}
    )
    
    target_include_directories(anvil_memory
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # Link against Python libraries explicitly
    target_link_libraries(anvil_memory PRIVATE Python3::Python)
    
    target_compile_options(anvil_memory PRIVATE -O0 -g -mavx2)

    # Copy the module to the test directory for easy importing
    add_custom_command(TARGET anvil_memory POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:anvil_memory>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/
        COMMENT "Copying anvil_memory module to tests directory"
    )

    if (MEMCHECK)
        message("${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS}")
        add_test(
        NAME ${MODULE_NAME}_pytest
        COMMAND ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} ${Python3_EXECUTABLE} -m pytest --hypothesis-show-statistics "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
        )
    else()
        add_test(
            NAME ${MODULE_NAME}_pytest
            COMMAND ${Python3_EXECUTABLE} -m pytest -s -v --hypothesis-show-statistics "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
        )
        set_tests_properties(${MODULE_NAME}_pytest PROPERTIES
            ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${CMAKE_SOURCE_DIR}/libs"
        )
    endif()


endif()

install(TARGETS ${MODULE_NAME}
    EXPORT ${PROJECT_NAME}-${MODULE_NAME}-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/memory
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)



