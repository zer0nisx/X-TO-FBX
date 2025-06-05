#pragma once

#include "XFileData.h"
#include "Logger.h"
#include <string>
#include <vector>
#include <memory>

// FBX SDK includes (conditional compilation)
#ifdef FBXSDK_FOUND
#include <fbxsdk.h>
#endif

namespace X2FBX {

// Export result information
struct FBXExportResult {
    bool success;
    std::string outputPath;
    std::string errorMessage;

    // Statistics
    int verticesExported;
    int facesExported;
    int materialsExported;
    int bonesExported;
    int animationsExported;
    float exportTimeMs;

    FBXExportResult() : success(false), verticesExported(0), facesExported(0),
                       materialsExported(0), bonesExported(0), animationsExported(0),
                       exportTimeMs(0.0f) {}
};

// Export options
struct FBXExportOptions {
    bool exportAnimations = true;
    bool exportMaterials = true;
    bool exportTextures = true;
    bool embedTextures = false;
    bool optimizeMesh = true;
    bool validateOutput = true;

    // Coordinate system conversion
    bool convertCoordinateSystem = true;
    bool flipYZ = true;              // DirectX to FBX coordinate conversion

    // Animation settings
    bool separateAnimationFiles = true;
    float animationFrameRate = 30.0f;

    // File format
    enum class FileFormat {
        BINARY,
        ASCII
    } fileFormat = FileFormat::BINARY;

    FBXExportOptions() = default;
};

class FBXExporter {
private:
    Logger& logger_;

#ifdef FBXSDK_FOUND
    // FBX SDK objects
    FbxManager* fbxManager_;
    FbxScene* fbxScene_;
    FbxExporter* fbxExporter_;

    // Current export state
    std::vector<FbxNode*> boneNodes_;
    std::map<std::string, FbxNode*> boneNodeMap_;
#endif

    // Export statistics
    mutable FBXExportResult lastExportResult_;

public:
    FBXExporter();
    ~FBXExporter();

    // Main export functions
    FBXExportResult ExportToFBX(const XFileData& xData,
                                const std::string& outputPath,
                                const FBXExportOptions& options = FBXExportOptions());

    FBXExportResult ExportStaticMesh(const XMeshData& meshData,
                                     const std::string& outputPath,
                                     const FBXExportOptions& options = FBXExportOptions());

    FBXExportResult ExportAnimatedMesh(const XMeshData& meshData,
                                      const XAnimationSet& animation,
                                      const std::string& outputPath,
                                      const FBXExportOptions& options = FBXExportOptions());

    std::vector<FBXExportResult> ExportAllAnimations(const XMeshData& meshData,
                                                     const std::string& outputDirectory,
                                                     const std::string& baseFileName,
                                                     const FBXExportOptions& options = FBXExportOptions());

    // Validation
    bool ValidateExportedFile(const std::string& filePath) const;

    // Get last export result
    const FBXExportResult& GetLastExportResult() const { return lastExportResult_; }

    // SDK availability
    static bool IsFBXSDKAvailable();
    static std::string GetFBXSDKVersion();

private:
#ifdef FBXSDK_FOUND
    // Initialization
    bool InitializeFBX();
    void CleanupFBX();
    bool InitializeFBXSDK();
    void CleanupFBXSDK();
    bool CreateScene(const std::string& sceneName);

    // Export implementations
    FBXExportResult ExportWithFBXSDK(const XFileData& xData, const std::string& outputPath, const FBXExportOptions& options);
    bool ExportMeshes(const XFileData& xData, const FBXExportOptions& options);
    bool CreateMesh(const XMeshData& meshData, const FBXExportOptions& options);
    bool SaveFBXFile(const std::string& outputPath);
    bool ExportSeparateAnimations(const XFileData& xData, const std::string& basePath, const FBXExportOptions& options);
    bool ExportCombinedAnimations(const XFileData& xData, const FBXExportOptions& options);
#endif

    // Placeholder export when SDK not available
    FBXExportResult ExportPlaceholder(const XFileData& xData, const std::string& outputPath, const FBXExportOptions& options);

    // Mesh conversion
    FbxMesh* CreateFBXMesh(const XMeshData& meshData, const std::string& meshName);
    bool ConvertVertices(const XMeshData& meshData, FbxMesh* fbxMesh);
    bool ConvertFaces(const XMeshData& meshData, FbxMesh* fbxMesh);
    bool ConvertNormals(const XMeshData& meshData, FbxMesh* fbxMesh);
    bool ConvertUVs(const XMeshData& meshData, FbxMesh* fbxMesh);

    // Material conversion
    std::vector<FbxSurfacePhong*> ConvertMaterials(const XMeshData& meshData);
    FbxSurfacePhong* CreateFBXMaterial(const XMaterial& xMaterial);
    FbxFileTexture* CreateFBXTexture(const std::string& texturePath);

    // Skeleton conversion
    bool CreateSkeleton(const XMeshData& meshData);
    FbxNode* CreateBoneNode(const XBone& bone, FbxNode* parentNode = nullptr);
    bool ApplySkinWeights(const XMeshData& meshData, FbxMesh* fbxMesh);

    // Animation conversion
    bool CreateAnimation(const XAnimationSet& animation, float frameRate);
    FbxAnimStack* CreateAnimationStack(const XAnimationSet& animation);
    FbxAnimLayer* CreateAnimationLayer(FbxAnimStack* animStack);
    bool CreateKeyframes(const XAnimationSet& animation, FbxAnimLayer* animLayer);
    bool CreateNodeAnimation(FbxNode* node, const std::vector<XKeyframe>& keyframes, FbxAnimLayer* animLayer);

    // Coordinate system conversion
    void ConvertCoordinateSystem(FbxVector4& vector) const;
    void ConvertCoordinateSystem(FbxMatrix& matrix) const;
    FbxQuaternion ConvertQuaternion(const XQuaternion& xQuat) const;
    FbxVector4 ConvertVector3(const XVector3& xVec) const;
    FbxVector2 ConvertVector2(const XVector2& xVec) const;

    // File export
    bool ExportScene(const std::string& outputPath, const FBXExportOptions& options);
    bool ConfigureExporter(const FBXExportOptions& options);

    // Validation helpers
    bool ValidateScene() const;
    bool ValidateMesh(FbxMesh* mesh) const;
    bool ValidateAnimation(FbxAnimStack* animStack) const;

    // Utility functions
    std::string GenerateUniqueNodeName(const std::string& baseName) const;
    void LogExportStatistics() const;
};

// Utility functions for FBX export
namespace FBXUtils {
    // Coordinate system conversion utilities
    XVector3 DirectXToFBXPosition(const XVector3& dxPos);
    XQuaternion DirectXToFBXRotation(const XQuaternion& dxRot);
    XMatrix4x4 DirectXToFBXMatrix(const XMatrix4x4& dxMatrix);

    // Animation timing conversion
    double ConvertXTimeToFBXTime(float xTime, float xTicksPerSecond);
    float ConvertFBXTimeToXTime(double fbxTime, float xTicksPerSecond);

    // Validation utilities
    bool IsValidFBXFile(const std::string& filePath);
    std::string GetFBXFileInfo(const std::string& filePath);

    // File size and optimization
    size_t GetFileSize(const std::string& filePath);
    bool OptimizeFBXFile(const std::string& filePath);

    // Mesh validation
    struct MeshValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        // Statistics
        int totalVertices;
        int totalFaces;
        int degenerateTriangles;
        int duplicateVertices;
    };

    MeshValidationResult ValidateMeshGeometry(const XMeshData& meshData);

    // Animation validation
    struct AnimationValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        // Statistics
        float totalDuration;
        int totalKeyframes;
        int emptyAnimations;
        std::vector<std::string> animationNames;
    };

    AnimationValidationResult ValidateAnimationData(const std::vector<XAnimationSet>& animations);
}

} // namespace X2FBX
