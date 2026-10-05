#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

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

private:
    SkyVisibilityLayout BuildLayout() const;
    bool Allocate(const SkyVisibilityLayout& layout);
    void StartBake();
    bool LoadCache(const SkyVisibilityLayout& layout);
    void SaveCache(const void* data, u64 bytes) const;
    void PollReadback();
    void ReleaseReadback();

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
};
}
