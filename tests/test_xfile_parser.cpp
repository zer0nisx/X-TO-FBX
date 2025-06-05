#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include "XFileParser.h"
#include "BinaryXFileParser.h"
#include "Logger.h"

using namespace X2FBX;

// Test helper functions
bool CreateTestXFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    file.close();
    return true;
}

// Test X file content templates
const std::string SIMPLE_MESH_X_FILE = R"(xof 0303txt 0032

Mesh testMesh {
    4;
    0.0; 0.0; 0.0;,
    1.0; 0.0; 0.0;,
    1.0; 1.0; 0.0;,
    0.0; 1.0; 0.0;;

    2;
    3; 0, 1, 2;,
    3; 0, 2, 3;;
}
)";

const std::string ANIMATED_MESH_X_FILE = R"(xof 0303txt 0032

Mesh testMesh {
    3;
    0.0; 0.0; 0.0;,
    1.0; 0.0; 0.0;,
    0.5; 1.0; 0.0;;

    1;
    3; 0, 1, 2;;
}

AnimationSet testAnimation {
    Animation {
        AnimationKey {
            2;
            3;
            0; 3; 0.0, 0.0, 0.0;;
            1600; 3; 1.0, 0.0, 0.0;;
            4800; 3; 0.0, 0.0, 0.0;;
        }
    }
}
)";

const std::string MESH_WITH_MATERIALS_X_FILE = R"(xof 0303txt 0032

Mesh testMesh {
    4;
    0.0; 0.0; 0.0;,
    1.0; 0.0; 0.0;,
    1.0; 1.0; 0.0;,
    0.0; 1.0; 0.0;;

    2;
    3; 0, 1, 2;,
    3; 0, 2, 3;;

    MeshMaterialList {
        1;
        2;
        0, 0;;

        Material testMaterial {
            1.0; 0.0; 0.0; 1.0;;
            10.0;
            1.0; 1.0; 1.0;;
            0.0; 0.0; 0.0;;

            TextureFilename {
                "test_texture.jpg";
            }
        }
    }
}
)";

// Test functions
bool TestBasicXFileParsing() {
    std::cout << "Testing basic X file parsing..." << std::endl;

    // Create test file
    if (!CreateTestXFile("test_simple.x", SIMPLE_MESH_X_FILE)) {
        std::cout << "  FAIL: Could not create test file" << std::endl;
        return false;
    }

    XFileParser parser;
    parser.SetVerboseLogging(false);

    if (!parser.ParseFile("test_simple.x")) {
        std::cout << "  FAIL: Failed to parse simple X file" << std::endl;
        return false;
    }

    const XFileData& data = parser.GetParsedData();

    if (!data.IsValid()) {
        std::cout << "  FAIL: Parsed data is not valid" << std::endl;
        auto errors = data.meshData.GetValidationErrors();
        for (const auto& error : errors) {
            std::cout << "    Error: " << error << std::endl;
        }
        return false;
    }

    // Check parsed mesh data
    if (data.meshData.GetVertexCount() != 4) {
        std::cout << "  FAIL: Expected 4 vertices, got " << data.meshData.GetVertexCount() << std::endl;
        return false;
    }

    if (data.meshData.GetFaceCount() != 2) {
        std::cout << "  FAIL: Expected 2 faces, got " << data.meshData.GetFaceCount() << std::endl;
        return false;
    }

    std::cout << "  PASS: Basic X file parsing" << std::endl;
    return true;
}

bool TestAnimationParsing() {
    std::cout << "Testing animation parsing..." << std::endl;

    if (!CreateTestXFile("test_animated.x", ANIMATED_MESH_X_FILE)) {
        std::cout << "  FAIL: Could not create animated test file" << std::endl;
        return false;
    }

    XFileParser parser;
    parser.SetVerboseLogging(false);

    if (!parser.ParseFile("test_animated.x")) {
        std::cout << "  FAIL: Failed to parse animated X file" << std::endl;
        return false;
    }

    const XFileData& data = parser.GetParsedData();

    if (data.meshData.GetAnimationCount() == 0) {
        std::cout << "  FAIL: No animations found in animated X file" << std::endl;
        return false;
    }

    const auto& animation = data.meshData.animations[0];
    if (animation.keyframes.empty()) {
        std::cout << "  FAIL: Animation has no keyframes" << std::endl;
        return false;
    }

    if (animation.keyframes.size() != 3) {
        std::cout << "  FAIL: Expected 3 keyframes, got " << animation.keyframes.size() << std::endl;
        return false;
    }

    // Check timing
    if (animation.duration <= 0) {
        std::cout << "  FAIL: Animation has invalid duration: " << animation.duration << std::endl;
        return false;
    }

    std::cout << "  PASS: Animation parsing (" << animation.keyframes.size() << " keyframes)" << std::endl;
    return true;
}

bool TestMaterialParsing() {
    std::cout << "Testing material parsing..." << std::endl;

    if (!CreateTestXFile("test_materials.x", MESH_WITH_MATERIALS_X_FILE)) {
        std::cout << "  FAIL: Could not create material test file" << std::endl;
        return false;
    }

    XFileParser parser;
    parser.SetVerboseLogging(false);

    if (!parser.ParseFile("test_materials.x")) {
        std::cout << "  FAIL: Failed to parse X file with materials" << std::endl;
        return false;
    }

    const XFileData& data = parser.GetParsedData();

    if (data.meshData.materials.empty()) {
        std::cout << "  FAIL: No materials found" << std::endl;
        return false;
    }

    const auto& material = data.meshData.materials[0];

    // Check diffuse color (should be red: 1.0, 0.0, 0.0)
    if (material.diffuseColor.x != 1.0f || material.diffuseColor.y != 0.0f || material.diffuseColor.z != 0.0f) {
        std::cout << "  FAIL: Incorrect diffuse color: " << material.diffuseColor.x << ", "
                  << material.diffuseColor.y << ", " << material.diffuseColor.z << std::endl;
        return false;
    }

    // Check texture
    if (material.diffuseTexture.empty()) {
        std::cout << "  FAIL: No diffuse texture found" << std::endl;
        return false;
    }

    if (material.diffuseTexture != "test_texture.jpg") {
        std::cout << "  FAIL: Incorrect texture filename: " << material.diffuseTexture << std::endl;
        return false;
    }

    std::cout << "  PASS: Material parsing (texture: " << material.diffuseTexture << ")" << std::endl;
    return true;
}

bool TestHeaderParsing() {
    std::cout << "Testing X file header parsing..." << std::endl;

    XFileParser parser;

    // Test with simple mesh
    if (!parser.ParseFile("test_simple.x")) {
        std::cout << "  FAIL: Failed to parse file for header test" << std::endl;
        return false;
    }

    const XFileData& data = parser.GetParsedData();

    // Check header information
    if (data.header.majorVersion != 3 || data.header.minorVersion != 3) {
        std::cout << "  FAIL: Incorrect version: " << data.header.majorVersion << "." << data.header.minorVersion << std::endl;
        return false;
    }

    if (data.header.format != XFileHeader::Format::TEXT) {
        std::cout << "  FAIL: Incorrect format detected" << std::endl;
        return false;
    }

    std::cout << "  PASS: Header parsing (v" << data.header.majorVersion << "." << data.header.minorVersion << ")" << std::endl;
    return true;
}

bool TestUtilityFunctions() {
    std::cout << "Testing X file utility functions..." << std::endl;

    // Test timing extraction
    std::string testContent = R"(
        AnimTicksPerSecond {
            4800;
        }
    )";

    float extractedTicks = 0;
    if (!XFileUtils::ExtractTicksPerSecond(testContent, extractedTicks)) {
        std::cout << "  FAIL: Failed to extract ticks per second" << std::endl;
        return false;
    }

    if (std::abs(extractedTicks - 4800.0f) > 0.1f) {
        std::cout << "  FAIL: Incorrect ticks extracted: " << extractedTicks << std::endl;
        return false;
    }

    // Test float parsing
    bool success;
    float testFloat = XFileUtils::ParseFloat("123.456", success);
    if (!success || std::abs(testFloat - 123.456f) > 0.001f) {
        std::cout << "  FAIL: Float parsing failed" << std::endl;
        return false;
    }

    // Test int parsing
    int testInt = XFileUtils::ParseInt("789", success);
    if (!success || testInt != 789) {
        std::cout << "  FAIL: Int parsing failed" << std::endl;
        return false;
    }

    // Test comment removal
    std::string contentWithComments = R"(
        // This is a comment
        Mesh {
            3; // vertex count
            0.0; 0.0; 0.0;, // vertex 1
            /* block comment */ 1.0; 0.0; 0.0;, // vertex 2
        }
    )";

    std::string cleaned = XFileUtils::RemoveComments(contentWithComments);
    if (cleaned.find("//") != std::string::npos || cleaned.find("/*") != std::string::npos) {
        std::cout << "  FAIL: Comment removal failed" << std::endl;
        return false;
    }

    std::cout << "  PASS: Utility functions" << std::endl;
    return true;
}

bool TestFileValidation() {
    std::cout << "Testing X file validation..." << std::endl;

    // Test valid file
    if (!XFileUtils::ValidateXFileSignature("test_simple.x")) {
        std::cout << "  FAIL: Valid X file not recognized" << std::endl;
        return false;
    }

    // Create invalid file
    if (!CreateTestXFile("test_invalid.x", "This is not an X file")) {
        std::cout << "  FAIL: Could not create invalid test file" << std::endl;
        return false;
    }

    if (XFileUtils::ValidateXFileSignature("test_invalid.x")) {
        std::cout << "  FAIL: Invalid X file incorrectly validated" << std::endl;
        return false;
    }

    // Test non-existent file
    if (XFileUtils::ValidateXFileSignature("nonexistent.x")) {
        std::cout << "  FAIL: Non-existent file incorrectly validated" << std::endl;
        return false;
    }

    std::cout << "  PASS: File validation" << std::endl;
    return true;
}

bool TestErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;

    XFileParser parser;
    parser.SetStrictMode(true);

    // Test with malformed content
    std::string malformedContent = R"(xof 0303txt 0032
    Mesh {
        // Missing vertex count and data
    }
    )";

    if (!CreateTestXFile("test_malformed.x", malformedContent)) {
        std::cout << "  FAIL: Could not create malformed test file" << std::endl;
        return false;
    }

    if (parser.ParseFile("test_malformed.x")) {
        std::cout << "  FAIL: Malformed file should not parse successfully" << std::endl;
        return false;
    }

    const XFileData& data = parser.GetParsedData();
    if (data.parseErrors.empty()) {
        std::cout << "  FAIL: No parse errors reported for malformed file" << std::endl;
        return false;
    }

    std::cout << "  PASS: Error handling (" << data.parseErrors.size() << " errors caught)" << std::endl;
    return true;
}

bool TestEnhancedParser() {
    std::cout << "Testing enhanced X file parser..." << std::endl;

    EnhancedXFileParser enhancedParser;
    enhancedParser.SetVerboseLogging(false);

    if (!enhancedParser.ParseFile("test_simple.x")) {
        std::cout << "  FAIL: Enhanced parser failed to parse simple file" << std::endl;
        return false;
    }

    const XFileData& data = enhancedParser.GetParsedData();

    if (!data.IsValid()) {
        std::cout << "  FAIL: Enhanced parser produced invalid data" << std::endl;
        return false;
    }

    // Test format detection
    XFileHeader::Format detectedFormat = enhancedParser.DetectFileFormat("test_simple.x");
    if (detectedFormat != XFileHeader::Format::TEXT) {
        std::cout << "  FAIL: Incorrect format detection" << std::endl;
        return false;
    }

    std::cout << "  PASS: Enhanced parser" << std::endl;
    return true;
}

// Cleanup function
void CleanupTestFiles() {
    std::remove("test_simple.x");
    std::remove("test_animated.x");
    std::remove("test_materials.x");
    std::remove("test_invalid.x");
    std::remove("test_malformed.x");
}

// Main test runner
bool RunAllXFileParserTests() {
    std::cout << "\n=== X File Parser Tests ===" << std::endl;

    bool allPassed = true;

    allPassed &= TestBasicXFileParsing();
    allPassed &= TestAnimationParsing();
    allPassed &= TestMaterialParsing();
    allPassed &= TestHeaderParsing();
    allPassed &= TestUtilityFunctions();
    allPassed &= TestFileValidation();
    allPassed &= TestErrorHandling();
    allPassed &= TestEnhancedParser();

    // Cleanup
    CleanupTestFiles();

    if (allPassed) {
        std::cout << "\n✓ All X file parser tests PASSED!" << std::endl;
    } else {
        std::cout << "\n✗ Some X file parser tests FAILED!" << std::endl;
    }

    return allPassed;
}
