add_subdirectory(linear_algebra)
add_subdirectory(comparison)

add_library(math INTERFACE)

target_link_libraries(math INTERFACE 
        math_linear_algebra
        math_comparison
)
