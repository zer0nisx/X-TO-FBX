cmake_minimum_required(VERSION 3.10)
project(bzip2 VERSION 1.0.5 LANGUAGES C)

# Source files for the bzip2 library (excluding command-line tools)
set(BZIP2_SOURCES
    src/blocksort.c
    src/huffman.c
    src/crctable.c
    src/randtable.c
    src/compress.c
    src/decompress.c
    src/bzlib.c
)

# Create static library
add_library(bzip2_static STATIC ${BZIP2_SOURCES})

# Set library properties
set_target_properties(bzip2_static PROPERTIES
    OUTPUT_NAME bz2
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(bzip2_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler flags
if(WIN32)
    # Windows specific flags
    target_compile_definitions(bzip2_static PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(bzip2_static PRIVATE /W3)
    endif()
else()
    # Unix-like systems
    target_compile_options(bzip2_static PRIVATE -Wall -Wno-unused-function)
endif()

# Create alias for easier linking
add_library(BZip2::BZip2 ALIAS bzip2_static)

# Installation rules (optional)
install(TARGETS bzip2_static
    EXPORT bzip2Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES include/bzlib.h
    DESTINATION include
)

install(EXPORT bzip2Targets
    FILE bzip2Targets.cmake
    NAMESPACE BZip2::
    DESTINATION lib/cmake/bzip2
)

# Print configuration
message(STATUS "BZip2 library configuration:")
message(STATUS "  Source files: ${BZIP2_SOURCES}")
message(STATUS "  Output name: libbz2")
message(STATUS "  C standard: C99")
