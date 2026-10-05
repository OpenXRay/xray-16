#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>
#include "SkyVisibilityDebug.h"

namespace xray::render::fg
{
class RenderDevice;

class SkyVisibilityLayout
{
public:
    Fvector origin = {};
    float spacing = 0.0f;
    u32 dims[3] = {};
    u32 count = 0;
    u32 rays = 0;
    float rayDistance = 0.0f;
    float backfaceLimit = 0.0f;

    bool Matches(const SkyVisibilityLayout& other) const;
};

enum class SkyVisibilityState : u8
{
    Empty,
    Uploading,
    Baking,
    Saving,
    Ready
};

enum class SkyProbeDebugStatus : u8
{
    Unavailable,
    NoData,
    Live,
    Frozen,
    Pending,
    Invalid,
    Error
};

class SkyProbeDebugReportContext
{
public:
    SkyVisibilityLayout layout;
    SkyVisibilityState gridState = SkyVisibilityState::Empty;
    u32 bakeNext = 0;
    u32 currentFrame = 0;
    u32 readbackFrame = 0;
    u32 captureIndex = 0;
    u32 readbackCount = 0;
    u32 cacheVersion = 0;
    u32 probeBytes = 0;
    bool frozen = false;
    bool explicitCapture = false;
    xr_string statusText;
    xr_string cachePath;
    xr_string backendName;
    u64 geometryStamp = 0;
    xr_string issues;
    xr_string warnings;
};

class SkyProbeDebugReport
{
public:
    static xr_string Build(const SkyProbeDebugSnapshot& packet, const SkyProbeDebugReportContext& context);
    static void Validate(const SkyProbeDebugSnapshot& packet, const SkyVisibilityLayout& layout, xr_string& issues,
        xr_string& warnings);
    static const char* SurfaceStatusName(u32 status);
    static const char* CornerStateName(u32 state);
    static const char* RayStatusName(u32 status);
    static char RayStatusLetter(u32 status);
    static const char* RayLabel(u32 ray);
    static const char* SceneStatusName(u32 status);
    static const char* GridStateName(SkyVisibilityState state);
    static const char* StatusName(SkyProbeDebugStatus status);
};

class SkyVisibilityGrid
{
public:
    static constexpr u32 kProbeBytes = 48;

    void Initialize(RenderDevice* device);
    void Shutdown();
    void BeginLevel(const Fbox& bounds, u64 geometryStamp);
    void EndLevel();
    void RequestRebake();
    void Update();
    bool NeedsScene() const;
    bool HasPendingWork() const;
    bool TakeBakeSlice(u32 budget, u32& first, u32& count) const;
    void RecordPending(nvrhi::ICommandList* commandList);
    void CommitBakeSlice(nvrhi::ICommandList* commandList, u32 first, u32 count);
    const SkyVisibilityLayout& GetLayout() const;
    nvrhi::IBuffer* GetBuffer() const;
    SkyVisibilityState GetState() const;

    bool PrepareDebugSnapshot();
    bool NeedsDebugSnapshot() const;
    bool HasDebugSnapshot() const;
    nvrhi::IBuffer* GetDebugBuffer() const;
    nvrhi::IBuffer* GetDebugReadbackBuffer() const;
    void RecordDebugSnapshot(nvrhi::ICommandList* commandList);
    void ReportDebugFailure(const char* reason);
    void RequestDebugCapture();
    void ResetDebugSnapshots();
    void DrawDebugInspector();

private:
    SkyVisibilityLayout BuildLayout() const;
    bool Allocate(const SkyVisibilityLayout& layout);
    void StartBake();
    bool LoadCache(const SkyVisibilityLayout& layout);
    void SaveCache(const void* data, u64 bytes) const;
    void PollReadback();
    void ReleaseReadback();
    void ConsumeDebugControls();
    void PollDebugReadback();
    void ReleaseDebugReadback();
    void ExportDebugCapture();
    void FailDebugCapture(const char* reason);
    SkyProbeDebugStatus GetDebugStatus() const;
    SkyProbeDebugReportContext BuildDebugContext(bool explicitCapture) const;
    void DrawDebugSurface(const SkyProbeDebugSnapshot& packet);
    void DrawDebugCorners(const SkyProbeDebugSnapshot& packet);
    void DrawDebugLabels(const SkyProbeDebugSnapshot& packet) const;
    void DrawDebugLegend() const;

    RenderDevice* m_device = nullptr;
    SkyVisibilityLayout m_layout;
    SkyVisibilityState m_state = SkyVisibilityState::Empty;
    nvrhi::BufferHandle m_buffer;
    nvrhi::BufferHandle m_readback;
    xr_vector<u8> m_upload;
    Fbox m_bounds = {};
    u64 m_geometryStamp = 0;
    string_path m_cachePath = {};
    CTimer m_bakeTimer;
    u64 m_readbackLease = 0;
    u32 m_bakeNext = 0;
    u32 m_progressStep = 0;
    bool m_clearPending = false;
    bool m_rebakeRequested = false;
    bool m_levelActive = false;

    nvrhi::BufferHandle m_debugBuffer;
    nvrhi::BufferHandle m_debugReadback;
    SkyProbeDebugSnapshot m_debugPacket;
    xr_string m_debugError;
    xr_string m_debugIssues;
    xr_string m_debugWarnings;
    xr_string m_debugExportStatus;
    xr_string m_debugReport;
    u64 m_debugLease = 0;
    u32 m_debugRecordFrame = 0;
    u32 m_debugRequestFrame = 0;
    u32 m_debugReadbackFrame = 0;
    u32 m_debugReadbackCount = 0;
    u32 m_debugCaptureCount = 0;
    u32 m_debugFailures = 0;
    bool m_debugGpuValid = false;
    bool m_debugDecoded = false;
    bool m_debugDesync = false;
    bool m_debugFailed = false;
    bool m_debugAllocFailed = false;
    bool m_debugPendingCapture = false;
    bool m_debugCaptureRequested = false;
    bool m_debugShowRaw = false;
};
}
