#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace X2FBX {

// Forward declarations
struct XVector3;
struct XVector2;
struct XQuaternion;
struct XMatrix4x4;

// Basic math structures
struct XVector3 {
    float x, y, z;

    XVector3() : x(0), y(0), z(0) {}
    XVector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct XVector2 {
    float u, v;

    XVector2() : u(0), v(0) {}
    XVector2(float u_, float v_) : u(u_), v(v_) {}
};

struct XQuaternion {
    float x, y, z, w;

    XQuaternion() : x(0), y(0), z(0), w(1) {}
    XQuaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

struct XMatrix4x4 {
    float m[4][4];

    XMatrix4x4();
    static XMatrix4x4 Identity();
    XMatrix4x4 operator*(const XMatrix4x4& other) const;
};

// Material and texture data
struct XMaterial {
    std::string name;
    XVector3 diffuseColor;
    XVector3 specularColor;
    XVector3 emissiveColor;
    float shininess;
    float transparency;
    std::string diffuseTexture;
    std::string normalTexture;
    std::string specularTexture;
};

// Keyframe data for animations
struct XKeyframe {
    float time;                    // Time in .x file ticks
    XVector3 position;
    XQuaternion rotation;
    XVector3 scale;

    XKeyframe() : time(0), scale(1, 1, 1) {}
};

// Animation set representing one named animation
struct XAnimationSet {
    std::string name;
    float duration;                // Total duration in .x file ticks
    float ticksPerSecond;         // Ticks per second (critical for timing!)
    std::vector<XKeyframe> keyframes;
    std::map<std::string, std::vector<XKeyframe>> boneKeyframes; // Per-bone keyframes

    XAnimationSet() : duration(0), ticksPerSecond(4800.0f) {} // Default DirectX value

    // Get duration in real seconds
    float GetDurationInSeconds() const {
        return ticksPerSecond > 0 ? duration / ticksPerSecond : 0;
    }
};

// Bone/Joint data
struct XBone {
    std::string name;
    std::string parentName;
    int parentIndex;
    XMatrix4x4 bindPose;          // Bind pose transformation
    XMatrix4x4 offsetMatrix;      // Inverse bind pose
    std::vector<int> childIndices;

    XBone() : parentIndex(-1) {}
};

// Vertex data
struct XVertex {
    XVector3 position;
    XVector3 normal;
    XVector2 texCoord;
    std::vector<int> boneIndices;     // Up to 4 bone influences
    std::vector<float> boneWeights;   // Corresponding weights

    XVertex() {
        boneIndices.reserve(4);
        boneWeights.reserve(4);
    }
};

// Face/Triangle data
struct XFace {
    int indices[3];               // Triangle vertex indices
    std::vector<int> vertexIndices; // Alternative access as vector
    int materialIndex;

    XFace() : materialIndex(-1) {
        indices[0] = indices[1] = indices[2] = 0;
        vertexIndices = {0, 0, 0};
    }

    // Update both representations when indices change
    void SetIndices(int i0, int i1, int i2) {
        indices[0] = i0; indices[1] = i1; indices[2] = i2;
        vertexIndices = {i0, i1, i2};
    }
};

// Complete mesh data from .x file
struct XMeshData {
    std::string name;
    std::vector<XVertex> vertices;
    std::vector<XFace> faces;
    std::vector<XMaterial> materials;
    std::vector<XBone> bones;
    std::vector<XAnimationSet> animations;

    // File-level timing information
    float globalTicksPerSecond;
    bool hasTimingInfo;

    XMeshData() : globalTicksPerSecond(4800.0f), hasTimingInfo(false) {}

    // Utility functions
    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetFaceCount() const { return faces.size(); }
    size_t GetBoneCount() const { return bones.size(); }
    size_t GetAnimationCount() const { return animations.size(); }

    // Validation
    bool IsValid() const;
    std::vector<std::string> GetValidationErrors() const;
};

// File header information
struct XFileHeader {
    enum Format {
        TEXT,
        BINARY,
        COMPRESSED
    };

    Format format;
    int majorVersion;
    int minorVersion;
    bool hasAnimationTimingInfo;
    float ticksPerSecond;

    XFileHeader() : format(TEXT), majorVersion(3), minorVersion(3),
                   hasAnimationTimingInfo(false), ticksPerSecond(4800.0f) {}
};

// Complete .x file data
struct XFileData {
    XFileHeader header;
    XMeshData meshData;
    std::vector<XMeshData> meshes;           // Multiple meshes support
    std::vector<XMaterial> materials;        // Materials
    std::vector<XAnimationSet> animations;   // Animations
    std::map<std::string, std::string> metadata;

    // Parse result information
    bool parseSuccessful;
    std::vector<std::string> parseErrors;
    std::vector<std::string> parseWarnings;

    XFileData() : parseSuccessful(false) {}

    bool IsValid() const {
        return parseSuccessful && meshData.IsValid();
    }

    // Get all animation names
    std::vector<std::string> GetAnimationNames() const {
        std::vector<std::string> names;
        for (const auto& anim : meshData.animations) {
            names.push_back(anim.name);
        }
        return names;
    }
};

} // namespace X2FBX
