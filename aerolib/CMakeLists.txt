# === library: aerolib ===
add_library(aerolib STATIC)

# Sources
file(GLOB_RECURSE AEROLIB_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
target_sources(aerolib PRIVATE ${AEROLIB_SOURCES})

# Public includes
target_include_directories(aerolib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)


