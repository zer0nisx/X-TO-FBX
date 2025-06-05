#include "AnimationTimingCorrector.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace X2FBX {

// Static constants
const std::vector<float> AnimationTimingCorrector::COMMON_TICK_RATES = {
    160.0f,     // Alternative DirectX rate
    1000.0f,    // Millisecond-based
    2400.0f,    // Half of default
    4800.0f,    // Default DirectX rate
    9600.0f,    // Double default
    30.0f,      // Frame-based (30 FPS)
    60.0f,      // Frame-based (60 FPS)
    24.0f,      // Film standard
    25.0f,      // PAL standard
    29.97f      // NTSC standard
};

AnimationTimingCorrector::AnimationTimingCorrector()
    : logger_(Logger::GetInstance()) {
    LOG_DEBUG("AnimationTimingCorrector initialized");
}

TimingAnalysis AnimationTimingCorrector::AnalyzeAnimationTiming(const XAnimationSet& animation) const {
    TIME_OPERATION("AnimationTimingAnalysis");

    TimingAnalysis analysis;

    if (animation.keyframes.empty()) {
        LOG_WARNING("Animation '" + animation.name + "' has no keyframes for timing analysis");
        return analysis;
    }

    // Method 1: Use explicitly set ticksPerSecond if available and reasonable
    if (animation.ticksPerSecond > 0) {
        float durationSeconds = animation.duration / animation.ticksPerSecond;
        if (IsReasonableDuration(durationSeconds)) {
            analysis.detectedTicksPerSecond = animation.ticksPerSecond;
            analysis.confidenceLevel = 0.9f;
            analysis.detectionMethod = "Explicit from animation header";
            LOG_INFO("Using explicit ticksPerSecond: " + std::to_string(animation.ticksPerSecond));
            return analysis;
        }
    }

    // Method 2: Analyze keyframe patterns
    std::vector<float> candidateRates = GetCandidateTickRates(animation);
    analysis.candidateTickRates = candidateRates;

    float bestTickRate = 4800.0f;
    float bestScore = 0.0f;

    for (float tickRate : candidateRates) {
        float score = ScoreTickRate(tickRate, animation);
        if (score > bestScore) {
            bestScore = score;
            bestTickRate = tickRate;
        }
    }

    analysis.detectedTicksPerSecond = bestTickRate;
    analysis.confidenceLevel = bestScore;
    analysis.detectionMethod = GetDetectionMethodDescription(bestTickRate, animation);

    LOG_INFO("Detected ticksPerSecond: " + std::to_string(bestTickRate) +
             " (confidence: " + std::to_string(bestScore) + ")");

    return analysis;
}

TimingCorrectionResult AnimationTimingCorrector::CorrectAnimationTiming(XAnimationSet& animation) const {
    TIME_OPERATION("TimingCorrection");

    TimingCorrectionResult result;

    // Store original values
    float originalTicksPerSecond = animation.ticksPerSecond;
    float originalDuration = animation.duration;
    result.originalDurationSeconds = originalDuration / originalTicksPerSecond;

    // Analyze timing
    TimingAnalysis analysis = AnalyzeAnimationTiming(animation);
    result.detectedTicksPerSecond = analysis.detectedTicksPerSecond;

    // Apply correction if needed
    if (std::abs(originalTicksPerSecond - analysis.detectedTicksPerSecond) > 0.1f) {
        LOG_INFO("Correcting timing for animation '" + animation.name + "' from " +
                std::to_string(originalTicksPerSecond) + " to " +
                std::to_string(analysis.detectedTicksPerSecond) + " ticks/sec");

        // Update animation timing
        animation.ticksPerSecond = analysis.detectedTicksPerSecond;

        // Convert keyframes if necessary
        if (!animation.keyframes.empty()) {
            // The keyframe times stay the same, only the interpretation changes
            // But we might need to validate and adjust them

            float timeScale = originalTicksPerSecond / analysis.detectedTicksPerSecond;
            if (std::abs(timeScale - 1.0f) > 0.01f) {
                // Only scale if there's a significant difference
                for (auto& keyframe : animation.keyframes) {
                    keyframe.time *= timeScale;
                }

                // Update duration
                animation.duration *= timeScale;
            }
        }
    }

    // Calculate corrected duration
    result.correctedDurationSeconds = animation.duration / animation.ticksPerSecond;
    result.timingErrorSeconds = std::abs(result.originalDurationSeconds - result.correctedDurationSeconds);

    // Validate result
    result.isValid = ValidateAnimationDuration(result.correctedDurationSeconds) &&
                     result.timingErrorSeconds <= TIMING_TOLERANCE;

    if (!result.isValid) {
        result.errorDescription = "Timing correction failed validation. ";
        if (!ValidateAnimationDuration(result.correctedDurationSeconds)) {
            result.errorDescription += "Duration out of reasonable range. ";
        }
        if (result.timingErrorSeconds > TIMING_TOLERANCE) {
            result.errorDescription += "Timing error too large: " +
                                     std::to_string(result.timingErrorSeconds) + "s";
        }
    }

    // Log result
    logger_.LogAnimationTiming(animation.name, result.originalDurationSeconds,
                              result.correctedDurationSeconds, analysis.detectedTicksPerSecond);

    return result;
}

float AnimationTimingCorrector::DetectTicksPerSecondFromKeyframes(const XAnimationSet& animation) const {
    if (animation.keyframes.size() < 2) {
        return 4800.0f; // Default
    }

    // Extract keyframe times
    std::vector<float> keyframeTimes = ExtractKeyframeTimes(animation);

    // Analyze the pattern
    return AnalyzeKeyframePattern(keyframeTimes);
}

float AnimationTimingCorrector::DetectTicksPerSecondFromDuration(const XAnimationSet& animation) const {
    if (animation.duration <= 0) {
        return 4800.0f;
    }

    // Try different tick rates and see which gives reasonable duration
    for (float tickRate : COMMON_TICK_RATES) {
        float durationSeconds = animation.duration / tickRate;
        if (IsReasonableDuration(durationSeconds)) {
            return tickRate;
        }
    }

    return 4800.0f; // Default fallback
}

float AnimationTimingCorrector::DetectTicksPerSecondFromHeader(const XFileData& fileData) const {
    // Check file header first
    if (fileData.header.hasAnimationTimingInfo && fileData.header.ticksPerSecond > 0) {
        return fileData.header.ticksPerSecond;
    }

    // Check mesh data global timing
    if (fileData.meshData.hasTimingInfo && fileData.meshData.globalTicksPerSecond > 0) {
        return fileData.meshData.globalTicksPerSecond;
    }

    return 4800.0f; // Default
}

bool AnimationTimingCorrector::ValidateAnimationDuration(float durationSeconds) const {
    return durationSeconds >= MIN_REASONABLE_DURATION &&
           durationSeconds <= MAX_REASONABLE_DURATION;
}

TimingCorrectionResult AnimationTimingCorrector::ValidateTimingCorrection(
    const XAnimationSet& original, const XAnimationSet& corrected) const {

    TimingCorrectionResult result;

    result.originalDurationSeconds = original.duration / original.ticksPerSecond;
    result.correctedDurationSeconds = corrected.duration / corrected.ticksPerSecond;
    result.timingErrorSeconds = std::abs(result.originalDurationSeconds - result.correctedDurationSeconds);
    result.detectedTicksPerSecond = corrected.ticksPerSecond;

    result.isValid = ValidateAnimationDuration(result.correctedDurationSeconds) &&
                     result.timingErrorSeconds <= TIMING_TOLERANCE;

    if (!result.isValid) {
        std::stringstream ss;
        ss << "Validation failed: ";
        if (!ValidateAnimationDuration(result.correctedDurationSeconds)) {
            ss << "Invalid duration (" << result.correctedDurationSeconds << "s) ";
        }
        if (result.timingErrorSeconds > TIMING_TOLERANCE) {
            ss << "Large timing error (" << result.timingErrorSeconds << "s)";
        }
        result.errorDescription = ss.str();
    }

    return result;
}

std::vector<float> AnimationTimingCorrector::GetCandidateTickRates(const XAnimationSet& animation) const {
    std::vector<float> candidates = COMMON_TICK_RATES;

    // Add analysis-based candidates
    if (!animation.keyframes.empty()) {
        float keyframeBasedRate = DetectTicksPerSecondFromKeyframes(animation);
        if (std::find(candidates.begin(), candidates.end(), keyframeBasedRate) == candidates.end()) {
            candidates.push_back(keyframeBasedRate);
        }
    }

    // Sort by likelihood (common rates first)
    std::sort(candidates.begin(), candidates.end(), [](float a, float b) {
        // Prioritize common DirectX rates
        if (std::abs(a - 4800.0f) < 0.1f) return true;
        if (std::abs(b - 4800.0f) < 0.1f) return false;
        if (std::abs(a - 160.0f) < 0.1f) return true;
        if (std::abs(b - 160.0f) < 0.1f) return false;
        return a < b;
    });

    return candidates;
}

float AnimationTimingCorrector::CalculateConfidence(float tickRate, const XAnimationSet& animation) const {
    return ScoreTickRate(tickRate, animation);
}

std::vector<XKeyframe> AnimationTimingCorrector::ConvertKeyframeTiming(
    const std::vector<XKeyframe>& originalKeyframes,
    float originalTicksPerSecond,
    float targetTicksPerSecond) const {

    std::vector<XKeyframe> convertedKeyframes = originalKeyframes;

    if (std::abs(originalTicksPerSecond - targetTicksPerSecond) > 0.1f) {
        float timeScale = originalTicksPerSecond / targetTicksPerSecond;

        for (auto& keyframe : convertedKeyframes) {
            keyframe.time *= timeScale;
        }
    }

    return convertedKeyframes;
}

std::vector<TimingCorrectionResult> AnimationTimingCorrector::CorrectAllAnimations(
    std::vector<XAnimationSet>& animations) const {

    std::vector<TimingCorrectionResult> results;
    results.reserve(animations.size());

    LOG_INFO("Correcting timing for " + std::to_string(animations.size()) + " animations");

    for (size_t i = 0; i < animations.size(); i++) {
        logger_.LogProgress("Animation timing correction", static_cast<int>(i + 1),
                           static_cast<int>(animations.size()));

        TimingCorrectionResult result = CorrectAnimationTiming(animations[i]);
        results.push_back(result);

        if (!result.isValid) {
            LOG_ERROR("Failed to correct timing for animation '" + animations[i].name +
                     "': " + result.errorDescription);
        }
    }

    return results;
}

void AnimationTimingCorrector::GenerateTimingReport(const std::vector<TimingCorrectionResult>& results) const {
    LOG_INFO("=== TIMING CORRECTION REPORT ===");

    int successCount = 0;
    int failureCount = 0;
    float totalTimingError = 0.0f;

    for (const auto& result : results) {
        if (result.isValid) {
            successCount++;
        } else {
            failureCount++;
        }
        totalTimingError += result.timingErrorSeconds;
    }

    LOG_INFO("Successfully corrected: " + std::to_string(successCount) + " animations");
    if (failureCount > 0) {
        LOG_ERROR("Failed to correct: " + std::to_string(failureCount) + " animations");
    }

    if (!results.empty()) {
        float averageError = totalTimingError / results.size();
        LOG_INFO("Average timing error: " + std::to_string(averageError) + " seconds");
    }

    // Detailed results for failures
    for (size_t i = 0; i < results.size(); i++) {
        const auto& result = results[i];
        if (!result.isValid) {
            LOG_ERROR("Animation " + std::to_string(i) + " timing correction failed: " +
                     result.errorDescription);
        }
    }
}

// Private helper methods
bool AnimationTimingCorrector::IsReasonableDuration(float durationSeconds) const {
    return ValidateAnimationDuration(durationSeconds);
}

float AnimationTimingCorrector::ScoreTickRate(float tickRate, const XAnimationSet& animation) const {
    if (animation.duration <= 0) {
        return 0.0f;
    }

    float durationSeconds = animation.duration / tickRate;

    // Score based on duration reasonableness
    float durationScore = 0.0f;
    if (IsReasonableDuration(durationSeconds)) {
        // Prefer durations in common ranges
        if (durationSeconds >= 0.5f && durationSeconds <= 60.0f) {
            durationScore = 1.0f;
        } else if (durationSeconds >= 0.1f && durationSeconds <= 300.0f) {
            durationScore = 0.7f;
        } else {
            durationScore = 0.3f;
        }
    }

    // Bonus for common tick rates
    float commonRateBonus = 0.0f;
    if (std::abs(tickRate - 4800.0f) < 0.1f) commonRateBonus = 0.3f;
    else if (std::abs(tickRate - 160.0f) < 0.1f) commonRateBonus = 0.2f;
    else if (std::abs(tickRate - 1000.0f) < 0.1f) commonRateBonus = 0.1f;

    return std::min(1.0f, durationScore + commonRateBonus);
}

std::string AnimationTimingCorrector::GetDetectionMethodDescription(float tickRate,
                                                                   const XAnimationSet& animation) const {
    std::stringstream ss;
    ss << "Detected rate " << tickRate << " ticks/sec";

    if (std::abs(tickRate - 4800.0f) < 0.1f) {
        ss << " (DirectX default)";
    } else if (std::abs(tickRate - animation.ticksPerSecond) < 0.1f) {
        ss << " (from animation header)";
    } else {
        ss << " (from duration analysis)";
    }

    return ss.str();
}

std::vector<float> AnimationTimingCorrector::ExtractKeyframeTimes(const XAnimationSet& animation) const {
    std::vector<float> times;
    times.reserve(animation.keyframes.size());

    for (const auto& keyframe : animation.keyframes) {
        times.push_back(keyframe.time);
    }

    return times;
}

float AnimationTimingCorrector::AnalyzeKeyframePattern(const std::vector<float>& keyframeTimes) const {
    if (keyframeTimes.size() < 2) {
        return 4800.0f;
    }

    // Calculate intervals between keyframes
    std::vector<float> intervals;
    for (size_t i = 1; i < keyframeTimes.size(); i++) {
        intervals.push_back(keyframeTimes[i] - keyframeTimes[i-1]);
    }

    // Find common intervals (suggesting frame-based timing)
    if (!intervals.empty()) {
        float medianInterval = CalculateMedian(intervals);

        // If median interval suggests frame-based timing
        for (float fps : {24.0f, 25.0f, 30.0f, 60.0f}) {
            float expectedInterval = 4800.0f / fps; // Assuming 4800 base rate
            if (std::abs(medianInterval - expectedInterval) < expectedInterval * 0.1f) {
                return 4800.0f;
            }
        }
    }

    return 4800.0f; // Default fallback
}

float AnimationTimingCorrector::CalculateMedian(std::vector<float> values) const {
    if (values.empty()) return 0.0f;

    std::sort(values.begin(), values.end());
    size_t middle = values.size() / 2;

    if (values.size() % 2 == 0) {
        return (values[middle - 1] + values[middle]) / 2.0f;
    } else {
        return values[middle];
    }
}

// TimingUtils namespace implementation
namespace TimingUtils {
    const float DIRECTX_DEFAULT_TICKS = 4800.0f;
    const float DIRECTX_ALT_TICKS_1 = 160.0f;
    const float DIRECTX_ALT_TICKS_2 = 1000.0f;
    const float DIRECTX_ALT_TICKS_3 = 2400.0f;
    const float DIRECTX_ALT_TICKS_4 = 9600.0f;

    double XTicksToSeconds(float xTicks, float ticksPerSecond) {
        return ticksPerSecond > 0 ? static_cast<double>(xTicks) / ticksPerSecond : 0.0;
    }

    float SecondsToXTicks(double seconds, float ticksPerSecond) {
        return static_cast<float>(seconds * ticksPerSecond);
    }

    bool IsValidTickRate(float tickRate) {
        return tickRate > 0 && tickRate <= 1000000.0f; // Reasonable upper bound
    }

    bool IsValidDuration(float durationSeconds) {
        return durationSeconds > 0 && durationSeconds <= 3600.0f; // Up to 1 hour
    }
}

} // namespace X2FBX
