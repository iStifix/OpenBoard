# Find libevdev
#
find_package(PkgConfig REQUIRED)
pkg_check_modules(LibEvdev IMPORTED_TARGET libevdev)

if(LibEvdev_FOUND)
    target_link_libraries(${PROJECT_NAME}
        PkgConfig::LibEvdev
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIBEVDEV)
endif()
