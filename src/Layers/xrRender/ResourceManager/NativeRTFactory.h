#pragma once

#include "ResourceHandle.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg {
    class RenderDevice;
}

namespace xray::render::resources {

class TextureManager;

class NativeRTFactory {
public:
    explicit NativeRTFactory(xray::render::fg::RenderDevice* device, TextureManager* textureManager);
    ~NativeRTFactory();

    TextureHandle CreatePositionBuffer(u32 width, u32 height, const char* name = "rt_Position");

    TextureHandle CreateNormalBuffer(u32 width, u32 height, const char* name = "rt_Normal");

    TextureHandle CreateAlbedoBuffer(u32 width, u32 height, bool sRGB = false, const char* name = "rt_Albedo");

    TextureHandle CreateDepthStencil(u32 width, u32 height, bool use32bit = false, const char* name = "rt_Depth");

    TextureHandle CreateGBufferRT(u32 width, u32 height, nvrhi::Format format, const char* name);

    TextureHandle CreateLDRTarget(u32 width, u32 height, const char* name = "rt_LDR");

    TextureHandle CreateHistoryBuffer(u32 width, u32 height, nvrhi::Format format, const char* name = "rt_History");

    TextureHandle CreateVelocityBuffer(u32 width, u32 height, const char* name = "rt_Velocity");

    TextureHandle CreateShadowMap(u32 size, bool use32bit = false, const char* name = "rt_Shadow");

    TextureHandle CreateCascadedShadowMap(u32 size, u32 cascadeCount, bool use32bit = false, const char* name = "rt_CascadedShadow");

    TextureHandle CreateComputeTarget(u32 width, u32 height, nvrhi::Format format, const char* name = "rt_Compute");

    TextureHandle CreateVolumeTarget(u32 width, u32 height, u32 depth, nvrhi::Format format, const char* name = "rt_Volume");

    struct RTSetDesc {
        u32 width;
        u32 height;
        bool includePosition;
        bool includeNormal;
        bool includeAlbedo;
        bool includeDepth;
        bool includeVelocity;
        bool sRGB;
    };

    struct RTSet {
        TextureHandle position;
        TextureHandle normal;
        TextureHandle albedo;
        TextureHandle depth;
        TextureHandle velocity;
    };

    RTSet CreateGBufferSet(const RTSetDesc& desc);

    void ResizeRenderTargets(u32 newWidth, u32 newHeight);

    void ReleaseRenderTarget(TextureHandle& handle);

    struct Statistics {
        u32 numRenderTargets;
        u64 totalMemoryUsed;
        u32 numResizes;
    };

    Statistics GetStatistics() const;
    void PrintStatistics() const;

private:
    TextureHandle CreateRenderTargetInternal(
        u32 width,
        u32 height,
        u32 depth,
        u32 arraySize,
        u32 mipLevels,
        nvrhi::Format format,
        bool isDepthStencil,
        bool isUAV,
        const char* debugName
    );

    struct ManagedRT {
        TextureHandle handle;
        u32 width;
        u32 height;
        nvrhi::Format format;
        bool isDepthStencil;
        bool isUAV;
        shared_str name;
    };

    xray::render::fg::RenderDevice* m_device;
    TextureManager* m_textureManager;
    xr_vector<ManagedRT> m_managedRTs;

    mutable Statistics m_stats;
};

}
