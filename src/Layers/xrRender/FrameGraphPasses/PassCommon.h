#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

namespace xray::render::fg::passes {

inline void DrawIndexedIndirectCountOrFallback(
    nvrhi::ICommandList* cmdList,
    uint32_t paramOffsetBytes,
    uint32_t countOffsetBytes,
    uint32_t maxDrawCount)
{
#if defined(XR_PLATFORM_APPLE)
    (void)countOffsetBytes;
    cmdList->drawIndexedIndirect(paramOffsetBytes, maxDrawCount);
#else
    cmdList->drawIndexedIndirectCount(paramOffsetBytes, countOffsetBytes, maxDrawCount);
#endif
}

struct LightingConstants {
    Fvector4 sunDirection;
    Fvector4 sunColor;
    Fvector4 ambientColor;
    Fvector4 cameraPosition;
    Fvector4 fogParams;
    Fvector4 fogColor;
};
static_assert(sizeof(LightingConstants) == 96, "LightingConstants must be 96 bytes");

inline nvrhi::VertexAttributeDesc* GetUnifiedVertexAttributes(u32& outCount)
{
    static nvrhi::VertexAttributeDesc attrs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
            .setBufferIndex(0).setOffset(0).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(12).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(16).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(20).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)
            .setArraySize(2).setBufferIndex(0).setOffset(24).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(40).setElementStride(48),
        nvrhi::VertexAttributeDesc()
            .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
            .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true),
    };
    outCount = 7;
    return attrs;
}

inline void QueryBindingLayoutFromPipeline(
    nvrhi::IGraphicsPipeline* pipeline,
    nvrhi::BindingLayoutHandle& outLayout)
{
    const auto& desc = pipeline->getDesc();
    if (!desc.bindingLayouts.empty())
        outLayout = desc.bindingLayouts[0];
}

nvrhi::BufferHandle GetOrCreateDrawIndexBuffer(const char* passName, nvrhi::IDevice* device);

LightingConstants FillLightingConstants();

u32 ExtractFrustumPlanes(Fvector4 outPlanes[6]);

} // namespace xray::render::fg::passes
