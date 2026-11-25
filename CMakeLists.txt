# File: /home/kami/ics-guard/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(ics_guard VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# RPATH configuration: ensures binaries resolve /usr/local/lib ecosystem shared libraries
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "/usr/local/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -O3
        -march=native
        -Wall
        -Wextra
        -Werror=return-type
        -fno-omit-frame-pointer
    )
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /home/kami/blackbox-sentinel/include
    /usr/local/include
)

find_package(Threads REQUIRED)
find_library(BLACKBOX_LIB blackbox REQUIRED PATHS /usr/local/lib /usr/lib)
find_library(XINFER_LIB xinfer REQUIRED PATHS /usr/local/lib /usr/lib)
find_library(SENTINELFS_LIB sentinelfs REQUIRED PATHS /usr/local/lib /usr/lib)


message(STATUS "[ics-guard] Linked Blackbox Essential: ${BLACKBOX_LIB}")
message(STATUS "[ics-guard] Linked Xinfer Essential:   ${XINFER_LIB}")

set(LIB_SOURCES
    src/core/tsc_clock.cpp
    src/protocols/modbus_tcp.cpp
    src/protocols/s7comm.cpp
    src/protocols/iec104.cpp
    src/protocols/synchrophasor.cpp
    src/defense/plant_ode.c
    src/defense/cbf_safety_gate.cpp
    src/defense/actuator_airgap.cpp
    src/pipeline/guard_engine.cpp
)

add_library(ics_guard SHARED ${LIB_SOURCES})
add_library(ics_guard_static STATIC ${LIB_SOURCES})
set_target_properties(ics_guard_static PROPERTIES OUTPUT_NAME ics_guard)

target_include_directories(ics_guard PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:/home/kami/blackbox-sentinel/include>
    $<BUILD_INTERFACE:/usr/local/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(ics_guard_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:/home/kami/blackbox-sentinel/include>
    $<BUILD_INTERFACE:/usr/local/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ics_guard PUBLIC
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
    Threads::Threads
    m
)

target_link_libraries(ics_guard_static PUBLIC
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
    Threads::Threads
    m
)

# Standalone CLI Tool
add_executable(ics-guard-ctl tools/ics_guard_ctl/main.cpp)
target_link_libraries(ics-guard-ctl PRIVATE
    ics_guard
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
)

# Extension Plugin for sentinel-stack CLI (aryo ics ...)
add_executable(aryo-ics tools/aryo_ics/main.cpp)
target_link_libraries(aryo-ics PRIVATE
    ics_guard
    ${SENTINELFS_LIB}

    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
)

# Honeytap Active Decoy Mock PLC Digital Twin
add_executable(mock_plc
    tools/mock_plc/mock_plc.c
    src/defense/plant_ode.c
)
target_include_directories(mock_plc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mock_plc PRIVATE Threads::Threads m)

# Dynamic Native Dissector Plugin for blackbox-sentinel
add_library(ics_guard_dissector_plugin SHARED
    tools/sentinel_plugin/ics_guard_plugin.cpp
)
target_link_libraries(ics_guard_dissector_plugin PRIVATE
    ics_guard
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
)
set_target_properties(ics_guard_dissector_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ics_guard_dissector_plugin"
)

# Hardware Latency Benchmark (< 120 ns)
add_executable(bench_latency benchmarks/bench_latency.cpp)
target_link_libraries(bench_latency PRIVATE
    ics_guard
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
)

# Test Suite
enable_testing()

add_executable(test_modbus tests/test_modbus_validation.cpp)
target_link_libraries(test_modbus PRIVATE ics_guard)
add_test(NAME TestModbusValidation COMMAND test_modbus)

add_executable(test_s7_iec104 tests/test_s7_iec104_validation.cpp)
target_link_libraries(test_s7_iec104 PRIVATE ics_guard)
add_test(NAME TestS7Iec104Validation COMMAND test_s7_iec104)

add_executable(test_cbf tests/test_cbf_invariance.cpp)
target_link_libraries(test_cbf PRIVATE ics_guard)
add_test(NAME TestCbfInvariance COMMAND test_cbf)

add_executable(test_plugin tests/test_sentinel_plugin.cpp)
target_link_libraries(test_plugin PRIVATE
    ics_guard
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
    ${CMAKE_DL_LIBS}
)
add_test(NAME TestSentinelPlugin COMMAND test_plugin)

add_executable(test_ics_tier1_tier2_pipeline tests/test_ics_tier1_tier2_pipeline.cpp)
target_link_libraries(test_ics_tier1_tier2_pipeline PRIVATE
    ics_guard
    -Wl,--no-as-needed
    ${BLACKBOX_LIB}
    ${XINFER_LIB}
    -Wl,--as-needed
    ${CMAKE_DL_LIBS}
)
add_test(NAME TestIcsTier1Tier2Pipeline COMMAND test_ics_tier1_tier2_pipeline)

# Installation Rules
install(TARGETS ics_guard ics_guard_static ics-guard-ctl aryo-ics mock_plc ics_guard_dissector_plugin
    EXPORT IcsGuardTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS ics_guard_dissector_plugin
    DESTINATION /usr/local/lib/blackbox
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/IcsGuard)
configure_package_config_file(
    cmake/IcsGuardConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/IcsGuardConfig.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/IcsGuardConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/IcsGuardConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/IcsGuardConfigVersion.cmake
    DESTINATION ${CONFIG_INSTALL_DIR}
)
install(EXPORT IcsGuardTargets
    FILE IcsGuardTargets.cmake
    NAMESPACE IcsGuard::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

configure_file(
    cmake/ics_guard.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/ics_guard.pc
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ics_guard.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
