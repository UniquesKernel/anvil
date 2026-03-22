add_subdirectory(linear_algebra)
add_subdirectory(comparison)

add_library(math STATIC
    $<TARGET_OBJECTS:math_linear_algebra>
    $<TARGET_OBJECTS:math_comparison>
)

target_include_directories(math
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
