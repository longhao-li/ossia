cmake_minimum_required(VERSION 3.16)

project(ossia VERSION 1.0.0 LANGUAGES CXX)

# Disable in-source build.
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source build is not allowed.")
endif()

option(OSSIA_BUILD_SHARED_LIBS "Build ossia runtime as shared libraries" OFF)
option(OSSIA_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(OSSIA_ENABLE_LTO "Enable link-time optimization." OFF)
option(OSSIA_BUILD_TESTS "Build unit tests." OFF)

# Build ossia runtime.
file(GLOB_RECURSE OSSIA_HEADER_FILES "include/*.hpp")
file(GLOB_RECURSE OSSIA_SOURCE_FILES "src/*.cpp")

if(OSSIA_BUILD_SHARED_LIBS)
    add_library(ossia SHARED ${OSSIA_HEADER_FILES} ${OSSIA_SOURCE_FILES})

    # Options for shared library.
    if(WIN32)
        target_compile_definitions(ossia PRIVATE "OSSIA_API=__declspec(dllexport)")
        target_compile_definitions(ossia INTERFACE "OSSIA_API=__declspec(dllimport)")
    else()
        target_compile_definitions(ossia PUBLIC "OSSIA_API=__attribute__((visibility(\"default\")))")
    endif()

    set_target_properties(
        ossia
        PROPERTIES  POSITION_INDEPENDENT_CODE ON
                    CXX_VISIBILITY_PRESET hidden
                    VISIBILITY_INLINES_HIDDEN ON
    )

    # Enable LTO if supported.
    if(OSSIA_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT OSSIA_LTO_SUPPORTED OUTPUT OSSIA_LTO_MESSAGE)

        if(OSSIA_LTO_SUPPORTED)
            set_target_properties(ossia PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
            message(STATUS "LTO is enabled for ossia runtime.")
        else()
            message(NOTICE "LTO is not supported: ${OSSIA_LTO_MESSAGE}")
        endif()
    endif()
else()
    if(OSSIA_ENABLE_LTO)
        message(WARNING "LTO is not supported for static libraries.")
    endif()

    add_library(ossia STATIC ${OSSIA_HEADER_FILES} ${OSSIA_SOURCE_FILES})
    target_compile_definitions(ossia PUBLIC "OSSIA_API=")
endif()

# Alias target to be consistent with the package name.
add_library(ossia::ossia ALIAS ossia)

# Enable C++20 for coroutine support.
target_compile_features(ossia PUBLIC cxx_std_23)

# Include directories.
target_include_directories(
    ossia
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
)

# Compiler options.
if(MSVC)
    target_compile_options(ossia PRIVATE /W4 /permissive- /wd4324)
    if(OSSIA_WARNINGS_AS_ERRORS)
        target_compile_options(ossia PRIVATE /WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(ossia PRIVATE -Wall -Wextra -Wshadow -Woverloaded-virtual -Wold-style-cast)
    if(OSSIA_WARNINGS_AS_ERRORS)
        target_compile_options(ossia PRIVATE -Werror)
    endif()
endif()

# Link system libraries.
if(WIN32)
    # Link Windows socket library.
    target_link_libraries(ossia PRIVATE ws2_32)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Link liburing. We assume that liburing is a system library.
    target_link_libraries(ossia PRIVATE uring)
endif()

# Configure the package.
include(GNUInstallDirs)

install(
    TARGETS     ossia
    EXPORT      ossia-targets
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT      ossia-targets
    FILE        ossia-targets.cmake
    NAMESPACE   ossia::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ossia
)

# Generate the package configuration file.
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ossia-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ossia-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ossia
)

write_basic_package_version_file(
    ossia-config-version.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES       ${CMAKE_CURRENT_BINARY_DIR}/ossia-config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/ossia-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ossia
)

# Install the header files.
install(
    DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/include/ossia
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Build tests.
if(OSSIA_BUILD_TESTS)
    # Enable testing support in CMake.
    include(CTest)

    # Fetch doctest.
    include(FetchContent)

    # CMake policy CMP0135 controls extraction behavior of the FetchContent module.
    # This policy was introduced in CMake 3.21. We set the policy to NEW to avoid
    # unnecessary downloads of the same content.
    if(POLICY CMP0135)
    	cmake_policy(SET CMP0135 NEW)
    	set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
    endif()

    message(STATUS "Fetching doctest from https://github.com/doctest/doctest")

    set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "")
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
    FetchContent_Declare(
        doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.zip
        URL_HASH SHA256=6745E17682D6E2BE1EA31EC8BF1814A3D7CB17D61D120E5F2EE5A075926634AD
    )

    FetchContent_MakeAvailable(doctest)

    # Build the test executable.
    file(GLOB_RECURSE OSSIA_TEST_FILES "tests/*.cpp")
    add_executable(ossia-test ${OSSIA_TEST_FILES})

    target_link_libraries(ossia-test PRIVATE ossia doctest::doctest)

    # Configure CMake tests
    include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
    doctest_discover_tests(ossia-test)
endif()
