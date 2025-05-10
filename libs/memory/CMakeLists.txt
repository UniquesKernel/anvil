set(MODULE_NAME memory)

set(MODULE_SOURCE 
    src/utility.c
    src/memory_allocation.c
)

set(MODULE_PUBLIC_HEADER 
)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_include_directories(${MODULE_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if(BUILD_TESTING)

    add_library(${MODULE_NAME}_test_shared SHARED ${MODULE_SOURCE})

    target_include_directories(${MODULE_NAME}_test_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src>
    )

    set_target_properties(${MODULE_NAME}_test_shared PROPERTIES
        C_VISIBILITY_PRESET default
        VISIBILITY_INLINES_HIDDEN OFF
    )

    target_compile_options(${MODULE_NAME}_test_shared PRIVATE -fPIC -O0 -g)

    message("BINARY DIR: ${CMAKE_CURRENT_BINARY_DIR}")
    message("SOURCE DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(
        NAME ${MODULE_NAME}_pytest
        COMMAND ${Python3_EXECUTABLE} -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties(${MODULE_NAME}_pytest PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR};PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

install(TARGETS ${MODULE_NAME}
    EXPORT ${PROJECT_NAME}-${MODULE_NAME}-targets # Make it part of an export set
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    # PUBLIC_HEADER DESTINATION include/${MODULE_NAME} # This is a more modern way
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/${MODULE_NAME} # Install this module's headers into a namespaced dir
    FILES_MATCHING PATTERN "*.h"
)
