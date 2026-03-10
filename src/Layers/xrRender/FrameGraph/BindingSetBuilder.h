#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {

struct ExtractedReflection;

class BindingSetBuilder {
public:
    BindingSetBuilder(const ExtractedReflection& reflection, nvrhi::IDevice* device);

    BindingSetBuilder(const ExtractedReflection& vsReflection,
                      const ExtractedReflection& psReflection,
                      nvrhi::IDevice* device);

    BindingSetBuilder& Texture(const char* name, nvrhi::ITexture* texture,
                               nvrhi::Format format = nvrhi::Format::UNKNOWN,
                               nvrhi::TextureSubresourceSet subresources = nvrhi::AllSubresources);

    BindingSetBuilder& TextureUAV(const char* name, nvrhi::ITexture* texture,
                                   nvrhi::Format format = nvrhi::Format::UNKNOWN,
                                   nvrhi::TextureSubresourceSet subresources = nvrhi::AllSubresources);

    BindingSetBuilder& BufferSRV(const char* name, nvrhi::IBuffer* buffer);
    BindingSetBuilder& BufferUAV(const char* name, nvrhi::IBuffer* buffer);
    BindingSetBuilder& ConstantBuffer(const char* name, nvrhi::IBuffer* buffer);
    BindingSetBuilder& AccelStruct(const char* name, nvrhi::rt::IAccelStruct* as);

    BindingSetBuilder& TextureSlot(u32 slot, nvrhi::ITexture* texture,
                                    nvrhi::Format format = nvrhi::Format::UNKNOWN,
                                    nvrhi::TextureSubresourceSet subresources = nvrhi::AllSubresources);
    BindingSetBuilder& TextureUAVSlot(u32 slot, nvrhi::ITexture* texture,
                                       nvrhi::Format format = nvrhi::Format::UNKNOWN,
                                       nvrhi::TextureSubresourceSet subresources = nvrhi::AllSubresources);
    BindingSetBuilder& BufferSRVSlot(u32 slot, nvrhi::IBuffer* buffer);
    BindingSetBuilder& BufferUAVSlot(u32 slot, nvrhi::IBuffer* buffer);
    BindingSetBuilder& ConstantBufferSlot(u32 slot, nvrhi::IBuffer* buffer);

    nvrhi::BindingSetDesc Build();

public:
    struct ReflectedResource {
        const char* name;
        u32 slot;
        nvrhi::ResourceType layoutType;
    };

private:
    xr_vector<ReflectedResource> m_srvs;
    xr_vector<ReflectedResource> m_uavs;
    xr_vector<ReflectedResource> m_cbs;
    xr_vector<ReflectedResource> m_samplers;

    nvrhi::BindingSetDesc m_desc;
    nvrhi::IDevice* m_device;

    int FindSRVSlot(const char* name) const;
    int FindUAVSlot(const char* name) const;
    int FindCBSlot(const char* name) const;

    void CollectReflection(const ExtractedReflection& reflection);
    void AddSamplers();
};

}
