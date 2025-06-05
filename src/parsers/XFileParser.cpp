#include "XFileParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace X2FBX {

// Standard .x file templates and GUIDs
const std::map<std::string, XDataObjectType> XFileUtils::STANDARD_TEMPLATES = {
    {"Mesh", XDataObjectType::MESH},
    {"Frame", XDataObjectType::FRAME},
    {"AnimationSet", XDataObjectType::ANIMATION_SET},
    {"Animation", XDataObjectType::ANIMATION},
    {"AnimationKey", XDataObjectType::ANIMATION_KEY},
    {"Material", XDataObjectType::MATERIAL},
    {"TextureFilename", XDataObjectType::TEXTURE_FILENAME},
    {"MeshMaterialList", XDataObjectType::MESH_MATERIAL_LIST},
    {"MeshNormals", XDataObjectType::MESH_NORMALS},
    {"MeshTextureCoords", XDataObjectType::MESH_TEXTURE_COORDS},
    {"XSkinMeshHeader", XDataObjectType::SKIN_MESH_HEADER},
    {"SkinWeights", XDataObjectType::SKIN_WEIGHTS}
};

XFileParser::XFileParser()
    : logger_(Logger::GetInstance())
    , lineNumber_(0)
    , currentState_(ParseState::HEADER)
    , strictMode_(false)
    , verboseLogging_(false) {
}

XFileParser::~XFileParser() {
    if (fileStream_ && fileStream_->is_open()) {
        fileStream_->close();
    }
}

bool XFileParser::ParseFile(const std::string& filepath) {
    TIME_OPERATION("XFileParser::ParseFile");

    LOG_INFO("Starting to parse .x file: " + filepath);

    // Reset parser state
    ResetParserState();

    // Validate file existence and format
    if (!IsValidXFile(filepath)) {
        AddParseError("Invalid or non-existent .x file: " + filepath);
        return false;
    }

    // Open file
    fileStream_ = std::make_unique<std::ifstream>(filepath, std::ios::binary);
    if (!fileStream_->is_open()) {
        AddParseError("Failed to open file: " + filepath);
        return false;
    }

    // Read entire file content
    std::stringstream buffer;
    buffer << fileStream_->rdbuf();
    std::string content = buffer.str();
    fileStream_->close();

    LOG_INFO("File loaded, size: " + std::to_string(content.size()) + " bytes");

    return ParseFromString(content);
}

bool XFileParser::ParseFromString(const std::string& content) {
    TIME_OPERATION("XFileParser::ParseFromString");

    if (content.empty()) {
        AddParseError("Empty file content");
        return false;
    }

    try {
        // Parse header
        if (!ParseHeader(content)) {
            AddParseError("Failed to parse file header");
            return false;
        }

        // Detect and parse based on format
        bool parseSuccess = false;
        switch (parsedData_.header.format) {
            case XFileHeader::Format::TEXT:
                parseSuccess = ParseTextFormat(content);
                break;
            case XFileHeader::Format::BINARY:
                parseSuccess = ParseBinaryFormat(content);
                break;
            case XFileHeader::Format::COMPRESSED:
                AddParseError("Compressed .x files are not supported yet");
                return false;
            default:
                AddParseError("Unknown file format");
                return false;
        }

        if (!parseSuccess) {
            return false;
        }

        // Extract timing information
        ExtractTimingInformation();

        // Post-processing
        BuildSkeletonHierarchy();
        ProcessAnimationHierarchy();

        // Validate parsed data
        if (!ValidateParsedData()) {
            AddParseError("Parsed data validation failed");
            return false;
        }

        parsedData_.parseSuccessful = true;
        LOG_INFO("Successfully parsed .x file");

        // Report statistics
        LOG_INFO("Parsed: " + std::to_string(parsedData_.meshData.GetVertexCount()) + " vertices, " +
                std::to_string(parsedData_.meshData.GetFaceCount()) + " faces, " +
                std::to_string(parsedData_.meshData.GetBoneCount()) + " bones, " +
                std::to_string(parsedData_.meshData.GetAnimationCount()) + " animations");

        return true;

    } catch (const std::exception& e) {
        AddParseError("Exception during parsing: " + std::string(e.what()));
        return false;
    }
}

bool XFileParser::ParseHeader(const std::string& content) {
    if (content.size() < 16) {
        return false;
    }

    // Check magic signature "xof "
    if (content.substr(0, 4) != "xof ") {
        return false;
    }

    // Parse version (4 bytes)
    std::string versionStr = content.substr(4, 4);
    if (versionStr.size() >= 4) {
        parsedData_.header.majorVersion = versionStr[0] - '0';
        parsedData_.header.minorVersion = versionStr[2] - '0';
    }

    // Parse format (4 bytes) - should be "txt " for text format
    std::string formatStr = content.substr(8, 4);
    if (formatStr == "txt ") {
        parsedData_.header.format = XFileHeader::Format::TEXT;
    } else if (formatStr == "bin ") {
        parsedData_.header.format = XFileHeader::Format::BINARY;
    } else if (formatStr == "tzip" || formatStr == "bzip") {
        parsedData_.header.format = XFileHeader::Format::COMPRESSED;
    } else {
        LOG_WARNING("Unknown format identifier: " + formatStr);
        parsedData_.header.format = XFileHeader::Format::TEXT; // Assume text
    }

    // Parse float size (4 bytes) - should be "0032" for 32-bit floats
    std::string floatSize = content.substr(12, 4);
    if (floatSize != "0032") {
        LOG_WARNING("Non-standard float size: " + floatSize);
    }

    LOG_INFO("Parsed header - Version: " + std::to_string(parsedData_.header.majorVersion) +
             "." + std::to_string(parsedData_.header.minorVersion) +
             ", Format: " + formatStr);

    return true;
}

bool XFileParser::ParseTextFormat(const std::string& content) {
    TIME_OPERATION("ParseTextFormat");

    // Preprocess content - remove comments and normalize whitespace
    std::string cleanContent = XFileUtils::PreprocessTextContent(content);

    // Skip header (first 16 bytes)
    std::string dataContent = cleanContent.substr(16);

    // Create string stream for parsing
    std::stringstream stream(dataContent);

    // Parse template definitions first
    if (!ParseTemplateDefinitions(dataContent)) {
        LOG_WARNING("Template parsing failed, continuing with standard templates");
    }

    // Parse data objects
    return ParseDataObjects(dataContent);
}

bool XFileParser::ParseBinaryFormat(const std::string& content) {
    (void)content;  // Suppress unused parameter warning
    AddParseError("Binary format parsing not implemented in XFileParser, use BinaryXFileParser");
    return false;
}

bool XFileParser::ParseTemplateDefinitions(const std::string& content) {
    // Simple template parsing - look for template definitions
    std::regex templateRegex(R"(template\s+(\w+)\s*\{[^}]*\})");
    std::sregex_iterator iter(content.begin(), content.end(), templateRegex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string templateName = (*iter)[1].str();
        templates_[templateName] = (*iter).str();
        LOG_DEBUG("Found template: " + templateName);
    }

    return true;
}

bool XFileParser::ParseDataObjects(const std::string& content) {
    TIME_OPERATION("ParseDataObjects");

    // Create a stream for parsing
    std::stringstream stream(content);
    std::string line;
    lineNumber_ = 0;

    while (std::getline(stream, line)) {
        lineNumber_++;
        line = TrimWhitespace(line);

        if (line.empty() || line[0] == '/' || line.substr(0, 8) == "template") {
            continue; // Skip empty lines, comments, and templates
        }

        // Check for data object start
        if (line.find('{') != std::string::npos) {
            std::string objectType = line.substr(0, line.find_first_of(" \t{"));
            objectType = TrimWhitespace(objectType);

            if (objectType == "Mesh") {
                if (!ParseMeshObject(stream)) {
                    LOG_ERROR("Failed to parse Mesh object at line " + std::to_string(lineNumber_));
                    return false;
                }
            } else if (objectType == "Frame") {
                if (!ParseFrameObject(stream)) {
                    LOG_ERROR("Failed to parse Frame object at line " + std::to_string(lineNumber_));
                    return false;
                }
            } else if (objectType == "AnimationSet") {
                if (!ParseAnimationSetObject(stream)) {
                    LOG_ERROR("Failed to parse AnimationSet object at line " + std::to_string(lineNumber_));
                    return false;
                }
            } else if (objectType == "Material") {
                if (!ParseMaterialObject(stream)) {
                    LOG_ERROR("Failed to parse Material object at line " + std::to_string(lineNumber_));
                    return false;
                }
            } else {
                LOG_DEBUG("Skipping unknown object type: " + objectType);
                SkipToClosingBrace(stream);
            }
        }
    }

    return true;
}

bool XFileParser::ParseMeshObject(std::stringstream& stream) {
    TIME_OPERATION("ParseMeshObject");

    std::string line;

    // Read vertex count
    if (!std::getline(stream, line)) return false;
    lineNumber_++;

    line = TrimWhitespace(line);
    size_t semicolon = line.find(';');
    if (semicolon != std::string::npos) {
        line = line.substr(0, semicolon);
    }

    int vertexCount = std::stoi(line);
    LOG_DEBUG("Parsing mesh with " + std::to_string(vertexCount) + " vertices");

    // Read vertices
    parsedData_.meshData.vertices.reserve(vertexCount);
    for (int i = 0; i < vertexCount; i++) {
        if (!std::getline(stream, line)) return false;
        lineNumber_++;

        XVertex vertex = ParseVertexLine(line);
        parsedData_.meshData.vertices.push_back(vertex);
    }

    // Read face count
    if (!std::getline(stream, line)) return false;
    lineNumber_++;

    line = TrimWhitespace(line);
    semicolon = line.find(';');
    if (semicolon != std::string::npos) {
        line = line.substr(0, semicolon);
    }

    int faceCount = std::stoi(line);
    LOG_DEBUG("Parsing " + std::to_string(faceCount) + " faces");

    // Read faces
    parsedData_.meshData.faces.reserve(faceCount);
    for (int i = 0; i < faceCount; i++) {
        if (!std::getline(stream, line)) return false;
        lineNumber_++;

        XFace face = ParseFaceLine(line);
        parsedData_.meshData.faces.push_back(face);
    }

    // Parse nested objects (materials, normals, texture coords, etc.)
    while (std::getline(stream, line)) {
        lineNumber_++;
        line = TrimWhitespace(line);

        if (line == "}") {
            break; // End of mesh object
        }

        if (line.find("MeshMaterialList") != std::string::npos) {
            ParseMeshMaterialList(stream);
        } else if (line.find("MeshNormals") != std::string::npos) {
            ParseMeshNormals(stream);
        } else if (line.find("MeshTextureCoords") != std::string::npos) {
            ParseMeshTextureCoords(stream);
        } else if (line.find("XSkinMeshHeader") != std::string::npos) {
            ParseSkinMeshHeader(stream);
        } else if (line.find("SkinWeights") != std::string::npos) {
            ParseSkinWeights(stream);
        }
    }

    return true;
}

bool XFileParser::ParseFrameObject(std::stringstream& stream) {
    // Skip frame objects for now - they typically contain transformation matrices
    // that aren't strictly necessary for basic mesh conversion
    return SkipToClosingBrace(stream);
}

bool XFileParser::ParseAnimationSetObject(std::stringstream& stream) {
    TIME_OPERATION("ParseAnimationSetObject");

    XAnimationSet animSet;
    animSet.name = "Animation_" + std::to_string(parsedData_.meshData.animations.size());

    std::string line;
    while (std::getline(stream, line)) {
        lineNumber_++;
        line = TrimWhitespace(line);

        if (line == "}") {
            break; // End of animation set
        }

        if (line.find("Animation") != std::string::npos && line.find('{') != std::string::npos) {
            ParseAnimationObject(stream, animSet);
        }
    }

    if (!animSet.keyframes.empty()) {
        parsedData_.meshData.animations.push_back(animSet);
        LOG_DEBUG("Parsed animation set: " + animSet.name + " with " +
                  std::to_string(animSet.keyframes.size()) + " keyframes");
    }

    return true;
}

bool XFileParser::ParseAnimationObject(std::stringstream& stream, XAnimationSet& animSet) {
    std::string line;
    while (std::getline(stream, line)) {
        lineNumber_++;
        line = TrimWhitespace(line);

        if (line == "}") {
            break; // End of animation object
        }

        if (line.find("AnimationKey") != std::string::npos && line.find('{') != std::string::npos) {
            ParseAnimationKeyObject(stream, animSet);
        }
    }

    return true;
}

bool XFileParser::ParseAnimationKeyObject(std::stringstream& stream, XAnimationSet& animSet) {
    std::string line;

    // Read key type
    if (!std::getline(stream, line)) return false;
    lineNumber_++;

    int keyType = std::stoi(TrimWhitespace(line.substr(0, line.find(';'))));

    // Read number of keys
    if (!std::getline(stream, line)) return false;
    lineNumber_++;

    int numKeys = std::stoi(TrimWhitespace(line.substr(0, line.find(';'))));

    // Read keyframes
    for (int i = 0; i < numKeys; i++) {
        if (!std::getline(stream, line)) return false;
        lineNumber_++;

        XKeyframe keyframe = ParseKeyframeLine(line, keyType);
        animSet.keyframes.push_back(keyframe);

        if (keyframe.time > animSet.duration) {
            animSet.duration = keyframe.time;
        }
    }

    // Skip to closing brace
    while (std::getline(stream, line)) {
        lineNumber_++;
        if (TrimWhitespace(line) == "}") break;
    }

    return true;
}

bool XFileParser::ParseMaterialObject(std::stringstream& stream) {
    XMaterial material;
    std::string line;

    // Read material properties
    if (!std::getline(stream, line)) return false;
    lineNumber_++;

    // Parse diffuse color (r,g,b,a)
    auto values = ParseFloatArray(line);
    if (values.size() >= 3) {
        material.diffuseColor = XVector3(values[0], values[1], values[2]);
    }

    // Read other material properties...
    while (std::getline(stream, line)) {
        lineNumber_++;
        line = TrimWhitespace(line);

        if (line == "}") {
            break;
        }

        if (line.find("TextureFilename") != std::string::npos) {
            // Parse texture filename
            if (std::getline(stream, line)) {
                lineNumber_++;
                material.diffuseTexture = ParseStringLine(line);
            }
        }
    }

    parsedData_.meshData.materials.push_back(material);
    return true;
}

// Helper parsing methods
XVertex XFileParser::ParseVertexLine(const std::string& line) {
    XVertex vertex;
    auto values = ParseFloatArray(line);

    if (values.size() >= 3) {
        vertex.position = XVector3(values[0], values[1], values[2]);
    }

    return vertex;
}

XFace XFileParser::ParseFaceLine(const std::string& line) {
    XFace face;
    auto values = ParseIntArray(line);

    if (values.size() >= 4) {
        // First value is vertex count (should be 3 for triangles)
        if (values[0] == 3 && values.size() >= 4) {
            face.indices[0] = values[1];
            face.indices[1] = values[2];
            face.indices[2] = values[3];
        }
    }

    return face;
}

XKeyframe XFileParser::ParseKeyframeLine(const std::string& line, int keyType) {
    XKeyframe keyframe;
    auto values = ParseFloatArray(line);

    if (values.size() >= 1) {
        keyframe.time = values[0];
    }

    // Parse based on key type
    switch (keyType) {
        case 0: // Rotation (quaternion)
            if (values.size() >= 5) {
                keyframe.rotation = XQuaternion(values[2], values[3], values[4], values[1]);
            }
            break;
        case 1: // Scale
            if (values.size() >= 4) {
                keyframe.scale = XVector3(values[1], values[2], values[3]);
            }
            break;
        case 2: // Position
            if (values.size() >= 4) {
                keyframe.position = XVector3(values[1], values[2], values[3]);
            }
            break;
    }

    return keyframe;
}

// Additional helper methods for nested mesh objects
bool XFileParser::ParseMeshMaterialList(std::stringstream& stream) {
    return SkipToClosingBrace(stream); // Basic implementation
}

bool XFileParser::ParseMeshNormals(std::stringstream& stream) {
    return SkipToClosingBrace(stream); // Basic implementation
}

bool XFileParser::ParseMeshTextureCoords(std::stringstream& stream) {
    return SkipToClosingBrace(stream); // Basic implementation
}

bool XFileParser::ParseSkinMeshHeader(std::stringstream& stream) {
    return SkipToClosingBrace(stream); // Basic implementation
}

bool XFileParser::ParseSkinWeights(std::stringstream& stream) {
    return SkipToClosingBrace(stream); // Basic implementation
}

bool XFileParser::SkipToClosingBrace(std::stringstream& stream) {
    std::string line;
    int braceCount = 1;

    while (std::getline(stream, line) && braceCount > 0) {
        lineNumber_++;
        for (char c : line) {
            if (c == '{') braceCount++;
            else if (c == '}') braceCount--;
        }
    }

    return true;
}

std::string XFileParser::ParseStringLine(const std::string& line) {
    std::string cleaned = TrimWhitespace(line);

    // Remove quotes and semicolon
    if (cleaned.front() == '"' && cleaned.back() == ';') {
        cleaned = cleaned.substr(1, cleaned.length() - 3);
    } else if (cleaned.front() == '"') {
        cleaned = cleaned.substr(1, cleaned.length() - 2);
    }

    return cleaned;
}

std::string XFileParser::TrimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> XFileParser::SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(TrimWhitespace(item));
    }

    return result;
}

XDataObjectType XFileParser::StringToDataObjectType(const std::string& typeName) {
    auto it = XFileUtils::STANDARD_TEMPLATES.find(typeName);
    return (it != XFileUtils::STANDARD_TEMPLATES.end()) ? it->second : XDataObjectType::UNKNOWN;
}

bool XFileParser::IsValidXFile(const std::string& filepath) {
    return XFileUtils::ValidateXFileSignature(filepath);
}

// Static validation methods
bool XFileUtils::ValidateXFileSignature(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    char signature[5] = {0};
    file.read(signature, 4);
    file.close();

    return std::string(signature) == "xof ";
}

bool XFileUtils::ExtractTicksPerSecond(const std::string& content, float& ticksPerSecond) {
    // Look for AnimTicksPerSecond
    std::regex ticksRegex(R"(AnimTicksPerSecond\s*\{\s*([0-9.]+)\s*[;}])");
    std::smatch match;

    if (std::regex_search(content, match, ticksRegex)) {
        bool success;
        ticksPerSecond = ParseFloat(match[1].str(), success);
        return success;
    }

    return false;
}

float XFileUtils::ParseFloat(const std::string& str, bool& success) {
    try {
        float value = std::stof(str);
        success = true;
        return value;
    } catch (...) {
        success = false;
        return 0.0f;
    }
}

int XFileUtils::ParseInt(const std::string& str, bool& success) {
    try {
        int value = std::stoi(str);
        success = true;
        return value;
    } catch (...) {
        success = false;
        return 0;
    }
}

std::string XFileUtils::PreprocessTextContent(const std::string& content) {
    std::string result = RemoveComments(content);
    result = NormalizeWhitespace(result);
    return result;
}

std::string XFileUtils::RemoveComments(const std::string& content) {
    std::string result;
    result.reserve(content.size());

    bool inLineComment = false;
    bool inBlockComment = false;
    bool inString = false;

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];
        char next = (i + 1 < content.size()) ? content[i + 1] : '\0';

        if (inString) {
            result += c;
            if (c == '"' && (i == 0 || content[i-1] != '\\')) {
                inString = false;
            }
        } else if (inLineComment) {
            if (c == '\n' || c == '\r') {
                inLineComment = false;
                result += c;
            }
        } else if (inBlockComment) {
            if (c == '*' && next == '/') {
                inBlockComment = false;
                i++; // Skip the '/'
            }
        } else {
            if (c == '"') {
                inString = true;
                result += c;
            } else if (c == '/' && next == '/') {
                inLineComment = true;
                i++; // Skip the second '/'
            } else if (c == '/' && next == '*') {
                inBlockComment = true;
                i++; // Skip the '*'
            } else {
                result += c;
            }
        }
    }

    return result;
}

std::string XFileUtils::NormalizeWhitespace(const std::string& content) {
    std::string result;
    result.reserve(content.size());

    bool lastWasSpace = false;
    for (char c : content) {
        if (std::isspace(c)) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }

    return result;
}

std::vector<float> XFileParser::ParseFloatArray(const std::string& data) {
    std::vector<float> result;
    std::string cleanData = TrimWhitespace(data);

    // Remove trailing semicolon and comma
    if (!cleanData.empty() && (cleanData.back() == ';' || cleanData.back() == ',')) {
        cleanData.pop_back();
    }

    std::stringstream ss(cleanData);
    std::string token;

    while (std::getline(ss, token, ';') || std::getline(ss, token, ',')) {
        token = TrimWhitespace(token);
        if (!token.empty()) {
            bool success;
            float value = XFileUtils::ParseFloat(token, success);
            if (success) {
                result.push_back(value);
            }
        }
    }

    return result;
}

std::vector<int> XFileParser::ParseIntArray(const std::string& data) {
    std::vector<int> result;
    std::string cleanData = TrimWhitespace(data);

    // Remove trailing semicolon and comma
    if (!cleanData.empty() && (cleanData.back() == ';' || cleanData.back() == ',')) {
        cleanData.pop_back();
    }

    std::stringstream ss(cleanData);
    std::string token;

    while (std::getline(ss, token, ';') || std::getline(ss, token, ',')) {
        token = TrimWhitespace(token);
        if (!token.empty()) {
            bool success;
            int value = XFileUtils::ParseInt(token, success);
            if (success) {
                result.push_back(value);
            }
        }
    }

    return result;
}

bool XFileParser::ExtractTimingInformation() {
    // Try to extract timing information from parsed animations
    float globalTicks = 0;
    bool foundTiming = false;

    // Check if any animation has explicit timing
    for (auto& anim : parsedData_.meshData.animations) {
        if (anim.ticksPerSecond > 0) {
            globalTicks = anim.ticksPerSecond;
            foundTiming = true;
            break;
        }
    }

    if (!foundTiming) {
        // Use default DirectX timing
        globalTicks = 4800.0f;
        LOG_INFO("Using default DirectX timing: 4800 ticks/sec");
    } else {
        LOG_INFO("Extracted timing: " + std::to_string(globalTicks) + " ticks/sec");
    }

    parsedData_.meshData.globalTicksPerSecond = globalTicks;
    parsedData_.meshData.hasTimingInfo = foundTiming;
    parsedData_.header.hasAnimationTimingInfo = foundTiming;
    parsedData_.header.ticksPerSecond = globalTicks;

    // Update all animations with consistent timing
    for (auto& anim : parsedData_.meshData.animations) {
        if (anim.ticksPerSecond <= 0) {
            anim.ticksPerSecond = globalTicks;
        }
    }

    return true;
}

void XFileParser::BuildSkeletonHierarchy() {
    // Basic skeleton building - link bones by parent names
    for (size_t i = 0; i < parsedData_.meshData.bones.size(); i++) {
        auto& bone = parsedData_.meshData.bones[i];

        if (!bone.parentName.empty()) {
            // Find parent bone
            for (size_t j = 0; j < parsedData_.meshData.bones.size(); j++) {
                if (parsedData_.meshData.bones[j].name == bone.parentName) {
                    bone.parentIndex = static_cast<int>(j);
                    parsedData_.meshData.bones[j].childIndices.push_back(static_cast<int>(i));
                    break;
                }
            }
        }
    }

    LOG_DEBUG("Built skeleton hierarchy with " + std::to_string(parsedData_.meshData.bones.size()) + " bones");
}

void XFileParser::ProcessAnimationHierarchy() {
    // Link animations to skeleton bones
    for (auto& anim : parsedData_.meshData.animations) {
        // Process bone-specific keyframes
        for (const auto& boneKF : anim.boneKeyframes) {
            const std::string& boneName = boneKF.first;

            // Find corresponding bone
            bool boneFound = false;
            for (const auto& bone : parsedData_.meshData.bones) {
                if (bone.name == boneName) {
                    boneFound = true;
                    break;
                }
            }

            if (!boneFound) {
                LOG_WARNING("Animation references unknown bone: " + boneName);
            }
        }
    }
}

bool XFileParser::ValidateParsedData() {
    std::vector<std::string> errors = parsedData_.meshData.GetValidationErrors();

    for (const auto& error : errors) {
        AddParseError(error);
    }

    // Additional validation for animations
    for (const auto& anim : parsedData_.meshData.animations) {
        if (anim.keyframes.empty() && anim.boneKeyframes.empty()) {
            AddParseWarning("Animation '" + anim.name + "' has no keyframes");
        }

        if (anim.duration <= 0) {
            AddParseWarning("Animation '" + anim.name + "' has zero or negative duration");
        }
    }

    return errors.empty();
}

void XFileParser::AddParseError(const std::string& error) {
    parsedData_.parseErrors.push_back(error);
    LOG_ERROR("Parse error at line " + std::to_string(lineNumber_) + ": " + error);
}

void XFileParser::AddParseWarning(const std::string& warning) {
    parsedData_.parseWarnings.push_back(warning);
    LOG_WARNING("Parse warning at line " + std::to_string(lineNumber_) + ": " + warning);
}

void XFileParser::ResetParserState() {
    lineNumber_ = 0;
    currentState_ = ParseState::HEADER;
    parsedData_ = XFileData();
    templates_.clear();
}

bool XFileParser::IsNumeric(const std::string& str) {
    if (str.empty()) return false;

    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') start = 1;

    bool hasDecimal = false;
    for (size_t i = start; i < str.size(); i++) {
        if (str[i] == '.') {
            if (hasDecimal) return false;
            hasDecimal = true;
        } else if (!std::isdigit(str[i])) {
            return false;
        }
    }

    return true;
}

std::string XFileParser::DataObjectTypeToString(XDataObjectType type) {
    switch (type) {
        case XDataObjectType::MESH: return "Mesh";
        case XDataObjectType::FRAME: return "Frame";
        case XDataObjectType::ANIMATION_SET: return "AnimationSet";
        case XDataObjectType::ANIMATION: return "Animation";
        case XDataObjectType::ANIMATION_KEY: return "AnimationKey";
        case XDataObjectType::MATERIAL: return "Material";
        case XDataObjectType::TEXTURE_FILENAME: return "TextureFilename";
        case XDataObjectType::MESH_MATERIAL_LIST: return "MeshMaterialList";
        case XDataObjectType::MESH_NORMALS: return "MeshNormals";
        case XDataObjectType::MESH_TEXTURE_COORDS: return "MeshTextureCoords";
        case XDataObjectType::SKIN_MESH_HEADER: return "XSkinMeshHeader";
        case XDataObjectType::SKIN_WEIGHTS: return "SkinWeights";
        default: return "Unknown";
    }
}

void XFileParser::ReportProgress(const std::string& operation, float percentage) {
    if (verboseLogging_) {
        LOG_DEBUG(operation + ": " + std::to_string(static_cast<int>(percentage)) + "%");
    }
}

} // namespace X2FBX
