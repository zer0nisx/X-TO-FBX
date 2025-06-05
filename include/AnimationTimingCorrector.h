#pragma once

#include "XFileData.h"
#include "Logger.h"
#include <vector>
#include <string>

// Forward declarations for FBX SDK (when available)
#ifdef FBXSDK_FOUND
namespace fbxsdk {
    class FbxTime;
    class FbxAnimLayer;
    class FbxScene;
}
using FbxTime = fbxsdk::FbxTime;
using FbxAnimLayer = fbxsdk::FbxAnimLayer;
using FbxScene = fbxsdk::FbxScene;
#endif

namespace X2FBX {

// Result of timing correction validation
struct TimingCorrectionResult {
    bool isValid;
    float originalDurationSeconds;
    float correctedDurationSeconds;
    float timingErrorSeconds;
    float detectedTicksPerSecond;
    std::string errorDescription;

    TimingCorrectionResult()
        : isValid(false), originalDurationSeconds(0), correctedDurationSeconds(0)
        , timingErrorSeconds(0), detectedTicksPerSecond(4800.0f) {}
};

// Animation timing analysis result
struct TimingAnalysis {
    float detectedTicksPerSecond;
    float confidenceLevel;          // 0.0 to 1.0
    std::string detectionMethod;
    std::vector<float> candidateTickRates;

    TimingAnalysis() : detectedTicksPerSecond(4800.0f), confidenceLevel(0.0f) {}
};

class AnimationTimingCorrector {
private:
    Logger& logger_;

    // Common tick rates found in DirectX .x files
    static const std::vector<float> COMMON_TICK_RATES;

    // Reasonable animation duration range (in seconds)
    static constexpr float MIN_REASONABLE_DURATION = 0.05f;  // 50ms
    static constexpr float MAX_REASONABLE_DURATION = 600.0f; // 10 minutes

    // Timing tolerance for validation
    static constexpr float TIMING_TOLERANCE = 0.1f; // 100ms tolerance

public:
    AnimationTimingCorrector();

    // Main timing correction methods
    TimingAnalysis AnalyzeAnimationTiming(const XAnimationSet& animation) const;
    TimingCorrectionResult CorrectAnimationTiming(XAnimationSet& animation) const;

    // Detection methods
    float DetectTicksPerSecondFromKeyframes(const XAnimationSet& animation) const;
    float DetectTicksPerSecondFromDuration(const XAnimationSet& animation) const;
    float DetectTicksPerSecondFromHeader(const XFileData& fileData) const;

    // Validation methods
    bool ValidateAnimationDuration(float durationSeconds) const;
    TimingCorrectionResult ValidateTimingCorrection(
        const XAnimationSet& original,
        const XAnimationSet& corrected) const;

    // Utility methods
    std::vector<float> GetCandidateTickRates(const XAnimationSet& animation) const;
    float CalculateConfidence(float tickRate, const XAnimationSet& animation) const;

    // Keyframe time conversion
    std::vector<XKeyframe> ConvertKeyframeTiming(
        const std::vector<XKeyframe>& originalKeyframes,
        float originalTicksPerSecond,
        float targetTicksPerSecond) const;

#ifdef FBXSDK_FOUND
    // FBX-specific timing correction (when FBX SDK is available)
    bool ApplyTimingToFBXLayer(const XAnimationSet& animation, FbxAnimLayer* layer) const;
    void ConfigureFBXSceneTiming(FbxScene* scene) const;
#endif

    // Batch processing
    std::vector<TimingCorrectionResult> CorrectAllAnimations(
        std::vector<XAnimationSet>& animations) const;

    // Reporting
    void GenerateTimingReport(const std::vector<TimingCorrectionResult>& results) const;

private:
    // Internal helper methods
    bool IsReasonableDuration(float durationSeconds) const;
    float ScoreTickRate(float tickRate, const XAnimationSet& animation) const;
    std::string GetDetectionMethodDescription(float tickRate,
                                            const XAnimationSet& animation) const;

    // Statistical analysis
    float CalculateVariance(const std::vector<float>& values) const;
    float CalculateMedian(std::vector<float> values) const;

    // Keyframe analysis
    std::vector<float> ExtractKeyframeTimes(const XAnimationSet& animation) const;
    float AnalyzeKeyframePattern(const std::vector<float>& keyframeTimes) const;
};

// Utility functions for timing conversion
namespace TimingUtils {
    // Convert between different time units
    double XTicksToSeconds(float xTicks, float ticksPerSecond);
    float SecondsToXTicks(double seconds, float ticksPerSecond);

    // FBX time conversion (when SDK available)
#ifdef FBXSDK_FOUND
    FbxTime SecondsToFbxTime(double seconds);
    double FbxTimeToSeconds(const FbxTime& fbxTime);
#endif

    // Validation helpers
    bool IsValidTickRate(float tickRate);
    bool IsValidDuration(float durationSeconds);

    // Common DirectX .x file tick rates
    extern const float DIRECTX_DEFAULT_TICKS;     // 4800
    extern const float DIRECTX_ALT_TICKS_1;       // 160
    extern const float DIRECTX_ALT_TICKS_2;       // 1000
    extern const float DIRECTX_ALT_TICKS_3;       // 2400
    extern const float DIRECTX_ALT_TICKS_4;       // 9600
}

} // namespace X2FBX
