#include "stdafx.h"
#include "BindingLayoutBuilder.h"
#include "ShaderCache.h"

namespace xray::render::framegraph {

static nvrhi::BindingLayoutItem MakeSRVItem(u32 slot, ResourceShape shape)
{
    switch (shape) {
    case ResourceShape::StructuredBuffer:
        return nvrhi::BindingLayoutItem::StructuredBuffer_SRV(slot);
    case ResourceShape::RawBuffer:
        return nvrhi::BindingLayoutItem::RawBuffer_SRV(slot);
    case ResourceShape::AccelStruct:
        return nvrhi::BindingLayoutItem::RayTracingAccelStruct(slot);
    default:
        return nvrhi::BindingLayoutItem::Texture_SRV(slot);
    }
}

static nvrhi::BindingLayoutItem MakeUAVItem(u32 slot, ResourceShape shape)
{
    switch (shape) {
    case ResourceShape::StructuredBuffer:
        return nvrhi::BindingLayoutItem::StructuredBuffer_UAV(slot);
    case ResourceShape::RawBuffer:
        return nvrhi::BindingLayoutItem::RawBuffer_UAV(slot);
    default:
        return nvrhi::BindingLayoutItem::Texture_UAV(slot);
    }
}

static void AddBindingsFromReflection(
    const ExtractedReflection& reflection,
    nvrhi::BindingLayoutDesc& desc)
{
    for (const auto& cb : reflection.constantLayout.constantBuffers.buffers)
        desc.bindings.push_back(nvrhi::BindingLayoutItem::VolatileConstantBuffer(cb.slot));

    for (const auto& tex : reflection.rtBindings.inputTextures)
        desc.bindings.push_back(MakeSRVItem(tex.slot, tex.shape));

    for (const auto& uav : reflection.rtBindings.uavBindings)
        desc.bindings.push_back(MakeUAVItem(uav.slot, uav.shape));

    for (const auto& smp : reflection.rtBindings.samplers)
        desc.bindings.push_back(nvrhi::BindingLayoutItem::Sampler(smp.slot));
}

static int GetRegisterClass(nvrhi::ResourceType type)
{
    switch (type) {
    case nvrhi::ResourceType::Texture_SRV:
    case nvrhi::ResourceType::TypedBuffer_SRV:
    case nvrhi::ResourceType::StructuredBuffer_SRV:
    case nvrhi::ResourceType::RawBuffer_SRV:
    case nvrhi::ResourceType::RayTracingAccelStruct:
        return 0;
    case nvrhi::ResourceType::Texture_UAV:
    case nvrhi::ResourceType::TypedBuffer_UAV:
    case nvrhi::ResourceType::StructuredBuffer_UAV:
    case nvrhi::ResourceType::RawBuffer_UAV:
        return 1;
    case nvrhi::ResourceType::ConstantBuffer:
    case nvrhi::ResourceType::VolatileConstantBuffer:
        return 2;
    case nvrhi::ResourceType::Sampler:
        return 3;
    default:
        return -1;
    }
}

static bool IsBufferType(nvrhi::ResourceType type)
{
    switch (type) {
    case nvrhi::ResourceType::StructuredBuffer_SRV:
    case nvrhi::ResourceType::RawBuffer_SRV:
    case nvrhi::ResourceType::StructuredBuffer_UAV:
    case nvrhi::ResourceType::RawBuffer_UAV:
        return true;
    default:
        return false;
    }
}

static void DeduplicateBindings(nvrhi::BindingLayoutDesc& desc)
{
    auto& b = desc.bindings;
    for (size_t i = 0; i < b.size(); ++i)
    {
        for (size_t j = i + 1; j < b.size(); )
        {
            if (GetRegisterClass(b[i].type) == GetRegisterClass(b[j].type) && b[i].slot == b[j].slot)
            {
                if (!IsBufferType(b[i].type) && IsBufferType(b[j].type))
                {
                    b.erase(b.begin() + i);
                    --i;
                    break;
                }
                else
                {
                    b.erase(b.begin() + j);
                }
            }
            else
                ++j;
        }
    }
}

nvrhi::BindingLayoutDesc BindingLayoutBuilder::Build(
    const ExtractedReflection& reflection,
    nvrhi::ShaderType visibility)
{
    nvrhi::BindingLayoutDesc desc;
    desc.visibility = visibility;

    AddBindingsFromReflection(reflection, desc);
    DeduplicateBindings(desc);

    return desc;
}

nvrhi::BindingLayoutDesc BindingLayoutBuilder::Build(
    const ExtractedReflection& vsReflection,
    const ExtractedReflection& psReflection,
    nvrhi::ShaderType visibility)
{
    nvrhi::BindingLayoutDesc desc;
    desc.visibility = visibility;

    AddBindingsFromReflection(vsReflection, desc);
    AddBindingsFromReflection(psReflection, desc);

    DeduplicateBindings(desc);

    return desc;
}

}
