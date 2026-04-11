cmake_minimum_required(VERSION 3.14)

# Project configuration ---
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_STRING)
string(STRIP "${PROJECT_VERSION_STRING}" PROJECT_VERSION_STRING)

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?" PROJECT_VERSION_NUMERIC "${PROJECT_VERSION_STRING}")

if (NOT PROJECT_VERSION_NUMERIC)
    message(FATAL_ERROR "Invalid VERSION file format: '${PROJECT_VERSION_STRING}'")
endif()

project(mps-emulator
    VERSION ${PROJECT_VERSION_NUMERIC}
    LANGUAGES CXX
)

if (WIN32)
    enable_language(RC)
endif()

# === C++ Standard, compile flags, configs ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# for multi-config generators (Visual Studio, Xcode)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:RELEASE>
    _USE_MATH_DEFINES
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
    _WINSOCK_DEPRECATED_NO_WARNINGS
)

if (WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
endif()

# === Generated headers / resources ===
set(MPS_EMULATOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${MPS_EMULATOR_GENERATED_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${MPS_EMULATOR_GENERATED_DIR}/version.h
)

# === Dependencies ===
include(FetchContent)

FetchContent_Declare(
    scanivalve-mps-protocol
    GIT_REPOSITORY https://github.com/csooriyakumaran/scanivalve-mps-protocol.git
    GIT_TAG v0.1.4
)
FetchContent_MakeAvailable(scanivalve-mps-protocol)

# === Subdirectories ===
add_subdirectory(aerolib)
add_subdirectory(application)

target_include_directories(aerolib PUBLIC ${MPS_EMULATOR_GENERATED_DIR})
target_include_directories(mps-emulator PRIVATE ${MPS_EMULATOR_GENERATED_DIR})

if (WIN32)
    target_link_libraries(aerolib PUBLIC ws2_32)
    target_link_libraries(mps-emulator PRIVATE ws2_32)
endif()

# === Install Rules ===
install(TARGETS mps-emulator
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

install(FILES README.md LICENSE
    DESTINATION .
    COMPONENT Docs
)





