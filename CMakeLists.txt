cmake_minimum_required(VERSION 3.20)
project(X2FBXConverter VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# FBX SDK Configuration - Using the path you provided
set(FBX_SDK_ROOT "C:/Program Files/Autodesk/FBX/FBX SDK/2020.3.7" CACHE PATH "Path to FBX SDK")
set(FBX_INCLUDE_DIR "${FBX_SDK_ROOT}/include")
set(FBX_LIBRARY_DIR "${FBX_SDK_ROOT}/lib/x64/release")

# Verify FBX SDK paths
if(EXISTS "${FBX_INCLUDE_DIR}" AND EXISTS "${FBX_LIBRARY_DIR}")
    include_directories(${FBX_INCLUDE_DIR})
    link_directories(${FBX_LIBRARY_DIR})

    if(WIN32)
        set(FBX_LIBRARIES "libfbxsdk.lib")
    else()
        set(FBX_LIBRARIES "libfbxsdk.a")
    endif()

    # Define the FBXSDK_FOUND macro for conditional compilation
    add_definitions(-DFBXSDK_FOUND)
    set(FBXSDK_FOUND TRUE)

    message(STATUS "FBX SDK found at: ${FBX_SDK_ROOT}")
    message(STATUS "FBX Include Dir: ${FBX_INCLUDE_DIR}")
    message(STATUS "FBX Library Dir: ${FBX_LIBRARY_DIR}")
    message(STATUS "FBXSDK_FOUND macro defined - FBX functionality enabled")
else()
    message(WARNING "FBX SDK not found at: ${FBX_SDK_ROOT}")
    message(WARNING "Building without FBX SDK - placeholder files will be generated")
    set(FBXSDK_FOUND FALSE)
endif()

# Zlib SDK Configuration
set(ZLIB_SDK_ROOT "${CMAKE_SOURCE_DIR}/third_party/zlib" CACHE PATH "Path to zlib SDK")
set(ZLIB_INCLUDE_DIR "${ZLIB_SDK_ROOT}/include")

# Platform and configuration specific library paths
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ZLIB_LIBRARY_DIR "${ZLIB_SDK_ROOT}/lib/x64")
    else()
        set(ZLIB_LIBRARY_DIR "${ZLIB_SDK_ROOT}/lib/x86")
    endif()

    # Different library names to try
    set(ZLIB_LIBRARY_NAMES "zlib.lib" "zlibstatic.lib" "zlib1.lib")
else()
    set(ZLIB_LIBRARY_DIR "${ZLIB_SDK_ROOT}/lib")
    set(ZLIB_LIBRARY_NAMES "libz.a" "libzlib.a")
endif()

# Check for zlib headers
if(EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
    include_directories(${ZLIB_INCLUDE_DIR})
    add_definitions(-DHAVE_ZLIB)

    # Find the library file
    set(ZLIB_LIBRARY)
    foreach(config_dir "Release" "Debug" ".")
        if(NOT ZLIB_LIBRARY)
            foreach(lib_name ${ZLIB_LIBRARY_NAMES})
                set(lib_path "${ZLIB_LIBRARY_DIR}/${config_dir}/${lib_name}")
                if(EXISTS "${lib_path}")
                    set(ZLIB_LIBRARY "${lib_path}")
                    break()
                endif()
            endforeach()
        endif()
    endforeach()

    if(ZLIB_LIBRARY)
        message(STATUS "Found zlib SDK at: ${ZLIB_SDK_ROOT}")
        message(STATUS "zlib Include Dir: ${ZLIB_INCLUDE_DIR}")
        message(STATUS "zlib Library: ${ZLIB_LIBRARY}")
        set(ZLIB_FOUND TRUE)
    else()
        message(FATAL_ERROR "zlib library not found in ${ZLIB_LIBRARY_DIR}. Please place zlib library files in the SDK directory.")
    endif()
else()
    message(FATAL_ERROR "zlib headers not found at ${ZLIB_INCLUDE_DIR}. Please extract zlib SDK to ${ZLIB_SDK_ROOT}")
endif()

# BZip2 Configuration - Try to find system bzip2 first, then build from source
find_package(BZip2 QUIET)
if(BZIP2_FOUND)
    include_directories(${BZIP2_INCLUDE_DIRS})
    add_definitions(-DHAVE_BZIP2)
    message(STATUS "Found system BZip2: ${BZIP2_LIBRARIES}")
    set(BZIP2_LIBRARY ${BZIP2_LIBRARIES})
else()
    # Build bzip2 from bundled source code
    set(BZIP2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/bzip2")

    if(EXISTS "${BZIP2_SOURCE_DIR}/CMakeLists.txt" AND EXISTS "${BZIP2_SOURCE_DIR}/include/bzlib.h")
        message(STATUS "Building bzip2 from source at: ${BZIP2_SOURCE_DIR}")

        # Add bzip2 subdirectory
        add_subdirectory(${BZIP2_SOURCE_DIR} ${CMAKE_BINARY_DIR}/bzip2)

        # Set variables for linking
        set(BZIP2_LIBRARY bzip2_static)
        set(BZIP2_INCLUDE_DIRS "${BZIP2_SOURCE_DIR}/include")
        set(BZIP2_FOUND TRUE)

        # Add compile definition
        add_definitions(-DHAVE_BZIP2)

        message(STATUS "BZip2 will be built from source")
        message(STATUS "bzip2 Include Dir: ${BZIP2_INCLUDE_DIRS}")
        message(STATUS "bzip2 Target: ${BZIP2_LIBRARY}")
    else()
        message(WARNING "bzip2 source code not found at ${BZIP2_SOURCE_DIR}. bzip2 decompression will be disabled.")
        message(WARNING "Expected files: CMakeLists.txt and include/bzlib.h")
        set(BZIP2_FOUND FALSE)
    endif()
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
endif()

# Source files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE PARSER_SOURCES "src/parsers/*.cpp")
file(GLOB_RECURSE EXPORTER_SOURCES "src/exporters/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB_RECURSE DATA_SOURCES "src/data/*.cpp")

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${PARSER_SOURCES}
    ${EXPORTER_SOURCES}
    ${UTILS_SOURCES}
    ${DATA_SOURCES}
)

# Create main executable
add_executable(x2fbx-converter
    src/main.cpp
    ${ALL_SOURCES}
)

# Link FBX SDK, zlib, bzip2 and system libraries
if(FBXSDK_FOUND)
    target_link_libraries(x2fbx-converter ${FBX_LIBRARIES})
endif()

# Link zlib
if(ZLIB_FOUND)
    target_link_libraries(x2fbx-converter ${ZLIB_LIBRARY})
endif()

# Link bzip2
if(BZIP2_FOUND)
    target_link_libraries(x2fbx-converter ${BZIP2_LIBRARY})
endif()

if(WIN32)
    target_link_libraries(x2fbx-converter wininet.lib ws2_32.lib advapi32.lib)
elseif(APPLE)
    target_link_libraries(x2fbx-converter "-framework CoreFoundation" "-framework SystemConfiguration")
else() # Linux
    target_link_libraries(x2fbx-converter pthread dl)
endif()

# Additional system libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(x2fbx-converter stdc++fs)
endif()

# Install rules
install(TARGETS x2fbx-converter
    RUNTIME DESTINATION bin
)

# Testing
enable_testing()
add_subdirectory(tests)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
