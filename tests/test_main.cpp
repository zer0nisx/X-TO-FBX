#include <iostream>
#include <string>
#include <vector>

// Project headers
#include "XFileData.h"
#include "AnimationTimingCorrector.h"
#include "Logger.h"

using namespace X2FBX;

// Test function declarations
bool TestTimingCorrector();
bool TestXFileParser();
bool TestDataStructures();

int main(int argc, char* argv[]) {
    std::cout << "X2FBX Converter Test Suite" << std::endl;
    std::cout << "==========================" << std::endl;

    // Initialize logger for tests
    Logger::Initialize("test_log.txt", LogLevel::DEBUG);

    bool allTestsPassed = true;

    // Parse command line for specific tests
    bool runAll = true;
    bool runTiming = false;
    bool runParser = false;
    bool runData = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test-timing") {
            runTiming = true;
            runAll = false;
        } else if (arg == "--test-parser") {
            runParser = true;
            runAll = false;
        } else if (arg == "--test-data") {
            runData = true;
            runAll = false;
        }
    }

    if (runAll || runData) {
        std::cout << "\nRunning Data Structure Tests..." << std::endl;
        if (TestDataStructures()) {
            std::cout << "✓ Data Structure Tests PASSED" << std::endl;
        } else {
            std::cout << "✗ Data Structure Tests FAILED" << std::endl;
            allTestsPassed = false;
        }
    }

    if (runAll || runTiming) {
        std::cout << "\nRunning Timing Corrector Tests..." << std::endl;
        if (TestTimingCorrector()) {
            std::cout << "✓ Timing Corrector Tests PASSED" << std::endl;
        } else {
            std::cout << "✗ Timing Corrector Tests FAILED" << std::endl;
            allTestsPassed = false;
        }
    }

    if (runAll || runParser) {
        std::cout << "\nRunning X File Parser Tests..." << std::endl;
        if (TestXFileParser()) {
            std::cout << "✓ X File Parser Tests PASSED" << std::endl;
        } else {
            std::cout << "✗ X File Parser Tests FAILED" << std::endl;
            allTestsPassed = false;
        }
    }

    std::cout << "\n==========================" << std::endl;
    if (allTestsPassed) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        return 1;
    }
}

bool TestDataStructures() {
    // Test XMatrix4x4
    XMatrix4x4 identity = XMatrix4x4::Identity();
    if (identity.m[0][0] != 1.0f || identity.m[1][1] != 1.0f ||
        identity.m[2][2] != 1.0f || identity.m[3][3] != 1.0f) {
        std::cout << "  FAIL: Identity matrix incorrect" << std::endl;
        return false;
    }

    // Test XAnimationSet
    XAnimationSet testAnim;
    testAnim.name = "test_animation";
    testAnim.duration = 4800.0f;
    testAnim.ticksPerSecond = 4800.0f;

    float durationSeconds = testAnim.GetDurationInSeconds();
    if (std::abs(durationSeconds - 1.0f) > 0.001f) {
        std::cout << "  FAIL: Animation duration calculation incorrect: " << durationSeconds << std::endl;
        return false;
    }

    // Test XMeshData validation
    XMeshData meshData;

    // Empty mesh should be invalid
    if (meshData.IsValid()) {
        std::cout << "  FAIL: Empty mesh should be invalid" << std::endl;
        return false;
    }

    // Add basic valid data
    XVertex vertex;
    vertex.position = XVector3(0, 0, 0);
    meshData.vertices.push_back(vertex);
    meshData.vertices.push_back(XVertex());
    meshData.vertices.push_back(XVertex());

    XFace face;
    face.indices[0] = 0;
    face.indices[1] = 1;
    face.indices[2] = 2;
    meshData.faces.push_back(face);

    if (!meshData.IsValid()) {
        auto errors = meshData.GetValidationErrors();
        std::cout << "  FAIL: Valid mesh reported as invalid. Errors:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "    - " << error << std::endl;
        }
        return false;
    }

    std::cout << "  Data structure tests completed successfully" << std::endl;
    return true;
}

bool TestTimingCorrector() {
    AnimationTimingCorrector corrector;

    // Test 1: Animation with default DirectX timing
    XAnimationSet testAnim1;
    testAnim1.name = "test_walk";
    testAnim1.duration = 4800.0f;  // 1 second at 4800 ticks/sec
    testAnim1.ticksPerSecond = 4800.0f;

    // Add some keyframes
    XKeyframe keyframe1;
    keyframe1.time = 0;
    testAnim1.keyframes.push_back(keyframe1);

    XKeyframe keyframe2;
    keyframe2.time = 2400.0f;  // 0.5 seconds
    testAnim1.keyframes.push_back(keyframe2);

    XKeyframe keyframe3;
    keyframe3.time = 4800.0f;  // 1.0 seconds
    testAnim1.keyframes.push_back(keyframe3);

    TimingCorrectionResult result1 = corrector.CorrectAnimationTiming(testAnim1);

    if (!result1.isValid) {
        std::cout << "  FAIL: Valid animation timing reported as invalid: " << result1.errorDescription << std::endl;
        return false;
    }

    if (std::abs(result1.correctedDurationSeconds - 1.0f) > 0.01f) {
        std::cout << "  FAIL: Incorrect duration correction: " << result1.correctedDurationSeconds << " (expected ~1.0)" << std::endl;
        return false;
    }

    // Test 2: Animation with incorrect timing (very large duration)
    XAnimationSet testAnim2;
    testAnim2.name = "test_broken";
    testAnim2.duration = 4800000.0f;  // Unreasonably large
    testAnim2.ticksPerSecond = 1.0f;   // Incorrect rate

    TimingAnalysis analysis = corrector.AnalyzeAnimationTiming(testAnim2);

    if (analysis.detectedTicksPerSecond <= 1.0f) {
        std::cout << "  FAIL: Failed to detect reasonable tick rate for broken animation" << std::endl;
        return false;
    }

    // Test 3: Validation of reasonable durations
    if (!corrector.ValidateAnimationDuration(1.0f)) {
        std::cout << "  FAIL: 1 second duration should be valid" << std::endl;
        return false;
    }

    if (corrector.ValidateAnimationDuration(0.01f)) {
        std::cout << "  FAIL: 0.01 second duration should be invalid (too short)" << std::endl;
        return false;
    }

    if (corrector.ValidateAnimationDuration(1000.0f)) {
        std::cout << "  FAIL: 1000 second duration should be invalid (too long)" << std::endl;
        return false;
    }

    std::cout << "  Timing corrector tests completed successfully" << std::endl;
    return true;
}

bool TestXFileParser() {
    // Since we don't have actual .x files for testing, we'll test utility functions

    // Test timing extraction
    std::string testContent = R"(
        template AnimTicksPerSecond {
            <9E415A43-7BA6-4a73-8743-B73D47E88476>
            DWORD fps;
        }

        AnimTicksPerSecond {
            4800;
        }
    )";

    float extractedTicks = 0;
    if (!XFileUtils::ExtractTicksPerSecond(testContent, extractedTicks)) {
        std::cout << "  FAIL: Failed to extract ticks per second from test content" << std::endl;
        return false;
    }

    if (std::abs(extractedTicks - 4800.0f) > 0.1f) {
        std::cout << "  FAIL: Incorrect ticks per second extracted: " << extractedTicks << " (expected 4800)" << std::endl;
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
            0.0; 1.0; 0.0;; // vertex 3
        }
    )";

    std::string cleaned = XFileUtils::RemoveComments(contentWithComments);
    if (cleaned.find("//") != std::string::npos || cleaned.find("/*") != std::string::npos) {
        std::cout << "  FAIL: Comment removal failed" << std::endl;
        return false;
    }

    std::cout << "  X File parser tests completed successfully" << std::endl;
    return true;
}
