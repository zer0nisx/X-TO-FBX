#pragma once

#include "XFileData.h"
#include "XFileParser.h"
#include "Logger.h"
#include <vector>
#include <memory>
#include <fstream>

namespace X2FBX {

// Binary data reader utility
class BinaryReader {
private:
    const uint8_t* data_;
    size_t size_;
    size_t position_;
    bool littleEndian_;

public:
    BinaryReader(const uint8_t* data, size_t size, bool littleEndian = true);

    // Basic type reading
    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    int8_t ReadInt8();
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();
    float ReadFloat();
    double ReadDouble();

    // String reading
    std::string ReadString(size_t length);
    std::string ReadNullTerminatedString();
    std::string ReadLengthPrefixedString();

    // Array reading
    std::vector<uint8_t> ReadBytes(size_t count);
    std::vector<float> ReadFloatArray(size_t count);
    std::vector<uint32_t> ReadUInt32Array(size_t count);

    // Position management
    void Seek(size_t position);
    void Skip(size_t bytes);
    size_t GetPosition() const { return position_; }
    size_t GetSize() const { return size_; }
    size_t GetRemainingBytes() const { return size_ - position_; }
    bool IsAtEnd() const { return position_ >= size_; }

    // Peek without advancing position
    uint8_t PeekUInt8();
    uint32_t PeekUInt32();

    // Validation
    bool CanRead(size_t bytes) const;
};

// Compressed file decompressor
class XFileDecompressor {
private:
    Logger& logger_;

public:
    XFileDecompressor();

    // Decompression methods
    bool DecompressZipped(const std::vector<uint8_t>& compressedData,
                         std::vector<uint8_t>& decompressedData);
    bool DecompressBzip2(const std::vector<uint8_t>& compressedData,
                        std::vector<uint8_t>& decompressedData);
    bool DecompressRawDeflate(const std::vector<uint8_t>& input,
                             std::vector<uint8_t>& output);
    bool DecompressDirectXBzip(const std::vector<uint8_t>& input,
                              std::vector<uint8_t>& output);
    bool DecompressDirectXLZ(const std::vector<uint8_t>& input,
                            std::vector<uint8_t>& output);
    bool DecompressBzip0032(const std::vector<uint8_t>& input,
                           std::vector<uint8_t>& output);

    // Detection
    bool IsZipCompressed(const std::vector<uint8_t>& data);
    bool IsBzip2Compressed(const std::vector<uint8_t>& data);
    bool IsDirectXLZCompressed(const std::vector<uint8_t>& data);

    // Utility
    static bool IsCompressionSupported();

private:
    bool DecompressWithZlib(const std::vector<uint8_t>& input,
                           std::vector<uint8_t>& output);
    bool DecompressWithBzip2(const std::vector<uint8_t>& input,
                            std::vector<uint8_t>& output);

    // New helper methods for bzip0032 format
    bool TryMultipleDecompressionMethods(const std::vector<uint8_t>& data,
                                       std::vector<uint8_t>& output);
    bool TryZlibDecompression(const std::vector<uint8_t>& data,
                            std::vector<uint8_t>& output);
    bool TryDeflateWithParams(const std::vector<uint8_t>& data,
                            std::vector<uint8_t>& output,
                            size_t offset, int windowBits);
    bool TryLZ77Decompression(const std::vector<uint8_t>& data,
                            std::vector<uint8_t>& output);
    bool TryPatternBasedDecompression(const std::vector<uint8_t>& data,
                                   std::vector<uint8_t>& output);
    bool ValidateDecompressedContent(const std::vector<uint8_t>& data);
};

// Binary .x file parser
class BinaryXFileParser {
private:
    Logger& logger_;
    std::unique_ptr<BinaryReader> reader_;
    XFileData parsedData_;

    // Template system for binary parsing
    struct BinaryTemplate {
        std::string name;
        std::string guid;
        std::vector<std::string> members;
        bool isOpen;  // Can contain other templates

        BinaryTemplate() : isOpen(false) {}
    };

    std::map<std::string, BinaryTemplate> templates_;
    std::map<std::string, uint32_t> templateIds_;

public:
    BinaryXFileParser();
    ~BinaryXFileParser();

    // Main parsing methods
    bool ParseBinaryFile(const std::string& filepath);
    bool ParseBinaryData(const std::vector<uint8_t>& data);
    bool ParseCompressedFile(const std::string& filepath);

    // Access parsed data
    const XFileData& GetParsedData() const { return parsedData_; }
    XFileData TakeParsedData() { return std::move(parsedData_); }

    // Format detection
    static bool IsBinaryXFile(const std::string& filepath);
    static bool IsCompressedXFile(const std::string& filepath);

private:
    // Core parsing
    bool ParseBinaryHeader();
    bool ParseBinaryContent();

    // Template parsing
    bool ParseTemplateDefinitions();
    bool ParseTemplate(BinaryTemplate& templ);

    // Data object parsing
    bool ParseDataObjects();
    bool ParseDataObject();

    // Specific object parsers
    bool ParseBinaryMesh();
    bool ParseBinaryFrame();
    bool ParseBinaryAnimationSet();
    bool ParseBinaryAnimation();
    bool ParseBinaryAnimationKey();
    bool ParseBinaryMaterial();
    bool ParseBinarySkinWeights();

    // Binary data conversion
    XVector3 ReadVector3();
    XVector2 ReadVector2();
    XQuaternion ReadQuaternion();
    XMatrix4x4 ReadMatrix4x4();

    // Array parsers
    std::vector<XVector3> ReadVector3Array(uint32_t count);
    std::vector<XVector2> ReadVector2Array(uint32_t count);
    std::vector<uint32_t> ReadIndexArray(uint32_t count);
    std::vector<float> ReadFloatArray(uint32_t count);

    // Animation parsing helpers
    bool ParseKeyframeData(XAnimationSet& animSet);
    bool ParseBoneAnimation(const std::string& boneName, XAnimationSet& animSet);

    // Timing extraction for binary files
    bool ExtractBinaryTimingInformation();

    // Template system
    uint32_t GetTemplateId(const std::string& templateName);
    bool IsKnownTemplate(uint32_t templateId);
    std::string GetTemplateName(uint32_t templateId);

    // Validation
    bool ValidateBinaryData();
    void AddBinaryParseError(const std::string& error);
    void AddBinaryParseWarning(const std::string& warning);

    // Utility
    void ResetBinaryParser();
    std::string ReadBinaryString();
    bool SkipUnknownData(uint32_t size);
};

// Enhanced X File Parser that supports all formats
class EnhancedXFileParser {
private:
    Logger& logger_;
    XFileParser textParser_;
    BinaryXFileParser binaryParser_;
    XFileDecompressor decompressor_;

public:
    EnhancedXFileParser();
    ~EnhancedXFileParser();

    // Main parsing method that auto-detects format
    bool ParseFile(const std::string& filepath);
    bool ParseFromData(const std::vector<uint8_t>& data);

    // Get parsed data
    const XFileData& GetParsedData() const;
    XFileData TakeParsedData();

    // Format detection
    XFileHeader::Format DetectFileFormat(const std::string& filepath);
    XFileHeader::Format DetectDataFormat(const std::vector<uint8_t>& data);

    // Configuration
    void SetStrictMode(bool strict);
    void SetVerboseLogging(bool verbose);

private:
    // Format-specific parsing
    bool ParseTextFormat(const std::string& filepath);
    bool ParseBinaryFormat(const std::string& filepath);
    bool ParseCompressedFormat(const std::string& filepath);

    // Helper methods
    std::vector<uint8_t> ReadFileToBytes(const std::string& filepath);
    bool ValidateFileSignature(const std::vector<uint8_t>& data);
};

// Utility functions for binary .x file handling
namespace BinaryXFileUtils {
    // File format detection
    bool HasBinaryXFileSignature(const std::vector<uint8_t>& data);
    bool HasCompressedXFileSignature(const std::vector<uint8_t>& data);

    // Endianness detection and conversion
    bool IsLittleEndian();
    uint16_t SwapBytes16(uint16_t value);
    uint32_t SwapBytes32(uint32_t value);
    uint64_t SwapBytes64(uint64_t value);
    float SwapBytesFloat(float value);

    // Binary data validation
    bool ValidateBinaryHeader(const std::vector<uint8_t>& data);
    bool ValidateCompressedHeader(const std::vector<uint8_t>& data);

    // Template GUID definitions (binary .x files use GUIDs instead of names)
    extern const std::map<std::string, std::string> STANDARD_TEMPLATE_GUIDS;

    // Common binary .x file constants
    extern const uint32_t BINARY_TOKEN_NAME;
    extern const uint32_t BINARY_TOKEN_STRING;
    extern const uint32_t BINARY_TOKEN_INTEGER;
    extern const uint32_t BINARY_TOKEN_FLOAT;
    extern const uint32_t BINARY_TOKEN_OBRACE;
    extern const uint32_t BINARY_TOKEN_CBRACE;
    extern const uint32_t BINARY_TOKEN_OPAREN;
    extern const uint32_t BINARY_TOKEN_CPAREN;
    extern const uint32_t BINARY_TOKEN_OBRACKET;
    extern const uint32_t BINARY_TOKEN_CBRACKET;
    extern const uint32_t BINARY_TOKEN_OANGLE;
    extern const uint32_t BINARY_TOKEN_CANGLE;
    extern const uint32_t BINARY_TOKEN_DOT;
    extern const uint32_t BINARY_TOKEN_COMMA;
    extern const uint32_t BINARY_TOKEN_SEMICOLON;
    extern const uint32_t BINARY_TOKEN_TEMPLATE;
    extern const uint32_t BINARY_TOKEN_WORD;
    extern const uint32_t BINARY_TOKEN_DWORD;
    extern const uint32_t BINARY_TOKEN_FLOAT_TOKEN;
    extern const uint32_t BINARY_TOKEN_DOUBLE;
    extern const uint32_t BINARY_TOKEN_CHAR;
    extern const uint32_t BINARY_TOKEN_UCHAR;
    extern const uint32_t BINARY_TOKEN_SWORD;
    extern const uint32_t BINARY_TOKEN_SDWORD;
    extern const uint32_t BINARY_TOKEN_VOID;
    extern const uint32_t BINARY_TOKEN_LPSTR;
    extern const uint32_t BINARY_TOKEN_UNICODE;
    extern const uint32_t BINARY_TOKEN_CSTRING;
    extern const uint32_t BINARY_TOKEN_ARRAY;
}

} // namespace X2FBX
