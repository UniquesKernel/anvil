include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(anvil STATIC
    $<TARGET_OBJECTS:math_linear_algebra>
    $<TARGET_OBJECTS:math_comparison>
    $<TARGET_OBJECTS:memory>
    $<TARGET_OBJECTS:container>
    $<TARGET_OBJECTS:graphic>
)

target_include_directories(anvil
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS anvil
    EXPORT anvilTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT anvilTargets
    FILE anvilTargets.cmake
    NAMESPACE anvil::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anvil
)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/anvilConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/anvilConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anvil
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/anvilConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anvil
)
