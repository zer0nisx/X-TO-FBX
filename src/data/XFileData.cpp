#include "XFileData.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace X2FBX {

// XMatrix4x4 implementation
XMatrix4x4::XMatrix4x4() {
    std::memset(m, 0, sizeof(m));
}

XMatrix4x4 XMatrix4x4::Identity() {
    XMatrix4x4 result;
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
}

XMatrix4x4 XMatrix4x4::operator*(const XMatrix4x4& other) const {
    XMatrix4x4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

// XMeshData validation
bool XMeshData::IsValid() const {
    auto errors = GetValidationErrors();
    return errors.empty();
}

std::vector<std::string> XMeshData::GetValidationErrors() const {
    std::vector<std::string> errors;

    // Check vertex data
    if (vertices.empty()) {
        errors.push_back("No vertices found in mesh");
        return errors; // No point checking further
    }

    // Check face data
    if (faces.empty()) {
        errors.push_back("No faces found in mesh");
    }

    // Validate face indices
    int maxVertexIndex = static_cast<int>(vertices.size()) - 1;
    for (size_t i = 0; i < faces.size(); i++) {
        const XFace& face = faces[i];
        for (int j = 0; j < 3; j++) {
            if (face.indices[j] < 0 || face.indices[j] > maxVertexIndex) {
                errors.push_back("Face " + std::to_string(i) + " has invalid vertex index: " +
                                std::to_string(face.indices[j]));
            }
        }

        // Check material index
        if (face.materialIndex >= static_cast<int>(materials.size()) && face.materialIndex != -1) {
            errors.push_back("Face " + std::to_string(i) + " has invalid material index: " +
                           std::to_string(face.materialIndex));
        }
    }

    // Validate bone data if present
    if (!bones.empty()) {
        for (size_t i = 0; i < bones.size(); i++) {
            const XBone& bone = bones[i];

            // Check parent index
            if (bone.parentIndex >= static_cast<int>(bones.size())) {
                errors.push_back("Bone '" + bone.name + "' has invalid parent index: " +
                               std::to_string(bone.parentIndex));
            }

            // Check for circular references (basic check)
            if (bone.parentIndex == static_cast<int>(i)) {
                errors.push_back("Bone '" + bone.name + "' references itself as parent");
            }
        }

        // Validate vertex bone weights
        for (size_t i = 0; i < vertices.size(); i++) {
            const XVertex& vertex = vertices[i];

            if (vertex.boneIndices.size() != vertex.boneWeights.size()) {
                errors.push_back("Vertex " + std::to_string(i) +
                               " has mismatched bone indices and weights count");
            }

            // Check bone indices validity
            for (int boneIndex : vertex.boneIndices) {
                if (boneIndex >= static_cast<int>(bones.size())) {
                    errors.push_back("Vertex " + std::to_string(i) +
                                   " references invalid bone index: " + std::to_string(boneIndex));
                }
            }

            // Check weights sum (should be close to 1.0 for skinned vertices)
            if (!vertex.boneWeights.empty()) {
                float weightSum = 0;
                for (float weight : vertex.boneWeights) {
                    weightSum += weight;
                }

                if (std::abs(weightSum - 1.0f) > 0.01f) {
                    errors.push_back("Vertex " + std::to_string(i) +
                                   " has bone weights that don't sum to 1.0: " + std::to_string(weightSum));
                }
            }
        }
    }

    // Validate animation data
    for (size_t i = 0; i < animations.size(); i++) {
        const XAnimationSet& anim = animations[i];

        if (anim.name.empty()) {
            errors.push_back("Animation " + std::to_string(i) + " has no name");
        }

        if (anim.ticksPerSecond <= 0) {
            errors.push_back("Animation '" + anim.name + "' has invalid ticksPerSecond: " +
                           std::to_string(anim.ticksPerSecond));
        }

        if (anim.keyframes.empty() && anim.boneKeyframes.empty()) {
            errors.push_back("Animation '" + anim.name + "' has no keyframes");
        }

        // Validate keyframe timing
        for (size_t j = 1; j < anim.keyframes.size(); j++) {
            if (anim.keyframes[j].time < anim.keyframes[j-1].time) {
                errors.push_back("Animation '" + anim.name + "' has keyframes out of order at index " +
                               std::to_string(j));
                break;
            }
        }

        // Validate bone keyframes references
        for (const auto& boneAnim : anim.boneKeyframes) {
            const std::string& boneName = boneAnim.first;
            bool boneExists = false;

            for (const XBone& bone : bones) {
                if (bone.name == boneName) {
                    boneExists = true;
                    break;
                }
            }

            if (!boneExists) {
                errors.push_back("Animation '" + anim.name + "' references non-existent bone: " + boneName);
            }
        }
    }

    // Validate timing consistency
    if (hasTimingInfo && globalTicksPerSecond <= 0) {
        errors.push_back("Invalid global ticks per second: " + std::to_string(globalTicksPerSecond));
    }

    return errors;
}

} // namespace X2FBX
