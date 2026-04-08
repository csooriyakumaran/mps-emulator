# Application: mps-emulator
add_executable(mps-emulator)

# Sources
file(GLOB_RECURSE MPS_EMULATOR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
target_sources(mps-emulator PRIVATE ${MPS_EMULATOR_SOURCES})

# Include paths
target_include_directories(mps-emulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/resources.rc.in
        ${MPS_EMULATOR_GENERATED_DIR}/resources.rc
        @ONLY
    )

    target_sources(mps-emulator PRIVATE
        ${MPS_EMULATOR_GENERATED_DIR}/resources.rc
    )
endif()

# Linking
target_link_libraries(mps-emulator PRIVATE aerolib scanivalve-mps-protocol)
