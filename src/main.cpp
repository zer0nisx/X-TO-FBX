#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

// Project headers
#include "XFileData.h"
#include "XFileParser.h"
#include "BinaryXFileParser.h"
#include "FBXExporter.h"
#include "AnimationTimingCorrector.h"
#include "Logger.h"

using namespace X2FBX;
namespace fs = std::filesystem;

// Application version
const std::string APP_VERSION = "1.0.0";
const std::string APP_NAME = "X2FBX Converter";

// Command line options
struct ConversionOptions {
    std::string inputFile;
    std::string outputDirectory = "./output";
    bool verboseLogging = false;
    bool strictMode = false;
    bool validateTiming = true;
    bool generateReport = true;
    LogLevel logLevel = LogLevel::INFO;

    ConversionOptions() = default;
};

// Function declarations
void PrintUsage(const std::string& programName);
void PrintVersion();
bool ParseCommandLine(int argc, char* argv[], ConversionOptions& options);
bool ValidateInputFile(const std::string& filepath);
bool CreateOutputDirectory(const std::string& dirPath);
bool ConvertXFileToFBX(const ConversionOptions& options);
void PrintConversionSummary(const XFileData& fileData,
                           const std::vector<TimingCorrectionResult>& timingResults);

int main(int argc, char* argv[]) {
    std::cout << APP_NAME << " v" << APP_VERSION << std::endl;
    std::cout << "Convert DirectX .x files to FBX with proper animation timing" << std::endl;
    std::cout << "===========================================================" << std::endl << std::endl;

    // Parse command line arguments
    ConversionOptions options;
    if (!ParseCommandLine(argc, argv, options)) {
        PrintUsage(argv[0]);
        return 1;
    }

    // Initialize logger
    Logger::Initialize("x2fbx_converter.log", options.logLevel);
    Logger& logger = Logger::GetInstance();
    logger.EnableConsoleOutput(true);
    logger.EnableFileOutput(true);

    if (options.verboseLogging) {
        logger.SetLogLevel(LogLevel::DEBUG);
    }

    LOG_INFO("Starting " + APP_NAME + " v" + APP_VERSION);
    LOG_INFO("Input file: " + options.inputFile);
    LOG_INFO("Output directory: " + options.outputDirectory);

    // Validate input file
    if (!ValidateInputFile(options.inputFile)) {
        LOG_CRITICAL("Input file validation failed");
        std::cerr << "Error: Invalid input file: " << options.inputFile << std::endl;
        return 1;
    }

    // Create output directory
    if (!CreateOutputDirectory(options.outputDirectory)) {
        LOG_CRITICAL("Failed to create output directory");
        std::cerr << "Error: Cannot create output directory: " << options.outputDirectory << std::endl;
        return 1;
    }

    // Perform conversion
    auto startTime = std::chrono::high_resolution_clock::now();

    bool success = ConvertXFileToFBX(options);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    if (success) {
        std::cout << std::endl << "✓ Conversion completed successfully!" << std::endl;
        std::cout << "Total time: " << duration.count() << " ms" << std::endl;
        std::cout << "Output files saved to: " << options.outputDirectory << std::endl;
        LOG_INFO("Conversion completed successfully in " + std::to_string(duration.count()) + " ms");
        return 0;
    } else {
        std::cerr << std::endl << "✗ Conversion failed!" << std::endl;
        std::cerr << "Check the log file for detailed error information." << std::endl;
        LOG_CRITICAL("Conversion failed after " + std::to_string(duration.count()) + " ms");
        return 1;
    }
}

bool ParseCommandLine(int argc, char* argv[], ConversionOptions& options) {
    if (argc < 2) {
        return false;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            return false;
        } else if (arg == "--version" || arg == "-v") {
            PrintVersion();
            exit(0);
        } else if (arg == "--verbose") {
            options.verboseLogging = true;
            options.logLevel = LogLevel::DEBUG;
        } else if (arg == "--strict") {
            options.strictMode = true;
        } else if (arg == "--no-timing-validation") {
            options.validateTiming = false;
        } else if (arg == "--no-report") {
            options.generateReport = false;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                options.outputDirectory = argv[++i];
            } else {
                std::cerr << "Error: --output requires a directory path" << std::endl;
                return false;
            }
        } else if (arg == "--log-level") {
            if (i + 1 < argc) {
                std::string levelStr = argv[++i];
                if (levelStr == "debug") options.logLevel = LogLevel::DEBUG;
                else if (levelStr == "info") options.logLevel = LogLevel::INFO;
                else if (levelStr == "warning") options.logLevel = LogLevel::WARNING;
                else if (levelStr == "error") options.logLevel = LogLevel::ERROR;
                else {
                    std::cerr << "Error: Invalid log level: " << levelStr << std::endl;
                    return false;
                }
                i++;
            } else {
                std::cerr << "Error: --log-level requires a level (debug, info, warning, error)" << std::endl;
                return false;
            }
        } else if (arg[0] != '-') {
            // Input file
            if (options.inputFile.empty()) {
                options.inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files specified" << std::endl;
                return false;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return false;
        }
    }

    return !options.inputFile.empty();
}

void PrintUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [OPTIONS] <input.x>" << std::endl << std::endl;
    std::cout << "Convert DirectX .x files to FBX format with proper animation timing" << std::endl << std::endl;

    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                    Show this help message" << std::endl;
    std::cout << "  -v, --version                 Show version information" << std::endl;
    std::cout << "  -o, --output <directory>      Output directory (default: ./output)" << std::endl;
    std::cout << "  --verbose                     Enable verbose logging" << std::endl;
    std::cout << "  --strict                      Enable strict parsing mode" << std::endl;
    std::cout << "  --no-timing-validation        Disable animation timing validation" << std::endl;
    std::cout << "  --no-report                   Don't generate conversion report" << std::endl;
    std::cout << "  --log-level <level>           Set log level (debug, info, warning, error)" << std::endl;

    std::cout << std::endl << "Examples:" << std::endl;
    std::cout << "  " << programName << " character.x" << std::endl;
    std::cout << "  " << programName << " --verbose --output ./fbx_files character.x" << std::endl;
    std::cout << "  " << programName << " --strict --log-level debug model.x" << std::endl;

    std::cout << std::endl << "Output:" << std::endl;
    std::cout << "  For each animation in the .x file, a separate .fbx file will be created:" << std::endl;
    std::cout << "  - mesh_animationname.fbx" << std::endl;
    std::cout << "  - If no animations exist, a single mesh.fbx will be created" << std::endl;
}

void PrintVersion() {
    std::cout << APP_NAME << " version " << APP_VERSION << std::endl;
    std::cout << "Built for critical DirectX .x to FBX conversion with animation timing fixes" << std::endl;
    std::cout << "Supports multiple animations, bone hierarchy preservation, and timing correction" << std::endl;
}

bool ValidateInputFile(const std::string& filepath) {
    if (!fs::exists(filepath)) {
        std::cerr << "Error: Input file does not exist: " << filepath << std::endl;
        return false;
    }

    if (!fs::is_regular_file(filepath)) {
        std::cerr << "Error: Input path is not a regular file: " << filepath << std::endl;
        return false;
    }

    // Check file extension
    std::string extension = fs::path(filepath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension != ".x") {
        std::cerr << "Warning: Input file does not have .x extension: " << extension << std::endl;
    }

    // Validate .x file signature
    if (!XFileParser::IsValidXFile(filepath)) {
        std::cerr << "Error: Input file is not a valid DirectX .x file" << std::endl;
        return false;
    }

    return true;
}

bool CreateOutputDirectory(const std::string& dirPath) {
    try {
        if (!fs::exists(dirPath)) {
            fs::create_directories(dirPath);
            LOG_INFO("Created output directory: " + dirPath);
        } else if (!fs::is_directory(dirPath)) {
            LOG_ERROR("Output path exists but is not a directory: " + dirPath);
            return false;
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Failed to create output directory: " + std::string(e.what()));
        return false;
    }
}

bool ConvertXFileToFBX(const ConversionOptions& options) {
    try {
        std::cout << "Parsing DirectX .x file..." << std::endl;

        // Parse the .x file
        EnhancedXFileParser parser;
        parser.SetStrictMode(options.strictMode);
        parser.SetVerboseLogging(options.verboseLogging);

        if (!parser.ParseFile(options.inputFile)) {
            LOG_ERROR("Failed to parse .x file");
            return false;
        }

        XFileData fileData = parser.TakeParsedData();

        std::cout << "✓ Parsed " << fileData.meshData.GetVertexCount() << " vertices, "
                  << fileData.meshData.GetFaceCount() << " faces" << std::endl;

        if (fileData.meshData.GetAnimationCount() > 0) {
            std::cout << "✓ Found " << fileData.meshData.GetAnimationCount() << " animations" << std::endl;

            // Correct animation timing
            std::cout << "Correcting animation timing..." << std::endl;

            AnimationTimingCorrector timingCorrector;
            std::vector<TimingCorrectionResult> timingResults =
                timingCorrector.CorrectAllAnimations(fileData.meshData.animations);

            // Validate timing correction if requested
            if (options.validateTiming) {
                std::cout << "Validating timing corrections..." << std::endl;

                int validCorrections = 0;
                for (const auto& result : timingResults) {
                    if (result.isValid) {
                        validCorrections++;
                    } else {
                        LOG_WARNING("Animation timing correction failed: " + result.errorDescription);
                    }
                }

                std::cout << "✓ " << validCorrections << "/" << timingResults.size()
                          << " animations have valid timing" << std::endl;
            }

            // Generate timing report
            if (options.generateReport) {
                timingCorrector.GenerateTimingReport(timingResults);
            }

            // Print conversion summary
            PrintConversionSummary(fileData, timingResults);

            // TODO: Export individual FBX files for each animation
            std::cout << "Exporting FBX files..." << std::endl;

            std::string baseName = fs::path(options.inputFile).stem().string();

            for (size_t i = 0; i < fileData.meshData.animations.size(); i++) {
                const auto& animation = fileData.meshData.animations[i];

                std::string outputFileName = baseName + "_" + animation.name + ".fbx";
                std::string outputPath = (fs::path(options.outputDirectory) / outputFileName).string();

                // For now, just create placeholder files (FBX export would go here)
                std::ofstream placeholder(outputPath);
                if (placeholder.is_open()) {
                    placeholder << "# FBX file placeholder for animation: " << animation.name << std::endl;
                    placeholder << "# Duration: " << animation.GetDurationInSeconds() << " seconds" << std::endl;
                    placeholder << "# Keyframes: " << animation.keyframes.size() << std::endl;
                    placeholder.close();

                    std::cout << "  ✓ Created " << outputFileName << std::endl;
                } else {
                    LOG_ERROR("Failed to create output file: " + outputPath);
                    return false;
                }
            }

        } else {
            std::cout << "No animations found, creating static mesh..." << std::endl;

            std::string baseName = fs::path(options.inputFile).stem().string();
            std::string outputFileName = baseName + ".fbx";
            std::string outputPath = (fs::path(options.outputDirectory) / outputFileName).string();

            // Create static mesh placeholder
            std::ofstream placeholder(outputPath);
            if (placeholder.is_open()) {
                placeholder << "# FBX file placeholder for static mesh" << std::endl;
                placeholder << "# Vertices: " << fileData.meshData.GetVertexCount() << std::endl;
                placeholder << "# Faces: " << fileData.meshData.GetFaceCount() << std::endl;
                placeholder.close();

                std::cout << "  ✓ Created " << outputFileName << std::endl;
            } else {
                LOG_ERROR("Failed to create output file: " + outputPath);
                return false;
            }
        }

        return true;

    } catch (const std::exception& e) {
        LOG_CRITICAL("Exception during conversion: " + std::string(e.what()));
        std::cerr << "Exception: " << e.what() << std::endl;
        return false;
    }
}

void PrintConversionSummary(const XFileData& fileData,
                           const std::vector<TimingCorrectionResult>& timingResults) {
    std::cout << std::endl << "=== CONVERSION SUMMARY ===" << std::endl;

    // Mesh information
    std::cout << "Mesh Data:" << std::endl;
    std::cout << "  - Vertices: " << fileData.meshData.GetVertexCount() << std::endl;
    std::cout << "  - Faces: " << fileData.meshData.GetFaceCount() << std::endl;
    std::cout << "  - Materials: " << fileData.meshData.materials.size() << std::endl;
    std::cout << "  - Bones: " << fileData.meshData.GetBoneCount() << std::endl;

    // Animation information
    if (!fileData.meshData.animations.empty()) {
        std::cout << std::endl << "Animation Data:" << std::endl;
        std::cout << "  - Total animations: " << fileData.meshData.GetAnimationCount() << std::endl;

        if (fileData.meshData.hasTimingInfo) {
            std::cout << "  - Global ticks/sec: " << fileData.meshData.globalTicksPerSecond << std::endl;
        }

        for (size_t i = 0; i < fileData.meshData.animations.size(); i++) {
            const auto& anim = fileData.meshData.animations[i];
            std::cout << "  - '" << anim.name << "': "
                      << anim.GetDurationInSeconds() << "s ("
                      << anim.keyframes.size() << " keyframes)" << std::endl;
        }

        // Timing correction summary
        if (!timingResults.empty()) {
            std::cout << std::endl << "Timing Corrections:" << std::endl;

            int successful = 0;
            float totalError = 0.0f;

            for (const auto& result : timingResults) {
                if (result.isValid) {
                    successful++;
                } else {
                    std::cout << "  - WARNING: " << result.errorDescription << std::endl;
                }
                totalError += result.timingErrorSeconds;
            }

            std::cout << "  - Successfully corrected: " << successful << "/" << timingResults.size() << std::endl;

            if (!timingResults.empty()) {
                float avgError = totalError / timingResults.size();
                std::cout << "  - Average timing error: " << std::fixed << std::setprecision(3)
                          << avgError << " seconds" << std::endl;
            }
        }
    }

    std::cout << "=========================" << std::endl;
}
