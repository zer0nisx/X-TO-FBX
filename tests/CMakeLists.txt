# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Test executable
add_executable(x2fbx-tests
    test_main.cpp
    test_timing_corrector.cpp
    test_xfile_parser.cpp
    test_data_structures.cpp
)

# Link with main library sources
target_link_libraries(x2fbx-tests
    ${ALL_SOURCES}
)

# Include test data directory
add_custom_target(copy_test_data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/test_data
    ${CMAKE_BINARY_DIR}/test_data
)

add_dependencies(x2fbx-tests copy_test_data)

# Add tests to CTest
add_test(NAME TimingCorrectorTests COMMAND x2fbx-tests --test-timing)
add_test(NAME XFileParserTests COMMAND x2fbx-tests --test-parser)
add_test(NAME DataStructureTests COMMAND x2fbx-tests --test-data)
