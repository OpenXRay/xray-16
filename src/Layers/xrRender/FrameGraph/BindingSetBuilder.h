#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {

struct ExtractedReflection;

class BindingSetBuilder {
public:
    struct ReflectedResource {
        const char* name;
        u32 slot;
        nvrhi::ResourceType layoutType;
    };

    struct ReflectedLists {
        xr_vector<ReflectedResource> srvs;
        xr_vector<ReflectedResource> uavs;
        xr_vector<ReflectedResource> cbs;
        xr_vector<nvrhi::BindingSetItem> samplerItems;
    };

    BindingSetBuilder(const ExtractedReflection& reflection, nvrhi::IDevice* device,
                      const char* debugLabel = nullptr);

    BindingSetBuilder(const ExtractedReflection& vsReflection,
                      const ExtractedReflection& psReflection,
                      nvrhi::IDevice* device,
                      const char* debugLabel = nullptr);

    static void InvalidateReflectionCache();

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

private:
    const ReflectedLists* m_lists;

    nvrhi::BindingSetDesc m_desc;

    int FindSRVSlot(const char* name) const;
    int FindUAVSlot(const char* name) const;
    int FindCBSlot(const char* name) const;

    void AddSamplers();
};

}
