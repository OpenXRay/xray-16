#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/LightingMode.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "ClusterLightPassSetup.h"
#include <memory>

namespace xray::render::framegraph
{
class FrameGraph;
}

namespace xray::render::fg::passes
{
class PathTracerSnapshotGlobals;

class PathTracerConfig
{
public:
    u32 maxBounces = 8;
    u32 diffuseMode = 0;
    u32 diagnosticMode = 0;
    u32 maxNullEvents = 256;
    u32 maxSamples = 0;
    float sunAngularRadius = 0.0f;
    bool freezeScene = false;
};

class PathTracerOutput
{
public:
    framegraph::VirtualResourceHandle composited;
};

class PathTracerCB
{
public:
    Fmatrix invViewProj;
    Fvector4 cameraPos_pad;
    Fvector4 sunDir_intensity;
    Fvector4 sunColor_skyWeight;
    float screenWidth;
    float screenHeight;
    u32 sampleIndex;
    u32 maxBounces;
    u32 identityStaticCount;
    u32 terrainBatchCount;
    u32 transparentBatchCount;
    u32 skinnedBatchStart;
    u32 grassBatchStart;
    u32 detailAtlasIndex;
    u32 diffuseMode;
    u32 lightCount;
    u32 diagnosticMode;
    u32 maxNullEvents;
    u32 emissiveCount;
    u32 maxSamples;
    float environmentRotation;
    float sunAngularRadius;
    float cameraConeSpread;
    float transportPad;
    u32 detailMeshBatchStart;
    u32 staticDetailBatchStart;
    u32 detailPbrIndex;
    u32 detailBumpIndex;
};

static_assert(sizeof(PathTracerCB) == 208);

class PathTracerSnapshotStats
{
public:
    bool freezeRequested = false;
    bool frozen = false;
    bool capturePending = false;
    bool valid = false;
    bool sceneValid = false;
    bool fallback = false;
    bool cdfActive = false;
    u32 captureFrame = 0;
    u32 captures = 0;
    u32 failedCaptures = 0;
    u32 capturedTextures = 0;
    u32 capturedIndices = 0;
    u64 capturedTextureBytes = 0;
    u32 capturedBuffers = 0;
    u64 capturedBufferBytes = 0;
    u32 retainedSceneBuffers = 0;
    u64 retainedSceneBytes = 0;
    u32 lightCount = 0;
    u32 emissiveCount = 0;
    u64 sceneRevision = 0;
    u64 textureRevision = 0;
    u64 lightingSignature = 0;
};

class PathTracerSnapshot
{
public:
    ~PathTracerSnapshot();

    std::shared_ptr<RTSceneGeneration> scene;
    std::shared_ptr<PathTracerSnapshotGlobals> globals;
    nvrhi::BufferHandle lightData;
    nvrhi::BufferHandle staticGlobals;
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
    nvrhi::DescriptorTableHandle textureTable;
    xr_vector<nvrhi::TextureHandle> textureClones;
    PathTracerCB world = {};
    float skyBlend = 0.0f;
    u64 sceneRevision = 0;
    u64 textureRevision = 0;
    u64 lightingSignature = 0;
    nvrhi::IDevice* device = nullptr;
    u32 captureFrame = 0;
    u32 pipelineRevision = 0;
    bool captured = false;
    bool submitted = false;
    bool promoted = false;
    PathTracerSnapshotStats stats;
};

class PathTracerHistory
{
public:
    bool MatchesCamera(const Fmatrix& view, const Fmatrix& project, const Fvector& position) const;

    Fmatrix cameraView = Fidentity;
    Fmatrix cameraProject = Fidentity;
    PathTracerCB parameters = {};
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
    u64 sceneRevision = 0;
    u64 textureRevision = 0;
    u64 lightingSignature = 0;
    Fvector4 foliageSSS = {};
    Fvector4 foliageParams = {};
    Fvector4 foliageParams2 = {};
    u32 samples = 0;
    bool valid = false;
    std::shared_ptr<PathTracerSnapshot> snapshot;
    bool capturedSnapshot = false;
};

class PathTracerPassState
{
public:
    nvrhi::TextureHandle accumulation;
    u32 width = 0;
    u32 height = 0;
    PathTracerHistory history;
    PathTracerHistory pending;
    std::shared_ptr<PathTracerSnapshot> snapshot;
    PathTracerSnapshotStats snapshotStats;
};

class PathTracerSnapshotReuse
{
public:
    bool reusable = false;
    bool invalid = false;
};

class PathTracerCaptureData
{
public:
    RenderDevice* device = nullptr;
    PathTracerSnapshot* snapshot = nullptr;
    nvrhi::IBuffer* lightDataSource = nullptr;
    nvrhi::IBuffer* lightDataTarget = nullptr;
    xr_vector<nvrhi::ITexture*> textureSources;
    xr_vector<nvrhi::TextureHandle> textureTargets;
};

class PathTracerData
{
public:
    RenderDevice* device = nullptr;
    LightingFrameState* lighting = nullptr;
    PathTracerPassState* state = nullptr;
    RTFrameResources scene;
    framegraph::VirtualResourceHandle outputTex;
    framegraph::VirtualResourceHandle accumulation;
    framegraph::VirtualResourceHandle lightData;
    framegraph::VirtualResourceHandle environmentCdf;
    PathTracerCB cbData;
    Fmatrix cameraView = Fidentity;
    Fmatrix cameraProject = Fidentity;
    u32 width = 0;
    u32 height = 0;
    u32 sampleCount = 0;
    u32 staticDetailInstanceCount = 0;
    nvrhi::TextureHandle sky0;
    nvrhi::TextureHandle sky1;
    nvrhi::DescriptorTableHandle textureTable;
    std::shared_ptr<PathTracerSnapshot> snapshot;
    nvrhi::IBuffer* staticGlobals = nullptr;
    std::shared_ptr<PathTracerSnapshotGlobals> globals;
};

LightingFallback EnsurePathTracerResources(RenderDevice* device, u32 width, u32 height, PathTracerPassState& state);
PathTracerSnapshotReuse EvaluatePathTracerSnapshotReuse(RenderDevice* device, RTAccelStructManager* accelMgr,
    PathTracerPassState& state, bool freezeRequested);
PathTracerOutput setupPathTracerPass(framegraph::FrameGraph& fg, RenderDevice* device, RTAccelStructManager* accelMgr,
    framegraph::VirtualResourceHandle sceneColorIn, const ClusterLightOutput& clusterLights,
    LightingFrameState& lighting, const PathTracerConfig& config, const Fmatrix& view, const Fmatrix& project,
    const Fmatrix& invViewProj, const Fvector& cameraPos, u32 width, u32 height, PathTracerPassState& state);

void DiscardPathTracerSnapshot(PathTracerPassState& state);
void ShutdownPathTracer();
}
