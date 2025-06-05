#pragma once

#include "XFileData.h"
#include "Logger.h"
#include <fstream>
#include <memory>
#include <regex>

namespace X2FBX {

// Parser states
enum class ParseState {
    HEADER,
    TEMPLATE_DEFINITIONS,
    DATA_OBJECTS,
    FINISHED,
    ERROR
};

// Data object types found in .x files
enum class XDataObjectType {
    MESH,
    FRAME,
    ANIMATION_SET,
    ANIMATION,
    ANIMATION_KEY,
    MATERIAL,
    TEXTURE_FILENAME,
    MESH_MATERIAL_LIST,
    MESH_NORMALS,
    MESH_TEXTURE_COORDS,
    SKIN_MESH_HEADER,
    SKIN_WEIGHTS,
    UNKNOWN
};

// Template for parsing .x file data objects
struct XDataObject {
    XDataObjectType type;
    std::string name;
    std::string guid;
    std::vector<uint8_t> data;
    std::vector<std::shared_ptr<XDataObject>> children;
    std::map<std::string, std::string> properties;

    XDataObject() : type(XDataObjectType::UNKNOWN) {}
};

class XFileParser {
private:
    Logger& logger_;
    std::unique_ptr<std::ifstream> fileStream_;
    std::string currentLine_;
    size_t lineNumber_;
    ParseState currentState_;

    // Parsed data
    XFileData parsedData_;
    std::vector<std::shared_ptr<XDataObject>> dataObjects_;

    // Parser configuration
    bool strictMode_;
    bool verboseLogging_;

public:
    XFileParser();
    ~XFileParser();

    // Main parsing methods
    bool ParseFile(const std::string& filepath);
    bool ParseFromString(const std::string& content);

    // Access parsed data
    const XFileData& GetParsedData() const { return parsedData_; }
    XFileData TakeParsedData() { return std::move(parsedData_); }

    // Configuration
    void SetStrictMode(bool strict) { strictMode_ = strict; }
    void SetVerboseLogging(bool verbose) { verboseLogging_ = verbose; }

    // Utility methods
    static bool IsValidXFile(const std::string& filepath);
    static XFileHeader::Format DetectFileFormat(const std::string& filepath);

private:
    // Core parsing methods
    bool ParseHeader(const std::string& content);
    bool ParseDataObjects(const std::string& content);
    std::shared_ptr<XDataObject> ParseDataObject(std::istream& stream);

    // Specific object parsers
    bool ParseMesh(const XDataObject& meshObject);
    bool ParseFrame(const XDataObject& frameObject);
    bool ParseAnimationSet(const XDataObject& animSetObject);
    bool ParseAnimation(const XDataObject& animObject, XAnimationSet& animSet);
    bool ParseAnimationKey(const XDataObject& keyObject, XAnimationSet& animSet);
    bool ParseMaterial(const XDataObject& materialObject);
    bool ParseSkinWeights(const XDataObject& skinObject);

    // New stream-based parsing methods
    bool ParseMeshObject(std::stringstream& stream);
    bool ParseFrameObject(std::stringstream& stream);
    bool ParseAnimationSetObject(std::stringstream& stream);
    bool ParseAnimationObject(std::stringstream& stream, XAnimationSet& animSet);
    bool ParseAnimationKeyObject(std::stringstream& stream, XAnimationSet& animSet);
    bool ParseMaterialObject(std::stringstream& stream);

    // Nested mesh object parsers
    bool ParseMeshMaterialList(std::stringstream& stream);
    bool ParseMeshNormals(std::stringstream& stream);
    bool ParseMeshTextureCoords(std::stringstream& stream);
    bool ParseSkinMeshHeader(std::stringstream& stream);
    bool ParseSkinWeights(std::stringstream& stream);

    // Helper parsing methods
    XVertex ParseVertexLine(const std::string& line);
    XFace ParseFaceLine(const std::string& line);
    XKeyframe ParseKeyframeLine(const std::string& line, int keyType);
    std::string ParseStringLine(const std::string& line);
    bool SkipToClosingBrace(std::stringstream& stream);

    // Data extraction methods
    std::vector<XVector3> ParseVector3Array(const std::string& data);
    std::vector<XVector2> ParseVector2Array(const std::string& data);
    std::vector<XFace> ParseFaceArray(const std::string& data);
    std::vector<float> ParseFloatArray(const std::string& data);
    std::vector<int> ParseIntArray(const std::string& data);
    XMatrix4x4 ParseMatrix(const std::string& data);
    XQuaternion ParseQuaternion(const std::string& data);

    // Timing extraction
    bool ExtractTimingInformation();
    bool ParseAnimTicksPerSecond(const std::string& content);
    bool ParseFrameRate(const std::string& content);

    // Validation and error handling
    bool ValidateParsedData();
    void AddParseError(const std::string& error);
    void AddParseWarning(const std::string& warning);

    // Utility methods
    std::string TrimWhitespace(const std::string& str);
    std::vector<std::string> SplitString(const std::string& str, char delimiter);
    bool IsNumeric(const std::string& str);
    XDataObjectType StringToDataObjectType(const std::string& typeName);
    std::string DataObjectTypeToString(XDataObjectType type);

    // File format specific parsers
    bool ParseTextFormat(const std::string& content);
    bool ParseBinaryFormat(const std::string& content);

    // Template handling
    bool ParseTemplateDefinitions(const std::string& content);
    std::map<std::string, std::string> templates_;

    // Animation-specific parsing helpers
    void ProcessAnimationHierarchy();
    void LinkAnimationsToSkeleton();
    bool ValidateAnimationConsistency();

    // Bone/skeleton processing
    void BuildSkeletonHierarchy();
    void CalculateBindPoses();
    void ValidateSkinWeights();

    // Error recovery
    bool AttemptErrorRecovery();
    void ResetParserState();

    // Progress reporting
    void ReportProgress(const std::string& operation, float percentage);
};

// Utility functions for .x file handling
namespace XFileUtils {
    // File validation
    bool ValidateXFileSignature(const std::string& filepath);
    std::string ReadFileSignature(const std::string& filepath);

    // Format detection
    bool IsTextFormat(const std::string& content);
    bool IsBinaryFormat(const std::string& content);
    bool IsCompressedFormat(const std::string& content);

    // Content preprocessing
    std::string PreprocessTextContent(const std::string& content);
    std::string RemoveComments(const std::string& content);
    std::string NormalizeWhitespace(const std::string& content);

    // Data parsing helpers
    float ParseFloat(const std::string& str, bool& success);
    int ParseInt(const std::string& str, bool& success);
    bool ParseBool(const std::string& str);

    // Animation timing detection
    std::vector<std::string> FindTimingKeywords(const std::string& content);
    bool ExtractTicksPerSecond(const std::string& content, float& ticksPerSecond);

    // Coordinate system detection
    enum class CoordinateSystem {
        LEFT_HANDED,    // DirectX default
        RIGHT_HANDED,   // OpenGL/FBX
        UNKNOWN
    };

    CoordinateSystem DetectCoordinateSystem(const XMeshData& meshData);

    // Common .x file templates and GUIDs
    extern const std::map<std::string, XDataObjectType> STANDARD_TEMPLATES;
    extern const std::map<std::string, std::string> TEMPLATE_GUIDS;
}

} // namespace X2FBX
