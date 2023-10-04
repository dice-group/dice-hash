include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

function(install_interface_library LIBRARY_NAME INCLUDE_PATH)

    target_include_directories(
            ${LIBRARY_NAME} INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    set_property(TARGET ${LIBRARY_NAME} PROPERTY EXPORT_NAME ${LIBRARY_NAME})

    install(
            TARGETS ${LIBRARY_NAME}
            EXPORT ${LIBRARY_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


    write_basic_package_version_file(
            "${LIBRARY_NAME}-config-version.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion)

    configure_package_config_file(
            "${PROJECT_SOURCE_DIR}/cmake/dummy-config.cmake.in"
            "${PROJECT_BINARY_DIR}/${LIBRARY_NAME}-config.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(
            EXPORT ${LIBRARY_NAME}-targets
            FILE ${LIBRARY_NAME}-targets.cmake
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(FILES "${PROJECT_BINARY_DIR}/${LIBRARY_NAME}-config.cmake"
            "${PROJECT_BINARY_DIR}/${LIBRARY_NAME}-config-version.cmake"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/${INCLUDE_PATH}/ DESTINATION include)
endfunction()