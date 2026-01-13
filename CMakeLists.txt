cmake_minimum_required(VERSION 3.16)
project(bitspan VERSION 0.1.1 DESCRIPTION "Read and write bit fields of any width at any bit offset, little or big endian" LANGUAGES C)

option(BITSPAN_BUILD_TESTS "Build tests" OFF)
option(BITSPAN_BUILD_EXAMPLES "Build examples" OFF)
option(BITSPAN_BUILD_FUZZ "Build libFuzzer targets" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(bitspan src/bitspan.c)
add_library(bitspan::bitspan ALIAS bitspan)
target_include_directories(bitspan PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(bitspan PROPERTIES
    C_STANDARD 99
    C_VISIBILITY_PRESET hidden
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

set(PC_CFLAGS "")
if(BUILD_SHARED_LIBS)
    target_compile_definitions(bitspan PRIVATE BITF_BUILD_SHARED INTERFACE BITF_SHARED)
    set(PC_CFLAGS "-DBITF_SHARED")
endif()

install(TARGETS bitspan EXPORT bitspanTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT bitspanTargets
    NAMESPACE bitspan::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitspan)

configure_package_config_file(cmake/bitspanConfig.cmake.in
    ${PROJECT_BINARY_DIR}/bitspanConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitspan)
write_basic_package_version_file(${PROJECT_BINARY_DIR}/bitspanConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${PROJECT_BINARY_DIR}/bitspanConfig.cmake
    ${PROJECT_BINARY_DIR}/bitspanConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitspan)

configure_file(bitspan.pc.in ${PROJECT_BINARY_DIR}/bitspan.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/bitspan.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(BITSPAN_BUILD_TESTS)
    enable_testing()
    add_executable(test_bitspan tests/test_bitspan.c)
    target_link_libraries(test_bitspan PRIVATE bitspan)
    add_test(NAME bitspan COMMAND test_bitspan)
    add_library(bitspan_header_check OBJECT tests/header_check.c)
    target_include_directories(bitspan_header_check PRIVATE include)
endif()

if(BITSPAN_BUILD_EXAMPLES)
    add_executable(bitspan_basic examples/basic.c)
    target_link_libraries(bitspan_basic PRIVATE bitspan)
endif()

if(BITSPAN_BUILD_FUZZ)
    add_executable(fuzz_roundtrip fuzz/fuzz_roundtrip.c)
    target_link_libraries(fuzz_roundtrip PRIVATE bitspan)
    target_link_options(fuzz_roundtrip PRIVATE -fsanitize=fuzzer)
endif()
