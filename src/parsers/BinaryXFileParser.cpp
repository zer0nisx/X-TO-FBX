#include "BinaryXFileParser.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdio>

#ifdef HAVE_BZIP2
#include <bzlib.h>
#endif

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

namespace X2FBX {

// =============================================================================
// BinaryReader Implementation
// =============================================================================

BinaryReader::BinaryReader(const uint8_t* data, size_t size, bool littleEndian)
    : data_(data), size_(size), position_(0), littleEndian_(littleEndian) {
}

uint8_t BinaryReader::ReadUInt8() {
    if (position_ >= size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }
    return data_[position_++];
}

uint16_t BinaryReader::ReadUInt16() {
    if (position_ + 2 > size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }

    uint16_t value;
    if (littleEndian_) {
        value = data_[position_] | (data_[position_ + 1] << 8);
    } else {
        value = (data_[position_] << 8) | data_[position_ + 1];
    }
    position_ += 2;
    return value;
}

uint32_t BinaryReader::ReadUInt32() {
    if (position_ + 4 > size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }

    uint32_t value;
    if (littleEndian_) {
        value = data_[position_] |
                (data_[position_ + 1] << 8) |
                (data_[position_ + 2] << 16) |
                (data_[position_ + 3] << 24);
    } else {
        value = (data_[position_] << 24) |
                (data_[position_ + 1] << 16) |
                (data_[position_ + 2] << 8) |
                data_[position_ + 3];
    }
    position_ += 4;
    return value;
}

uint64_t BinaryReader::ReadUInt64() {
    if (position_ + 8 > size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }

    uint64_t value;
    if (littleEndian_) {
        value = static_cast<uint64_t>(data_[position_]) |
                (static_cast<uint64_t>(data_[position_ + 1]) << 8) |
                (static_cast<uint64_t>(data_[position_ + 2]) << 16) |
                (static_cast<uint64_t>(data_[position_ + 3]) << 24) |
                (static_cast<uint64_t>(data_[position_ + 4]) << 32) |
                (static_cast<uint64_t>(data_[position_ + 5]) << 40) |
                (static_cast<uint64_t>(data_[position_ + 6]) << 48) |
                (static_cast<uint64_t>(data_[position_ + 7]) << 56);
    } else {
        value = (static_cast<uint64_t>(data_[position_]) << 56) |
                (static_cast<uint64_t>(data_[position_ + 1]) << 48) |
                (static_cast<uint64_t>(data_[position_ + 2]) << 40) |
                (static_cast<uint64_t>(data_[position_ + 3]) << 32) |
                (static_cast<uint64_t>(data_[position_ + 4]) << 24) |
                (static_cast<uint64_t>(data_[position_ + 5]) << 16) |
                (static_cast<uint64_t>(data_[position_ + 6]) << 8) |
                static_cast<uint64_t>(data_[position_ + 7]);
    }
    position_ += 8;
    return value;
}

int8_t BinaryReader::ReadInt8() {
    return static_cast<int8_t>(ReadUInt8());
}

int16_t BinaryReader::ReadInt16() {
    return static_cast<int16_t>(ReadUInt16());
}

int32_t BinaryReader::ReadInt32() {
    return static_cast<int32_t>(ReadUInt32());
}

int64_t BinaryReader::ReadInt64() {
    return static_cast<int64_t>(ReadUInt64());
}

float BinaryReader::ReadFloat() {
    uint32_t intValue = ReadUInt32();
    float floatValue;
    std::memcpy(&floatValue, &intValue, sizeof(float));
    return floatValue;
}

double BinaryReader::ReadDouble() {
    uint64_t intValue = ReadUInt64();
    double doubleValue;
    std::memcpy(&doubleValue, &intValue, sizeof(double));
    return doubleValue;
}

std::string BinaryReader::ReadString(size_t length) {
    if (position_ + length > size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }

    std::string result(reinterpret_cast<const char*>(data_ + position_), length);
    position_ += length;
    return result;
}

std::string BinaryReader::ReadNullTerminatedString() {
    std::string result;
    while (position_ < size_ && data_[position_] != 0) {
        result += static_cast<char>(data_[position_++]);
    }
    if (position_ < size_) {
        position_++; // Skip null terminator
    }
    return result;
}

std::string BinaryReader::ReadLengthPrefixedString() {
    uint32_t length = ReadUInt32();
    return ReadString(length);
}

std::vector<uint8_t> BinaryReader::ReadBytes(size_t count) {
    if (position_ + count > size_) {
        throw std::runtime_error("BinaryReader: Read beyond end of data");
    }

    std::vector<uint8_t> result(data_ + position_, data_ + position_ + count);
    position_ += count;
    return result;
}

std::vector<float> BinaryReader::ReadFloatArray(size_t count) {
    std::vector<float> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(ReadFloat());
    }
    return result;
}

std::vector<uint32_t> BinaryReader::ReadUInt32Array(size_t count) {
    std::vector<uint32_t> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.push_back(ReadUInt32());
    }
    return result;
}

void BinaryReader::Seek(size_t position) {
    if (position > size_) {
        throw std::runtime_error("BinaryReader: Seek beyond end of data");
    }
    position_ = position;
}

void BinaryReader::Skip(size_t bytes) {
    if (position_ + bytes > size_) {
        throw std::runtime_error("BinaryReader: Skip beyond end of data");
    }
    position_ += bytes;
}

uint8_t BinaryReader::PeekUInt8() {
    if (position_ >= size_) {
        throw std::runtime_error("BinaryReader: Peek beyond end of data");
    }
    return data_[position_];
}

uint32_t BinaryReader::PeekUInt32() {
    if (position_ + 4 > size_) {
        throw std::runtime_error("BinaryReader: Peek beyond end of data");
    }

    uint32_t value;
    if (littleEndian_) {
        value = data_[position_] |
                (data_[position_ + 1] << 8) |
                (data_[position_ + 2] << 16) |
                (data_[position_ + 3] << 24);
    } else {
        value = (data_[position_] << 24) |
                (data_[position_ + 1] << 16) |
                (data_[position_ + 2] << 8) |
                data_[position_ + 3];
    }
    return value;
}

bool BinaryReader::CanRead(size_t bytes) const {
    return position_ + bytes <= size_;
}

// =============================================================================
// XFileDecompressor Implementation
// =============================================================================

XFileDecompressor::XFileDecompressor() : logger_(Logger::GetInstance()) {
}

bool XFileDecompressor::DecompressZipped(const std::vector<uint8_t>& compressedData,
                                        std::vector<uint8_t>& decompressedData) {
    // Placeholder implementation - would need zlib integration
    logger_.Warning("Zip decompression not implemented");
    return false;
}

bool XFileDecompressor::DecompressBzip2(const std::vector<uint8_t>& compressedData,
                                       std::vector<uint8_t>& decompressedData) {
#ifdef HAVE_BZIP2
    if (compressedData.empty()) {
        logger_.Error("BZip2: Empty input data");
        return false;
    }

    // Verify bzip2 signature
    if (!IsBzip2Compressed(compressedData)) {
        logger_.Error("BZip2: Invalid file signature");
        return false;
    }

    logger_.Info("Starting BZip2 decompression of " + std::to_string(compressedData.size()) + " bytes");

    // Initialize bzip2 decompression
    bz_stream bzStream;
    bzStream.bzalloc = nullptr;
    bzStream.bzfree = nullptr;
    bzStream.opaque = nullptr;
    bzStream.next_in = const_cast<char*>(reinterpret_cast<const char*>(compressedData.data()));
    bzStream.avail_in = static_cast<unsigned int>(compressedData.size());

    int bzError = BZ2_bzDecompressInit(&bzStream, 0, 0);
    if (bzError != BZ_OK) {
        logger_.Warning("BZip2: Failed to initialize decompression (error: " + std::to_string(bzError) + ") - trying deflate fallback");
        // Try deflate decompression as fallback - DirectX might be lying about compression type
        return DecompressRawDeflate(compressedData, decompressedData);
    }

    // Prepare output buffer - start with 4x the compressed size
    const size_t initialBufferSize = compressedData.size() * 4;
    const size_t maxBufferSize = compressedData.size() * 50; // Reasonable limit
    decompressedData.resize(initialBufferSize);

    size_t totalDecompressed = 0;
    int result = BZ_OK;

    while (result == BZ_OK) {
        // Ensure we have space in the output buffer
        if (totalDecompressed >= decompressedData.size()) {
            size_t newSize = decompressedData.size() * 2;
            if (newSize > maxBufferSize) {
                logger_.Error("BZip2: Decompressed data exceeds maximum size limit");
                BZ2_bzDecompressEnd(&bzStream);
                return false;
            }
            decompressedData.resize(newSize);
            logger_.Info("BZip2: Expanding output buffer to " + std::to_string(newSize) + " bytes");
        }

        // Set up output buffer for this iteration
        bzStream.next_out = reinterpret_cast<char*>(decompressedData.data() + totalDecompressed);
        bzStream.avail_out = static_cast<unsigned int>(decompressedData.size() - totalDecompressed);

        // Decompress
        result = BZ2_bzDecompress(&bzStream);

        if (result != BZ_OK && result != BZ_STREAM_END) {
            logger_.Error("BZip2: Decompression failed with error: " + std::to_string(result));
            BZ2_bzDecompressEnd(&bzStream);
            return false;
        }

        // Update total decompressed bytes
        totalDecompressed = decompressedData.size() - bzStream.avail_out;

        // Log progress for large files
        if (totalDecompressed % (1024 * 1024) == 0 && totalDecompressed > 0) {
            logger_.Info("BZip2: Decompressed " + std::to_string(totalDecompressed / 1024) + " KB so far");
        }
    }

    // Clean up
    BZ2_bzDecompressEnd(&bzStream);

    // Resize to actual decompressed size
    decompressedData.resize(totalDecompressed);

    logger_.Info("BZip2 decompression completed successfully");
    logger_.Info("Compressed: " + std::to_string(compressedData.size()) + " bytes -> " +
                "Decompressed: " + std::to_string(decompressedData.size()) + " bytes");
    logger_.Info("Compression ratio: " +
                std::to_string(static_cast<double>(compressedData.size()) / decompressedData.size() * 100.0) + "%");

    return true;
#else
    // If bzip2 is not available, try deflate as fallback
    logger_.Warning("BZip2 not available - trying deflate decompression as fallback");
    return DecompressRawDeflate(compressedData, decompressedData);
#endif
}

bool XFileDecompressor::IsZipCompressed(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return false;
    // ZIP file signature: 0x504B0304
    return data[0] == 0x50 && data[1] == 0x4B && data[2] == 0x03 && data[3] == 0x04;
}

bool XFileDecompressor::IsBzip2Compressed(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return false;

    // Bzip2 signature: "B" followed by version and block size
    // Standard bzip2 files start with "BZh" where 'h' is the block size (1-9)
    if (data[0] == 'B' && data[1] == 'Z') {
        // Check for standard bzip2 format: BZh[1-9]
        if (data[2] == 'h' && data[3] >= '1' && data[3] <= '9') {
            return true;
        }
        // Some variants might just have "BZ" + other bytes
        // Allow this for broader compatibility
        return true;
    }

    return false;
}

bool XFileDecompressor::IsDirectXLZCompressed(const std::vector<uint8_t>& data) {
    if (data.size() < 8) return false;

    // Check for common DirectX LZ headers
    uint32_t header = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

    // Common DirectX LZ signatures
    return (header == 0x00038760 ||  // DirectX LZ variant 1
            header == 0x01038760 ||  // DirectX LZ variant 2
            header == 0x02038760);   // DirectX LZ variant 3
}

bool XFileDecompressor::IsCompressionSupported() {
#ifdef HAVE_BZIP2
    return true;
#else
    return false;
#endif
}

bool XFileDecompressor::DecompressWithZlib(const std::vector<uint8_t>& input,
                                          std::vector<uint8_t>& output) {
    return false; // Placeholder
}

bool XFileDecompressor::DecompressWithBzip2(const std::vector<uint8_t>& input,
                                           std::vector<uint8_t>& output) {
    return DecompressBzip2(input, output);
}

bool XFileDecompressor::DecompressRawDeflate(const std::vector<uint8_t>& input,
                                            std::vector<uint8_t>& output) {
#ifdef HAVE_ZLIB
    // Try raw deflate decompression - this might be what DirectX is actually using
    logger_.Info("Attempting raw deflate decompression of " + std::to_string(input.size()) + " bytes");

    z_stream zStream;
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.next_in = const_cast<Bytef*>(input.data());
    zStream.avail_in = static_cast<uInt>(input.size());

    // Initialize for raw deflate (negative window bits = raw deflate)
    int result = inflateInit2(&zStream, -15); // -15 = raw deflate
    if (result != Z_OK) {
        logger_.Warning("Failed to initialize raw deflate decompression: " + std::to_string(result));
        return false;
    }

    // Prepare output buffer
    output.resize(input.size() * 10); // Start with 10x expansion
    zStream.next_out = output.data();
    zStream.avail_out = static_cast<uInt>(output.size());

    result = inflate(&zStream, Z_FINISH);

    if (result == Z_STREAM_END) {
        size_t decompressedSize = output.size() - zStream.avail_out;
        output.resize(decompressedSize);
        inflateEnd(&zStream);

        logger_.Info("Raw deflate decompression successful! Decompressed " +
                    std::to_string(decompressedSize) + " bytes");

        // Check if result looks like X-file content
        if (decompressedSize > 16) {
            std::string start(reinterpret_cast<const char*>(output.data()), std::min<size_t>(16, decompressedSize));
            logger_.Info("Decompressed data starts with: " + start);

            if (start.find("xof") == 0 || start.find("template") != std::string::npos) {
                logger_.Info("Decompressed data appears to be valid X-file content!");
                return true;
            }
        }

        return true;
    } else {
        logger_.Warning("Raw deflate decompression failed with result: " + std::to_string(result));
        inflateEnd(&zStream);
        return false;
    }
#else
    (void)input; // Suppress unused parameter warning
    (void)output; // Suppress unused parameter warning
    logger_.Error("zlib support not compiled in - cannot decompress deflate streams");
    return false;
#endif
}

bool XFileDecompressor::DecompressDirectXBzip(const std::vector<uint8_t>& input,
                                              std::vector<uint8_t>& output) {
#ifdef HAVE_ZLIB
    // DirectX bzip0032 format analysis and custom decompression
    logger_.Info("Attempting DirectX proprietary bzip0032 decompression of " + std::to_string(input.size()) + " bytes");

    if (input.size() < 20) {
        logger_.Error("DirectX bzip: Input too small for DirectX compressed data");
        return false;
    }

    // Analyze DirectX header structure
    // DirectX compressed files often have:
    // - Custom headers with format indicators
    // - Modified compression parameters
    // - Different block structures

    // Try to detect the actual compression method used
    // Some DirectX files use deflate but with custom headers

    // Method 1: Try raw deflate with different window sizes
    std::vector<int> windowSizes = {-15, -14, -13, -12, -11, -10, 15, 14, 13, 12, 11, 10};

    for (int windowBits : windowSizes) {
        logger_.Info("Trying deflate decompression with window bits: " + std::to_string(windowBits));

        z_stream zStream;
        memset(&zStream, 0, sizeof(zStream));

        zStream.next_in = const_cast<Bytef*>(input.data());
        zStream.avail_in = static_cast<uInt>(input.size());

        int result = inflateInit2(&zStream, windowBits);
        if (result != Z_OK) {
            continue; // Try next window size
        }

        // Allocate output buffer
        size_t outputSize = input.size() * 10; // Start with 10x expansion
        output.resize(outputSize);

        zStream.next_out = output.data();
        zStream.avail_out = static_cast<uInt>(outputSize);

        result = inflate(&zStream, Z_FINISH);

        if (result == Z_STREAM_END) {
            // Success!
            output.resize(zStream.total_out);
            inflateEnd(&zStream);

            logger_.Info("DirectX bzip decompression successful with window bits " +
                        std::to_string(windowBits) + "! Decompressed " +
                        std::to_string(output.size()) + " bytes");

            // Validate decompressed content
            size_t decompressedSize = output.size();
            std::string start(reinterpret_cast<const char*>(output.data()), std::min<size_t>(16, decompressedSize));
            logger_.Info("Decompressed data starts with: " + start);

            if (start.find("xof") == 0 || start.find("template") != std::string::npos) {
                logger_.Info("Decompressed data appears to be valid X-file content!");
                return true;
            }

            logger_.Warning("Decompressed data doesn't appear to be valid X-file content");
            return false;
        }

        inflateEnd(&zStream);
    }

    // Method 2: Try skipping different header sizes
    std::vector<size_t> skipSizes = {0, 4, 8, 12, 16, 20, 24, 28, 32};

    for (size_t skip : skipSizes) {
        if (skip >= input.size()) continue;

        logger_.Info("Trying deflate with " + std::to_string(skip) + " bytes header skip");

        const uint8_t* dataPtr = input.data() + skip;
        size_t dataSize = input.size() - skip;

        z_stream zStream;
        memset(&zStream, 0, sizeof(zStream));

        zStream.next_in = const_cast<Bytef*>(dataPtr);
        zStream.avail_in = static_cast<uInt>(dataSize);

        // Try raw deflate first
        int result = inflateInit2(&zStream, -15);
        if (result != Z_OK) {
            continue;
        }

        size_t outputSize = dataSize * 10;
        output.resize(outputSize);

        zStream.next_out = output.data();
        zStream.avail_out = static_cast<uInt>(outputSize);

        result = inflate(&zStream, Z_FINISH);

        if (result == Z_STREAM_END) {
            output.resize(zStream.total_out);
            inflateEnd(&zStream);

            logger_.Info("DirectX bzip decompression successful with " +
                        std::to_string(skip) + " byte skip! Decompressed " +
                        std::to_string(output.size()) + " bytes");
            return true;
        }

        inflateEnd(&zStream);
    }

    // Method 3: Try to detect Microsoft cabinet/LZX compression
    // Some DirectX files use CAB-style compression
    logger_.Info("Trying Microsoft CAB-style decompression detection...");

    // Look for CAB compression signatures
    for (size_t i = 0; i < input.size() - 4; ++i) {
        if (input[i] == 0x4D && input[i+1] == 0x53 && input[i+2] == 0x43 && input[i+3] == 0x46) {
            logger_.Info("Found Microsoft CAB signature at offset " + std::to_string(i));
            // This would need a CAB decompression library
            break;
        }
    }

    logger_.Warning("All DirectX bzip decompression methods failed");
    return false;

#else
    logger_.Error("zlib support not available - cannot decompress DirectX bzip format");
    return false;
#endif
}

bool XFileDecompressor::DecompressBzip0032(const std::vector<uint8_t>& input,
                                           std::vector<uint8_t>& output) {
    logger_.Info("Attempting specialized bzip0032 DirectX format decompression");

    if (input.size() < 16) {
        logger_.Error("bzip0032: Input too small for DirectX bzip0032 format");
        return false;
    }

    // Log detailed header analysis
    std::string hexHeader;
    for (size_t i = 0; i < std::min<size_t>(32, input.size()); ++i) {
        char hex[3];
        sprintf(hex, "%02X", input[i]);
        hexHeader += hex;
        if (i < 31) hexHeader += " ";
    }
    logger_.Info("Full header (32 bytes): " + hexHeader);

    // DirectX bzip0032 specific analysis
    // The format seems to be: "xof 0303bzip0032" + compressed data
    const std::string expectedHeader = "xof 0303bzip0032";

    if (input.size() >= expectedHeader.length()) {
        std::string fileHeader(reinterpret_cast<const char*>(input.data()), expectedHeader.length());
        if (fileHeader == expectedHeader) {
            logger_.Info("Confirmed bzip0032 format header");

            // Skip the header and try to decompress the remaining data
            size_t dataOffset = expectedHeader.length();
            std::vector<uint8_t> compressedData(input.begin() + dataOffset, input.end());

            logger_.Info("Attempting decompression of " + std::to_string(compressedData.size()) + " bytes after header");

            // Try multiple decompression strategies for the data after header
            return TryMultipleDecompressionMethods(compressedData, output);
        }
    }

    // If header doesn't match exactly, try with different offsets
    logger_.Info("Header doesn't match exactly, trying with offsets...");

    for (size_t offset = 16; offset <= 32 && offset < input.size(); offset += 4) {
        std::vector<uint8_t> dataFromOffset(input.begin() + offset, input.end());
        if (TryMultipleDecompressionMethods(dataFromOffset, output)) {
            logger_.Info("Successfully decompressed data starting from offset " + std::to_string(offset));
            return true;
        }
    }

    logger_.Warning("bzip0032: All specialized decompression attempts failed");
    return false;
}

bool XFileDecompressor::TryMultipleDecompressionMethods(const std::vector<uint8_t>& data,
                                                       std::vector<uint8_t>& output) {
#ifdef HAVE_ZLIB
    if (data.empty()) return false;

    // Method 1: Try to detect if this might be zlib/deflate with custom wrapper
    logger_.Info("Trying enhanced deflate detection...");

    // Check for zlib header (0x78xx)
    if (data.size() >= 2 && data[0] == 0x78) {
        logger_.Info("Possible zlib header detected (0x78xx)");
        if (TryZlibDecompression(data, output)) {
            return true;
        }
    }

    // Method 2: Try raw deflate with all possible window sizes and offsets
    std::vector<int> windowSizes = {-15, -14, -13, -12, -11, -10, 15, 14, 13, 12, 11, 10, 9, 8};
    std::vector<size_t> offsets = {0, 1, 2, 3, 4, 8, 12, 16};

    for (size_t offset : offsets) {
        if (offset >= data.size()) continue;

        for (int windowBits : windowSizes) {
            if (TryDeflateWithParams(data, output, offset, windowBits)) {
                logger_.Info("Success with offset " + std::to_string(offset) +
                           " and window bits " + std::to_string(windowBits));
                return true;
            }
        }
    }

    // Method 3: Try to interpret as LZ77 variant
    logger_.Info("Trying LZ77 variant decompression...");
    if (TryLZ77Decompression(data, output)) {
        return true;
    }

    // Method 4: Try reverse engineering approach - look for patterns
    logger_.Info("Trying pattern-based decompression...");
    if (TryPatternBasedDecompression(data, output)) {
        return true;
    }

#endif
    return false;
}

bool XFileDecompressor::TryZlibDecompression(const std::vector<uint8_t>& data,
                                            std::vector<uint8_t>& output) {
#ifdef HAVE_ZLIB
    z_stream zStream;
    memset(&zStream, 0, sizeof(zStream));

    zStream.next_in = const_cast<Bytef*>(data.data());
    zStream.avail_in = static_cast<uInt>(data.size());

    int result = inflateInit(&zStream);
    if (result != Z_OK) {
        return false;
    }

    size_t outputSize = data.size() * 20; // Try larger expansion
    output.resize(outputSize);

    zStream.next_out = output.data();
    zStream.avail_out = static_cast<uInt>(outputSize);

    result = inflate(&zStream, Z_FINISH);

    if (result == Z_STREAM_END) {
        output.resize(zStream.total_out);
        inflateEnd(&zStream);

        // Validate the output
        if (ValidateDecompressedContent(output)) {
            logger_.Info("zlib decompression successful! Decompressed " +
                        std::to_string(output.size()) + " bytes");
            return true;
        }
    }

    inflateEnd(&zStream);
#endif
    return false;
}

bool XFileDecompressor::TryDeflateWithParams(const std::vector<uint8_t>& data,
                                            std::vector<uint8_t>& output,
                                            size_t offset, int windowBits) {
#ifdef HAVE_ZLIB
    if (offset >= data.size()) return false;

    z_stream zStream;
    memset(&zStream, 0, sizeof(zStream));

    zStream.next_in = const_cast<Bytef*>(data.data() + offset);
    zStream.avail_in = static_cast<uInt>(data.size() - offset);

    int result = inflateInit2(&zStream, windowBits);
    if (result != Z_OK) {
        return false;
    }

    size_t outputSize = (data.size() - offset) * 15; // Even larger expansion
    output.resize(outputSize);

    zStream.next_out = output.data();
    zStream.avail_out = static_cast<uInt>(outputSize);

    result = inflate(&zStream, Z_FINISH);

    if (result == Z_STREAM_END) {
        output.resize(zStream.total_out);
        inflateEnd(&zStream);

        if (ValidateDecompressedContent(output)) {
            return true;
        }
    }

    inflateEnd(&zStream);
#endif
    return false;
}

bool XFileDecompressor::TryLZ77Decompression(const std::vector<uint8_t>& data,
                                            std::vector<uint8_t>& output) {
    // Simple LZ77 decompression attempt
    // This is a basic implementation for DirectX-style LZ compression

    if (data.size() < 4) return false;

    output.clear();
    output.reserve(data.size() * 10);

    size_t pos = 0;

    while (pos < data.size()) {
        uint8_t control = data[pos++];

        for (int bit = 0; bit < 8 && pos < data.size(); ++bit) {
            if (control & (1 << bit)) {
                // Literal byte
                if (pos < data.size()) {
                    output.push_back(data[pos++]);
                }
            } else {
                // Length-distance pair
                if (pos + 1 >= data.size()) break;

                uint16_t lengthDistance = (data[pos + 1] << 8) | data[pos];
                pos += 2;

                uint16_t distance = (lengthDistance >> 4) + 1;
                uint16_t length = (lengthDistance & 0x0F) + 3;

                if (distance > output.size()) continue;

                size_t startPos = output.size() - distance;
                for (int i = 0; i < length; ++i) {
                    if (startPos + i < output.size()) {
                        output.push_back(output[startPos + i]);
                    }
                }
            }
        }
    }

    if (ValidateDecompressedContent(output)) {
        logger_.Info("LZ77 decompression successful! Decompressed " +
                    std::to_string(output.size()) + " bytes");
        return true;
    }

    return false;
}

bool XFileDecompressor::TryPatternBasedDecompression(const std::vector<uint8_t>& data,
                                                   std::vector<uint8_t>& output) {
    // Look for embedded X-file content or patterns

    // Search for "xof" pattern within the data
    for (size_t i = 0; i < data.size() - 3; ++i) {
        if (data[i] == 'x' && data[i+1] == 'o' && data[i+2] == 'f' && data[i+3] == ' ') {
            logger_.Info("Found embedded 'xof ' pattern at offset " + std::to_string(i));

            // Try to extract data from this point
            output.assign(data.begin() + i, data.end());

            if (ValidateDecompressedContent(output)) {
                logger_.Info("Pattern-based extraction successful!");
                return true;
            }
        }
    }

    // Try to find template patterns
    const std::string templatePattern = "template";
    for (size_t i = 0; i <= data.size() - templatePattern.length(); ++i) {
        if (std::memcmp(data.data() + i, templatePattern.c_str(), templatePattern.length()) == 0) {
            logger_.Info("Found 'template' pattern at offset " + std::to_string(i));

            // Extract from the beginning or from this point
            if (i > 0) {
                // Try from beginning with this as validation
                output.assign(data.begin(), data.end());
            } else {
                output.assign(data.begin() + i, data.end());
            }

            if (ValidateDecompressedContent(output)) {
                logger_.Info("Template pattern extraction successful!");
                return true;
            }
        }
    }

    return false;
}

bool XFileDecompressor::ValidateDecompressedContent(const std::vector<uint8_t>& data) {
    if (data.empty()) return false;

    // Check for X-file signatures
    std::string start(reinterpret_cast<const char*>(data.data()),
                     std::min<size_t>(16, data.size()));

    logger_.Info("Validating decompressed content, starts with: '" + start + "'");

    // Valid X-file should start with "xof " or contain "template"
    if (start.find("xof ") == 0) {
        logger_.Info("Valid X-file header found");
        return true;
    }

    if (data.size() > 100) {
        std::string larger(reinterpret_cast<const char*>(data.data()),
                          std::min<size_t>(100, data.size()));
        if (larger.find("template") != std::string::npos) {
            logger_.Info("X-file template found in content");
            return true;
        }
    }

    // Check for binary X-file signature
    if (data.size() >= 4) {
        if ((data[0] == 'x' && data[1] == 'o' && data[2] == 'f' && data[3] == ' ') ||
            (data[0] == 0x78 && data[1] == 0x6f && data[2] == 0x66 && data[3] == 0x20)) {
            logger_.Info("Binary X-file signature detected");
            return true;
        }
    }

    return false;
}

// Add new method to handle Microsoft LZ compression (commonly used in DirectX)
bool XFileDecompressor::DecompressDirectXLZ(const std::vector<uint8_t>& input,
                                           std::vector<uint8_t>& output) {
    logger_.Info("Attempting DirectX LZ decompression of " + std::to_string(input.size()) + " bytes");

    if (input.size() < 8) {
        logger_.Error("DirectX LZ: Input too small");
        return false;
    }

    // Analyze the header structure
    // Many DirectX files use a custom LZ format with specific headers
    const uint8_t* data = input.data();

    // Check for common DirectX LZ signatures
    uint32_t header = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

    logger_.Info("DirectX LZ header analysis: 0x" +
                std::to_string(header) + " (" +
                std::to_string(data[0]) + " " +
                std::to_string(data[1]) + " " +
                std::to_string(data[2]) + " " +
                std::to_string(data[3]) + ")");

    // Method 1: Try Microsoft LZ decompression algorithm
    // This is a simplified LZ77 implementation often used by DirectX
    if (header == 0x00038760) { // Common DirectX LZ header
        logger_.Info("Detected DirectX LZ format with signature 0x00038760");

        size_t inputPos = 4; // Skip header
        output.clear();
        output.reserve(input.size() * 8); // Reserve space

        while (inputPos < input.size()) {
            if (inputPos >= input.size()) break;

            uint8_t flag = data[inputPos++];

            // Process 8 bits in the flag byte
            for (int bit = 0; bit < 8 && inputPos < input.size(); ++bit) {
                if (flag & (1 << bit)) {
                    // Literal byte
                    if (inputPos < input.size()) {
                        output.push_back(data[inputPos++]);
                    }
                } else {
                    // Length-distance pair
                    if (inputPos + 1 >= input.size()) break;

                    uint16_t lenDist = (data[inputPos + 1] << 8) | data[inputPos];
                    inputPos += 2;

                    int distance = (lenDist >> 4) + 1;
                    int length = (lenDist & 0xF) + 3;

                    if (distance > output.size()) {
                        logger_.Warning("DirectX LZ: Invalid distance reference");
                        break;
                    }

                    // Copy from the sliding window
                    size_t copyStart = output.size() - distance;
                    for (int i = 0; i < length; ++i) {
                        if (copyStart + (i % distance) < output.size()) {
                            output.push_back(output[copyStart + (i % distance)]);
                        }
                    }
                }
            }
        }

        if (!output.empty()) {
            logger_.Info("DirectX LZ decompression successful! Decompressed " +
                        std::to_string(output.size()) + " bytes");

            // Validate decompressed content
            std::string start(reinterpret_cast<const char*>(output.data()),
                            std::min<size_t>(16, output.size()));
            logger_.Info("Decompressed data starts with: " + start);

            if (start.find("xof") == 0 || start.find("template") != std::string::npos) {
                logger_.Info("Decompressed data appears to be valid X-file content!");
                return true;
            }
        }
    }

    // Method 2: Try LZSS decompression (another common DirectX format)
    logger_.Info("Trying LZSS decompression variant...");

    output.clear();
    size_t pos = 0;

    // Skip any header bytes and try direct LZSS
    for (size_t headerSkip : {0, 4, 8, 12, 16}) {
        if (headerSkip >= input.size()) continue;

        pos = headerSkip;
        output.clear();

        while (pos < input.size()) {
            if (pos >= input.size()) break;

            uint8_t flags = data[pos++];

            for (int i = 0; i < 8 && pos < input.size(); ++i) {
                if (flags & (0x01 << i)) {
                    // Literal
                    output.push_back(data[pos++]);
                } else {
                    // Match
                    if (pos + 1 >= input.size()) break;

                    uint16_t match = data[pos] | (data[pos + 1] << 8);
                    pos += 2;

                    int offset = (match >> 4) + 1;
                    int length = (match & 0x0F) + 3;

                    if (offset <= output.size()) {
                        size_t start = output.size() - offset;
                        for (int j = 0; j < length; ++j) {
                            output.push_back(output[start + (j % offset)]);
                        }
                    }
                }
            }
        }

        // Check if decompression was successful
        if (output.size() > 100) { // Reasonable size check
            std::string start(reinterpret_cast<const char*>(output.data()),
                            std::min<size_t>(16, output.size()));

            if (start.find("xof") == 0 || start.find("template") != std::string::npos) {
                logger_.Info("LZSS decompression successful with " +
                           std::to_string(headerSkip) + " byte skip! Decompressed " +
                           std::to_string(output.size()) + " bytes");
                return true;
            }
        }
    }

    logger_.Warning("All DirectX LZ decompression methods failed");
    return false;
}

// =============================================================================
// BinaryXFileParser Implementation
// =============================================================================

BinaryXFileParser::BinaryXFileParser() : logger_(Logger::GetInstance()) {
}

BinaryXFileParser::~BinaryXFileParser() = default;

bool BinaryXFileParser::ParseBinaryFile(const std::string& filepath) {
    logger_.Info("Attempting to parse binary .x file: " + filepath);

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        logger_.Error("Failed to open file: " + filepath);
        return false;
    }

    // Read file data
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    return ParseBinaryData(data);
}

bool BinaryXFileParser::ParseBinaryData(const std::vector<uint8_t>& data) {
    logger_.Warning("Binary .x file parsing not fully implemented");

    // Basic validation
    if (data.size() < 16) {
        logger_.Error("File too small to be a valid .x file");
        return false;
    }

    // Check for .x file signature
    if (data.size() >= 4) {
        std::string signature(data.begin(), data.begin() + 4);
        if (signature != "xof ") {
            logger_.Error("Invalid .x file signature");
            return false;
        }
    }

    // Placeholder - would implement full binary parsing here
    return false;
}

bool BinaryXFileParser::ParseCompressedFile(const std::string& filepath) {
    logger_.Info("Attempting to parse compressed .x file: " + filepath);

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        logger_.Error("Failed to open file: " + filepath);
        return false;
    }

    // Read file data
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    XFileDecompressor decompressor;
    std::vector<uint8_t> decompressedData;
    std::vector<uint8_t> compressedData;

    // Check if this is a DirectX .x file with compression
    if (data.size() >= 16) {
        std::string signature(data.begin(), data.begin() + 4);
        if (signature == "xof ") {
            std::string header(data.begin(), data.begin() + 16);
            std::string formatStr = header.substr(8, 4);

            logger_.Info("DirectX .x file detected with format: " + formatStr);

            if (formatStr == "bzip") {
                logger_.Info("DirectX .x file with bzip2 compression detected");

                // DirectX .x compressed files have a more complex structure
                // Let's analyze the file structure step by step
                logger_.Info("Analyzing DirectX .x file structure...");
                logger_.Info("File size: " + std::to_string(data.size()) + " bytes");

                // Log the first 32 bytes for analysis
                if (data.size() >= 32) {
                    std::string hexDump = "First 32 bytes: ";
                    for (size_t i = 0; i < 32; ++i) {
                        char hex[4];
                        sprintf(hex, "%02X ", data[i]);
                        hexDump += hex;
                    }
                    logger_.Info(hexDump);
                }

                // Try different offsets to find bzip2 data
                bool found = false;
                std::vector<size_t> offsetsToTry = {16, 20, 24, 28, 32, 40, 48, 64};

                for (size_t offset : offsetsToTry) {
                    if (data.size() > offset) {
                        compressedData.assign(data.begin() + offset, data.end());

                        logger_.Info("Trying offset " + std::to_string(offset) + " bytes...");
                        if (compressedData.size() >= 4) {
                            std::string firstBytes = "";
                            size_t maxBytes = (compressedData.size() < 8) ? compressedData.size() : 8;
                            for (size_t i = 0; i < maxBytes; ++i) {
                                char hex[4];
                                sprintf(hex, "%02X ", compressedData[i]);
                                firstBytes += hex;
                            }
                            logger_.Info("Data at offset " + std::to_string(offset) + ": " + firstBytes);
                        }

                        if (decompressor.IsBzip2Compressed(compressedData)) {
                            logger_.Info("Valid bzip2 compressed data found at offset " + std::to_string(offset));
                            if (!decompressor.DecompressBzip2(compressedData, decompressedData)) {
                                logger_.Error("Failed to decompress bzip2 data from DirectX .x file");
                                return false;
                            }
                            found = true;
                            break;
                        }
                    }
                }

                if (!found) {
                    // Let's also try to search for 'BZ' signature anywhere in the file
                    logger_.Info("Searching for BZ signature anywhere in the file...");
                    for (size_t i = 0; i < data.size() - 2; ++i) {
                        if (data[i] == 'B' && data[i + 1] == 'Z') {
                            logger_.Info("Found 'BZ' signature at byte offset " + std::to_string(i));

                            compressedData.assign(data.begin() + i, data.end());
                            if (decompressor.IsBzip2Compressed(compressedData)) {
                                logger_.Info("Valid bzip2 data found at offset " + std::to_string(i));
                                if (!decompressor.DecompressBzip2(compressedData, decompressedData)) {
                                    logger_.Error("Failed to decompress bzip2 data from DirectX .x file");
                                    return false;
                                }
                                found = true;
                                break;
                            }
                        }
                    }
                }

                if (!found) {
                    // This may be a DirectX proprietary compression format, not standard bzip2
                    logger_.Warning("Standard bzip2 signature not found - this may be DirectX proprietary compression");

                    // Try DirectX-specific bzip0032 decompression first
                    logger_.Info("Attempting DirectX proprietary bzip0032 decompression...");

                    // Use the specialized bzip0032 method with the full data including header
                    if (decompressor.DecompressBzip0032(data, decompressedData)) {
                        logger_.Info("Successfully decompressed using specialized bzip0032 method!");
                        return ParseBinaryData(decompressedData);
                    }

                    // Fallback to original method with payload only
                    if (data.size() > 16) {
                        std::vector<uint8_t> compressedPayload(data.begin() + 16, data.end());
                        if (decompressor.DecompressDirectXBzip(compressedPayload, decompressedData)) {
                            logger_.Info("Successfully decompressed using fallback DirectX bzip method!");
                            return ParseBinaryData(decompressedData);
                        }

                        // If DirectX bzip fails, try DirectX LZ compression
                        logger_.Info("DirectX bzip failed, trying DirectX LZ compression...");
                        if (decompressor.DecompressDirectXLZ(compressedPayload, decompressedData)) {
                            logger_.Info("Successfully decompressed using DirectX LZ method!");
                            return ParseBinaryData(decompressedData);
                        }
                    }
                }
            } else if (formatStr == "tzip") {
                logger_.Info("DirectX .x file with zip compression detected");

                // Skip the DirectX header (16 bytes) to get to compressed data
                if (data.size() > 16) {
                    compressedData.assign(data.begin() + 16, data.end());

                    if (decompressor.IsZipCompressed(compressedData)) {
                        logger_.Info("Valid zip compressed data found after DirectX header");
                        if (!decompressor.DecompressZipped(compressedData, decompressedData)) {
                            logger_.Error("Failed to decompress zip data from DirectX .x file");
                            return false;
                        }
                    } else {
                        logger_.Error("Expected zip data not found after DirectX header");
                        return false;
                    }
                } else {
                    logger_.Error("File too small to contain compressed data after DirectX header");
                    return false;
                }
            } else {
                logger_.Error("Unsupported DirectX .x compression format: " + formatStr);
                return false;
            }
        } else {
            // Not a DirectX .x file, check for pure compression formats
            if (decompressor.IsZipCompressed(data) || decompressor.IsBzip2Compressed(data) || decompressor.IsDirectXLZCompressed(data)) {
                logger_.Info("Pure compression detected");

                // Try to parse the compressed data as raw binary DirectX data
                // Some DirectX .x files with "bzip" header use proprietary compression
                if (data.size() > 16) {
                    std::vector<uint8_t> rawData(data.begin() + 16, data.end());
                    logger_.Info("Attempting to parse " + std::to_string(rawData.size()) + " bytes as raw DirectX binary data");

                    // Try to parse it directly as binary data
                    if (ParseBinaryData(rawData)) {
                        logger_.Info("Successfully parsed as raw DirectX binary data");
                        return true;
                    }

                    // If that fails, log more detailed analysis
                    logger_.Warning("Could not parse as raw binary data either");
                    logger_.Info("This file may require specialized DirectX compression handling");

                    // Log some additional analysis
                    if (rawData.size() >= 16) {
                        std::string secondHexDump = "Next 16 bytes after header: ";
                        for (size_t i = 0; i < 16; ++i) {
                            char hex[4];
                            sprintf(hex, "%02X ", rawData[i]);
                            secondHexDump += hex;
                        }
                        logger_.Info(secondHexDump);
                    }

                    // Check if this might be zlib instead
                    logger_.Info("Checking if data might be zlib compressed...");
                    if (decompressor.IsZipCompressed(rawData)) {
                        logger_.Info("Data appears to be zip/zlib compressed, attempting decompression...");
                        if (decompressor.DecompressZipped(rawData, decompressedData)) {
                            logger_.Info("Successfully decompressed as zip/zlib data");
                            return ParseBinaryData(decompressedData);
                        }
                    }

                    // Try raw deflate decompression (without zlib headers)
                    logger_.Info("Attempting raw deflate decompression...");
                    if (decompressor.DecompressRawDeflate(rawData, decompressedData)) {
                        logger_.Info("Successfully decompressed using raw deflate!");
                        return ParseBinaryData(decompressedData);
                    } else {
                        logger_.Warning("Raw deflate decompression also failed");
                    }

                    // Last resort: try to parse as text format
                    logger_.Info("Attempting to parse as text format...");
                    std::string textData(rawData.begin(), rawData.end());
                    if (textData.find("template") != std::string::npos ||
                        textData.find("Mesh") != std::string::npos ||
                        textData.find("{") != std::string::npos) {
                        logger_.Info("Data appears to contain text-like DirectX content");
                        logger_.Info("Attempting to parse with text parser...");

                        // Try parsing with the text parser
                        XFileParser tempTextParser;
                        if (tempTextParser.ParseFromString(textData)) {
                            logger_.Info("Successfully parsed as text format!");
                            parsedData_ = tempTextParser.TakeParsedData();
                            return true;
                        } else {
                            logger_.Warning("Text parser also failed - data might be corrupted or unknown format");
                        }
                    }
                }

                logger_.Error("Could not process DirectX .x file with bzip header");
                logger_.Error("This may be a proprietary or unsupported compression variant");
                return false;
            } else {
                logger_.Error("Unknown or unsupported .x file format");
                return false;
            }
        }
    } else {
        logger_.Error("File too small to determine compression format");
        return false;
    }

    logger_.Info("Successfully decompressed data, size: " + std::to_string(decompressedData.size()) + " bytes");
    return ParseBinaryData(decompressedData);
}

// =============================================================================
// EnhancedXFileParser Implementation
// =============================================================================

EnhancedXFileParser::EnhancedXFileParser()
    : logger_(Logger::GetInstance()),
      textParser_(),
      binaryParser_(),
      decompressor_() {
}

EnhancedXFileParser::~EnhancedXFileParser() = default;

bool EnhancedXFileParser::ParseFile(const std::string& filepath) {
    logger_.Info("Parsing .x file with enhanced parser: " + filepath);

    // Detect file format
    auto format = DetectFileFormat(filepath);

    switch (format) {
        case XFileHeader::TEXT:
            return ParseTextFormat(filepath);
        case XFileHeader::BINARY:
            return ParseBinaryFormat(filepath);
        case XFileHeader::COMPRESSED:
            return ParseCompressedFormat(filepath);
        default:
            logger_.Error("Unknown or unsupported .x file format");
            return false;
    }
}

bool EnhancedXFileParser::ParseFromData(const std::vector<uint8_t>& data) {
    auto format = DetectDataFormat(data);

    switch (format) {
        case XFileHeader::TEXT: {
            // Convert to string and parse
            std::string textData(data.begin(), data.end());
            return textParser_.ParseFromString(textData);
        }
        case XFileHeader::BINARY:
            return binaryParser_.ParseBinaryData(data);
        case XFileHeader::COMPRESSED: {
            std::vector<uint8_t> decompressedData;
            if (decompressor_.IsZipCompressed(data)) {
                if (!decompressor_.DecompressZipped(data, decompressedData)) {
                    logger_.Error("Failed to decompress ZIP data");
                    return false;
                }
            } else if (decompressor_.IsBzip2Compressed(data)) {
                if (!decompressor_.DecompressBzip2(data, decompressedData)) {
                    logger_.Error("Failed to decompress BZIP2 data");
                    return false;
                }
            } else if (decompressor_.IsDirectXLZCompressed(data)) {
                if (!decompressor_.DecompressDirectXLZ(data, decompressedData)) {
                    logger_.Error("Failed to decompress DirectX LZ data");
                    return false;
                }
            } else {
                logger_.Error("Unknown compression format - not DirectX .x or recognizable compression");
                return false;
            }
            return ParseFromData(decompressedData);
        }
        default:
            logger_.Error("Unknown data format");
            return false;
    }
}

const XFileData& EnhancedXFileParser::GetParsedData() const {
    // Return data from whichever parser was used
    if (!textParser_.GetParsedData().meshes.empty()) {
        return textParser_.GetParsedData();
    } else {
        return binaryParser_.GetParsedData();
    }
}

XFileData EnhancedXFileParser::TakeParsedData() {
    // Take data from whichever parser was used
    if (!textParser_.GetParsedData().meshes.empty()) {
        return textParser_.TakeParsedData();
    } else {
        return binaryParser_.TakeParsedData();
    }
}

XFileHeader::Format EnhancedXFileParser::DetectFileFormat(const std::string& filepath) {
    std::vector<uint8_t> data = ReadFileToBytes(filepath);
    if (data.empty()) {
        return XFileHeader::TEXT;
    }

    return DetectDataFormat(data);
}

XFileHeader::Format EnhancedXFileParser::DetectDataFormat(const std::vector<uint8_t>& data) {
    if (data.size() < 16) {
        return XFileHeader::TEXT;
    }

    // Check .x file signature first
    if (data.size() >= 4) {
        std::string signature(data.begin(), data.begin() + 4);
        if (signature != "xof ") {
            // If no .x signature, check for pure compression formats
            if (decompressor_.IsZipCompressed(data) || decompressor_.IsBzip2Compressed(data) || decompressor_.IsDirectXLZCompressed(data)) {
                return XFileHeader::COMPRESSED;
            }
            return XFileHeader::TEXT;
        }
    }

    // Check for format indicators in .x file header (positions 8-12)
    if (data.size() >= 16) {
        std::string header(data.begin(), data.begin() + 16);
        std::string formatStr = header.substr(8, 4);

        if (formatStr == "txt ") {
            return XFileHeader::TEXT;
        } else if (formatStr == "bin ") {
            return XFileHeader::BINARY;
        } else if (formatStr == "tzip" || formatStr == "bzip" || formatStr == "lz") {
            return XFileHeader::COMPRESSED;
        }
    }

    // Default to text if we can't determine
    return XFileHeader::TEXT;
}

void EnhancedXFileParser::SetStrictMode(bool strict) {
    textParser_.SetStrictMode(strict);
    // binaryParser_ would also support this if implemented
}

void EnhancedXFileParser::SetVerboseLogging(bool verbose) {
    textParser_.SetVerboseLogging(verbose);
    // binaryParser_ would also support this if implemented
}

bool EnhancedXFileParser::ParseTextFormat(const std::string& filepath) {
    return textParser_.ParseFile(filepath);
}

bool EnhancedXFileParser::ParseBinaryFormat(const std::string& filepath) {
    logger_.Warning("Binary .x format not fully supported yet, falling back to text parser");
    return textParser_.ParseFile(filepath);
}

bool EnhancedXFileParser::ParseCompressedFormat(const std::string& filepath) {
    return binaryParser_.ParseCompressedFile(filepath);
}

std::vector<uint8_t> EnhancedXFileParser::ReadFileToBytes(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        logger_.Error("Failed to open file: " + filepath);
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    return data;
}

bool EnhancedXFileParser::ValidateFileSignature(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return false;
    }

    std::string signature(data.begin(), data.begin() + 4);
    return signature == "xof ";
}

} // namespace X2FBX
