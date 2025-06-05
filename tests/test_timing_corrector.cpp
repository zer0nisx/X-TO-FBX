#include <iostream>
#include <cassert>
#include <cmath>
#include "AnimationTimingCorrector.h"
#include "Logger.h"

using namespace X2FBX;

// Test helper functions
bool FloatEqual(float a, float b, float tolerance = 0.001f) {
    return std::abs(a - b) < tolerance;
}

XAnimationSet CreateTestAnimation(const std::string& name, float duration, float ticksPerSecond) {
    XAnimationSet anim;
    anim.name = name;
    anim.duration = duration;
    anim.ticksPerSecond = ticksPerSecond;

    // Add some test keyframes
    for (int i = 0; i <= 3; i++) {
        XKeyframe keyframe;
        keyframe.time = (duration / 3.0f) * i;
        keyframe.position = XVector3(i * 1.0f, 0, 0);
        keyframe.rotation = XQuaternion(0, 0, 0, 1);
        keyframe.scale = XVector3(1, 1, 1);
        anim.keyframes.push_back(keyframe);
    }

    return anim;
}

// Test functions
bool TestBasicTimingCorrection() {
    std::cout << "Testing basic timing correction..." << std::endl;

    AnimationTimingCorrector corrector;

    // Test with correct DirectX timing
    XAnimationSet anim = CreateTestAnimation("test_walk", 4800.0f, 4800.0f);

    TimingCorrectionResult result = corrector.CorrectAnimationTiming(anim);

    if (!result.isValid) {
        std::cout << "  FAIL: Valid animation reported as invalid: " << result.errorDescription << std::endl;
        return false;
    }

    if (!FloatEqual(result.correctedDurationSeconds, 1.0f)) {
        std::cout << "  FAIL: Incorrect duration: " << result.correctedDurationSeconds << " (expected 1.0)" << std::endl;
        return false;
    }

    std::cout << "  PASS: Basic timing correction" << std::endl;
    return true;
}

bool TestTimingDetection() {
    std::cout << "Testing timing detection..." << std::endl;

    AnimationTimingCorrector corrector;

    // Test with broken timing (too fast)
    XAnimationSet badAnim = CreateTestAnimation("test_broken", 4800000.0f, 1.0f);

    TimingAnalysis analysis = corrector.AnalyzeAnimationTiming(badAnim);

    if (analysis.detectedTicksPerSecond <= 1.0f) {
        std::cout << "  FAIL: Failed to detect reasonable tick rate" << std::endl;
        return false;
    }

    if (analysis.confidenceLevel < 0.0f || analysis.confidenceLevel > 1.0f) {
        std::cout << "  FAIL: Invalid confidence level: " << analysis.confidenceLevel << std::endl;
        return false;
    }

    std::cout << "  PASS: Timing detection (detected " << analysis.detectedTicksPerSecond << " ticks/sec)" << std::endl;
    return true;
}

bool TestDurationValidation() {
    std::cout << "Testing duration validation..." << std::endl;

    AnimationTimingCorrector corrector;

    // Test valid durations
    if (!corrector.ValidateAnimationDuration(1.0f)) {
        std::cout << "  FAIL: 1 second duration should be valid" << std::endl;
        return false;
    }

    if (!corrector.ValidateAnimationDuration(30.0f)) {
        std::cout << "  FAIL: 30 second duration should be valid" << std::endl;
        return false;
    }

    // Test invalid durations
    if (corrector.ValidateAnimationDuration(0.01f)) {
        std::cout << "  FAIL: 0.01 second duration should be invalid (too short)" << std::endl;
        return false;
    }

    if (corrector.ValidateAnimationDuration(1000.0f)) {
        std::cout << "  FAIL: 1000 second duration should be invalid (too long)" << std::endl;
        return false;
    }

    std::cout << "  PASS: Duration validation" << std::endl;
    return true;
}

bool TestBatchCorrection() {
    std::cout << "Testing batch animation correction..." << std::endl;

    AnimationTimingCorrector corrector;

    // Create multiple test animations
    std::vector<XAnimationSet> animations;
    animations.push_back(CreateTestAnimation("walk", 4800.0f, 4800.0f));
    animations.push_back(CreateTestAnimation("run", 2400.0f, 4800.0f));
    animations.push_back(CreateTestAnimation("idle", 9600.0f, 4800.0f));

    std::vector<TimingCorrectionResult> results = corrector.CorrectAllAnimations(animations);

    if (results.size() != 3) {
        std::cout << "  FAIL: Expected 3 results, got " << results.size() << std::endl;
        return false;
    }

    int validCount = 0;
    for (const auto& result : results) {
        if (result.isValid) {
            validCount++;
        }
    }

    if (validCount != 3) {
        std::cout << "  FAIL: Expected 3 valid corrections, got " << validCount << std::endl;
        return false;
    }

    std::cout << "  PASS: Batch correction (" << validCount << "/3 valid)" << std::endl;
    return true;
}

bool TestTickRateDetection() {
    std::cout << "Testing tick rate detection methods..." << std::endl;

    AnimationTimingCorrector corrector;

    // Test detection from keyframes
    XAnimationSet anim = CreateTestAnimation("test", 4800.0f, 4800.0f);

    float detectedRate = corrector.DetectTicksPerSecondFromKeyframes(anim);
    if (detectedRate <= 0) {
        std::cout << "  FAIL: Failed to detect tick rate from keyframes" << std::endl;
        return false;
    }

    // Test detection from duration
    float durationRate = corrector.DetectTicksPerSecondFromDuration(anim);
    if (durationRate <= 0) {
        std::cout << "  FAIL: Failed to detect tick rate from duration" << std::endl;
        return false;
    }

    std::cout << "  PASS: Tick rate detection (keyframes: " << detectedRate <<
                 ", duration: " << durationRate << ")" << std::endl;
    return true;
}

bool TestCandidateRates() {
    std::cout << "Testing candidate tick rates..." << std::endl;

    AnimationTimingCorrector corrector;
    XAnimationSet anim = CreateTestAnimation("test", 4800.0f, 4800.0f);

    std::vector<float> candidates = corrector.GetCandidateTickRates(anim);

    if (candidates.empty()) {
        std::cout << "  FAIL: No candidate tick rates generated" << std::endl;
        return false;
    }

    // Should include common DirectX rates
    bool hasDefault = false;
    for (float rate : candidates) {
        if (FloatEqual(rate, 4800.0f)) {
            hasDefault = true;
            break;
        }
    }

    if (!hasDefault) {
        std::cout << "  FAIL: Default DirectX rate (4800) not in candidates" << std::endl;
        return false;
    }

    std::cout << "  PASS: Candidate rates (" << candidates.size() << " candidates)" << std::endl;
    return true;
}

bool TestKeyframeTimeConversion() {
    std::cout << "Testing keyframe time conversion..." << std::endl;

    AnimationTimingCorrector corrector;

    // Create keyframes with known timing
    std::vector<XKeyframe> originalKeyframes;
    for (int i = 0; i < 3; i++) {
        XKeyframe kf;
        kf.time = i * 1600.0f; // 1600 ticks intervals
        originalKeyframes.push_back(kf);
    }

    // Convert from 4800 ticks/sec to 30 fps
    std::vector<XKeyframe> convertedKeyframes = corrector.ConvertKeyframeTiming(
        originalKeyframes, 4800.0f, 30.0f);

    if (convertedKeyframes.size() != originalKeyframes.size()) {
        std::cout << "  FAIL: Keyframe count mismatch after conversion" << std::endl;
        return false;
    }

    // Check timing conversion (1600 ticks @ 4800 ticks/sec = 1/3 second = 10 frames @ 30fps)
    float expectedTime = (1600.0f / 4800.0f) * 30.0f; // Should be 10.0
    if (!FloatEqual(convertedKeyframes[1].time, expectedTime)) {
        std::cout << "  FAIL: Incorrect time conversion: " << convertedKeyframes[1].time <<
                     " (expected " << expectedTime << ")" << std::endl;
        return false;
    }

    std::cout << "  PASS: Keyframe time conversion" << std::endl;
    return true;
}

// Main test runner
bool RunAllTimingCorrectorTests() {
    std::cout << "\n=== Animation Timing Corrector Tests ===" << std::endl;

    bool allPassed = true;

    allPassed &= TestBasicTimingCorrection();
    allPassed &= TestTimingDetection();
    allPassed &= TestDurationValidation();
    allPassed &= TestBatchCorrection();
    allPassed &= TestTickRateDetection();
    allPassed &= TestCandidateRates();
    allPassed &= TestKeyframeTimeConversion();

    if (allPassed) {
        std::cout << "\n✓ All timing corrector tests PASSED!" << std::endl;
    } else {
        std::cout << "\n✗ Some timing corrector tests FAILED!" << std::endl;
    }

    return allPassed;
}
