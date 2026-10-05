#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/fgUIShader.h"
#include "Layers/xrRender/UIGeometryBatch.h"
#include "Layers/xrRender/FVisual.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/FProgressive.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RCShader.h"
#include "Layers/xrRender/RenderContext/RenderStateConversion.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "xrEngine/IRenderBackend.h"
#include "Layers/xrRender/Blender_CLSID.h"
#include "Layers/xrRender/blenders/Blender_BmmD.h"
#include "Layers/xrRender/r_constants.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/Materials/ShaderInfo.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "xrEngine/xr_object.h"


namespace xray::render {

using namespace xray::render::fg;




#ifdef DEBUG
static void LogConstantLayout(const framegraph::ShaderConstantLayout& layout, const char* shaderName) {
    using namespace framegraph;
    return;

    for (const auto& constant : layout.constants) {
        if (constant.cbIndex >= layout.constantBuffers.buffers.size()) {
            Msg("    - %s: ERROR - invalid cbIndex %u", constant.name.c_str(), constant.cbIndex);
            continue;
        }
    }
}
#endif


framegraph::ShaderConstantLayout MergeConstantLayouts(
    const framegraph::ShaderConstantLayout& vsLayout,
    const framegraph::ShaderConstantLayout& psLayout)
{
    using namespace framegraph;

    ShaderConstantLayout merged;


    struct CBDeduplicationInfo {
        shared_str name;
        u32 vsSlot = UINT32_MAX;
        u32 psSlot = UINT32_MAX;
        u32 size = 0;
        u16 mergedIndex = 0;
    };

    xr_map<shared_str, CBDeduplicationInfo> uniqueCBs;

    for (const auto& cb : vsLayout.constantBuffers.buffers) {
        auto& unique = uniqueCBs[cb.name];
        unique.name = cb.name;
        unique.vsSlot = cb.slot;
        unique.size = std::max(unique.size, cb.size);
    }

    for (const auto& cb : psLayout.constantBuffers.buffers) {
        auto& unique = uniqueCBs[cb.name];
        unique.name = cb.name;
        unique.psSlot = cb.slot;
        unique.size = std::max(unique.size, cb.size);
    }

    u16 mergedIndex = 0;
    for (auto& [cbName, cbInfo] : uniqueCBs) {
        cbInfo.mergedIndex = mergedIndex++;

        ConstantBufferInfo mergedCB;
        mergedCB.name = cbInfo.name;
        mergedCB.size = cbInfo.size;
        mergedCB.slot = (cbInfo.vsSlot != UINT32_MAX) ? cbInfo.vsSlot : cbInfo.psSlot;

        merged.constantBuffers.buffers.push_back(mergedCB);
    }


    xr_map<u16, u16> vsIndexToMerged;

    for (u16 vsIdx = 0; vsIdx < vsLayout.constantBuffers.buffers.size(); ++vsIdx) {
        const auto& vsCB = vsLayout.constantBuffers.buffers[vsIdx];

        auto it = uniqueCBs.find(vsCB.name);
        if (it != uniqueCBs.end()) {
            vsIndexToMerged[vsIdx] = it->second.mergedIndex;
        }
    }


    xr_map<u16, u16> psIndexToMerged;

    for (u16 psIdx = 0; psIdx < psLayout.constantBuffers.buffers.size(); ++psIdx) {
        const auto& psCB = psLayout.constantBuffers.buffers[psIdx];

        auto it = uniqueCBs.find(psCB.name);
        if (it != uniqueCBs.end()) {
            psIndexToMerged[psIdx] = it->second.mergedIndex;
        }
    }


    for (const auto& vsConstant : vsLayout.constants) {
        ShaderConstant mergedConstant = vsConstant;

        auto it = vsIndexToMerged.find(vsConstant.cbIndex);
        if (it != vsIndexToMerged.end()) {
            mergedConstant.cbIndex = it->second;
        }
        else {
            Msg("! [MergeConstantLayouts] WARNING: VS constant '%s' has unmapped cbIndex %u",
                vsConstant.name.c_str(), vsConstant.cbIndex);
        }

        merged.constants.push_back(mergedConstant);
    }


    for (const auto& psConstant : psLayout.constants) {
        bool isDuplicate = false;
        for (const auto& existingConstant : merged.constants) {
            if (existingConstant.name == psConstant.name) {
                isDuplicate = true;

                if (existingConstant.offset != psConstant.offset ||
                    existingConstant.size != psConstant.size) {
                    Msg("! [MergeConstantLayouts] WARNING: Constant '%s' differs between VS and PS",
                        psConstant.name.c_str());
                    Msg("    VS: offset=%u, size=%u", existingConstant.offset, existingConstant.size);
                    Msg("    PS: offset=%u, size=%u", psConstant.offset, psConstant.size);
                }

                break;
            }
        }

        if (isDuplicate) {
            continue;
        }

        ShaderConstant mergedConstant = psConstant;

        auto it = psIndexToMerged.find(psConstant.cbIndex);
        if (it != psIndexToMerged.end()) {
            mergedConstant.cbIndex = it->second;
        }
        else {
            Msg("! [MergeConstantLayouts] WARNING: PS constant '%s' has unmapped cbIndex %u",
                psConstant.name.c_str(), psConstant.cbIndex);
        }

        merged.constants.push_back(mergedConstant);
    }

    return merged;
}


MaterialCache::MaterialCache(
    fg::RenderDevice* device,
    resources::FGResourceManager* resourceManager,
    framegraph::VolatileConstantBufferPool* vcbPool)
    : m_device(device)
    , m_resourceManager(resourceManager)
    , m_vcbPool(vcbPool)
{
    VERIFY(m_device);
    VERIFY(m_resourceManager);
}

MaterialCache::~MaterialCache()
{
    FlushAlphaRefCache();
    Clear();
    ReleaseDeveloperMaterials();
    ReleaseMaterialTextures();
}

u32 MaterialCache::RegisterMaterialTexture(nvrhi::ITexture* texture)
{
    if (!texture)
        return fg::bindless::INVALID_TEXTURE_INDEX;
    const auto found = m_bindlessTextures.find(texture);
    if (found != m_bindlessTextures.end())
        return found->second;
    R_ASSERT(GEnv.Backend);
    R_ASSERT(!m_textureBackend || m_textureBackend == GEnv.Backend);
    m_textureBackend = GEnv.Backend;
    const u32 index = m_textureBackend->RegisterBindlessTexture(texture);
    if (index != fg::bindless::INVALID_TEXTURE_INDEX)
    {
        m_bindlessTextures.emplace(texture, index);
        m_bindlessTextureIndices.push_back(index);
    }
    return index;
}

u32 MaterialCache::AcquireMaterialTexture(const char* name, fg::TextureColorSpace colorSpace)
{
    if (!name || !name[0])
        return fg::bindless::INVALID_TEXTURE_INDEX;

    const auto key = std::make_pair(shared_str(name), colorSpace);
    const auto found = m_materialTextureIndices.find(key);
    if (found != m_materialTextureIndices.end())
        return found->second;

    u32 index = fg::bindless::INVALID_TEXTURE_INDEX;
    resources::TextureManager* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (texManager)
    {
        const resources::TextureHandle handle = texManager->LoadTexture(name, colorSpace);
        if (handle.IsValid())
        {
            index = RegisterMaterialTexture(texManager->GetNVRHITexture(handle));
            if (index != fg::bindless::INVALID_TEXTURE_INDEX)
                m_materialTextureHandles.push_back(handle);
            else
                texManager->Release(handle);
        }
    }

    m_materialTextureIndices.emplace(key, index);
    return index;
}

u32 MaterialCache::AcquireAlbedoTexture(const char* name)
{
    const u32 index = AcquireMaterialTexture(name, fg::TextureColorSpace::Srgb);
    if (index != fg::bindless::INVALID_TEXTURE_INDEX)
        return index;
    return AcquireMaterialTexture("ed" DELIMITER "ed_not_existing_texture", fg::TextureColorSpace::Srgb);
}

u32 MaterialCache::AcquireLightmapTexture(const shared_str& name)
{
    return AcquireMaterialTexture(name.c_str(), fg::TextureColorSpace::Linear);
}

void MaterialCache::ReleaseMaterialTextures()
{
    if (m_textureBackend && !m_bindlessTextureIndices.empty())
        m_textureBackend->ReleaseBindlessTextures(m_bindlessTextureIndices.data(), u32(m_bindlessTextureIndices.size()));
    m_bindlessTextureIndices.clear();
    m_bindlessTextures.clear();

    resources::TextureManager* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (texManager)
    {
        for (const resources::TextureHandle& handle : m_materialTextureHandles)
            texManager->Release(handle);
        for (const auto& [name, handle] : m_textureHandleCache)
            texManager->Release(handle);
    }
    m_materialTextureHandles.clear();
    m_materialTextureIndices.clear();
    m_textureHandleCache.clear();
}

void MaterialCache::ReleaseLevelMaterials()
{
    ReleaseDeveloperMaterials();
    ReleaseMaterialTextures();

    fg::bindless::MaterialBuffer::Instance().Reset();
    fg::bindless::TerrainMaterialBuffer::Instance().Reset();
    fg::bindless::VariantTextureBuffer::Instance().Reset();

    m_materialIDByNames.clear();
    m_particleTextureToMaterialID.clear();
    m_decalTextureToMaterialID.clear();
    m_shaderToTerrainMaterialID.clear();
    m_pendingMaterials.clear();
    m_pendingTerrainMaterials.clear();
    m_detailScaleCache.clear();
    ++m_visualMaterialEpoch;
}

u32 MaterialCache::RegisterDecalMaterial(const shared_str& textureName)
{
    using namespace fg::bindless;

    if (!textureName.size())
        return UINT32_MAX;

    const auto found = m_decalTextureToMaterialID.find(textureName);
    if (found != m_decalTextureToMaterialID.end())
        return found->second;

    u32 materialID = UINT32_MAX;
    const u32 diffuseIndex = AcquireMaterialTexture(textureName.c_str(), fg::TextureColorSpace::Srgb);
    if (diffuseIndex != INVALID_TEXTURE_INDEX)
    {
        MaterialData material = {};
        material.diffuseIndex = diffuseIndex;
        material.normalIndex = INVALID_TEXTURE_INDEX;
        material.detailIndex = INVALID_TEXTURE_INDEX;
        material.pbrIndex = INVALID_TEXTURE_INDEX;
        material.detailScale = 1.0f;
        material.alphaRef = 0.0f;
        material.flags = 0;
        material.shaderVariant = 0;
        materialID = MaterialBuffer::Instance().RegisterMaterial(material);
    }

    m_decalTextureToMaterialID.emplace(textureName, materialID);
    return materialID;
}

void MaterialCache::FlushAlphaRefCache()
{
    if (m_alphaRefByTexture.empty())
        return;

    string_path cachePath;
    FS.update_path(cachePath, "$app_data_root$", "pbr_cache" DELIMITER "alpha_ref.cache");

    xr_map<shared_str, u32> merged;

    if (FS.exist(cachePath)) {
        IReader* reader = FS.r_open(cachePath);
        if (reader) {
            u32 count = reader->r_u32();
            for (u32 i = 0; i < count && !reader->eof(); i++) {
                shared_str name;
                reader->r_stringZ(name);
                u32 aref = reader->r_u32();
                merged[name] = aref;
            }
            FS.r_close(reader);
        }
    }

    for (const auto& [tex, aref] : m_alphaRefByTexture) {
        auto it = merged.find(tex);
        if (it == merged.end())
            merged[tex] = aref;
        else
            it->second = std::max(it->second, aref);
    }

    IWriter* writer = FS.w_open(cachePath);
    if (!writer) {
        Msg("! [MaterialCache] Failed to write alpha ref cache: %s", cachePath);
        return;
    }

    writer->w_u32(static_cast<u32>(merged.size()));
    for (const auto& [tex, aref] : merged) {
        writer->w_stringZ(tex);
        writer->w_u32(aref);
    }
    FS.w_close(writer);

    Msg("* [MaterialCache] Flushed alpha ref cache: %u entries -> %s",
        static_cast<u32>(merged.size()), cachePath);
}


void MaterialCache::CreateBindingLayouts(MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    if (GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
    {
        matPSO->vsBindingLayout = CreateCombinedBindingLayout(matPSO);
        matPSO->psBindingLayout = nullptr;
    }
    else
    {
        matPSO->vsBindingLayout = CreateStageBindingLayout(
            matPSO, MaterialPSO::ShaderStage::Vertex, nvrhi::ShaderType::Vertex);
        matPSO->psBindingLayout = CreateStageBindingLayout(
            matPSO, MaterialPSO::ShaderStage::Pixel, nvrhi::ShaderType::Pixel);
    }
}


nvrhi::BindingLayoutHandle MaterialCache::CreateStageBindingLayout(
    const MaterialPSO* matPSO,
    MaterialPSO::ShaderStage stage,
    nvrhi::ShaderType nvrhiStage)
{
    VERIFY(matPSO);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhiStage;

    const char* stageName = (stage == MaterialPSO::ShaderStage::Vertex) ? "VS" : "PS";

    struct CBBinding {
        u32 slot;
        bool isVCB;
        shared_str name;
        u32 size;
    };
    xr_vector<CBBinding> allCBs;

    if (stage == MaterialPSO::ShaderStage::Vertex) {
        for (const auto& vcbReq : matPSO->vcbRequirements) {
            allCBs.push_back({vcbReq.slot, true, vcbReq.name, vcbReq.size});
        }
    }

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == stage) {
            allCBs.push_back({cbInfo.slot, false, cbInfo.name, cbInfo.size});
        }
    }

    std::sort(allCBs.begin(), allCBs.end(), [](const CBBinding& a, const CBBinding& b) {
        return a.slot < b.slot;
    });

    u32 cbCount = 0;
    for (const auto& cb : allCBs) {
        if (cb.isVCB) {
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::VolatileConstantBuffer(cb.slot));
        } else {
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(cb.slot));
        }
        cbCount++;
    }

    u32 texCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        xr_vector<MaterialPSO::TextureSlot> sortedTextures(matPSO->textures);
        std::sort(sortedTextures.begin(), sortedTextures.end(),
            [](const MaterialPSO::TextureSlot& a, const MaterialPSO::TextureSlot& b) {
                return a.slot < b.slot;
            });

        for (const auto& texSlot : sortedTextures) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
            texCount++;
        }
    }

    u32 samplerCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        xr_vector<MaterialPSO::SamplerInfo> stageSamplers;
        for (const auto& samplerInfo : matPSO->samplers) {
            if (samplerInfo.stage == stage) {
                stageSamplers.push_back(samplerInfo);
            }
        }

        std::sort(stageSamplers.begin(), stageSamplers.end(),
            [](const MaterialPSO::SamplerInfo& a, const MaterialPSO::SamplerInfo& b) {
                return a.slot < b.slot;
            });

        for (const auto& samplerInfo : stageSamplers) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
            samplerCount++;
        }
    }


    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        return nullptr;
    }

    return layout;
}

nvrhi::BindingLayoutHandle MaterialCache::CreateCombinedBindingLayout(const MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;

    struct CBBinding {
        u32 slot;
        bool isVCB;
    };
    xr_vector<CBBinding> allCBs;

    for (const auto& vcbReq : matPSO->vcbRequirements) {
        allCBs.push_back({vcbReq.slot, true});
    }

    for (const auto& cbInfo : matPSO->constantBuffers) {
        bool duplicate = false;
        for (const auto& existing : allCBs) {
            if (existing.slot == cbInfo.slot) { duplicate = true; break; }
        }
        if (!duplicate)
            allCBs.push_back({cbInfo.slot, false});
    }

    std::sort(allCBs.begin(), allCBs.end(), [](const CBBinding& a, const CBBinding& b) {
        return a.slot < b.slot;
    });

    for (const auto& cb : allCBs) {
        if (cb.isVCB)
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::VolatileConstantBuffer(cb.slot));
        else
            layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(cb.slot));
    }

    xr_vector<MaterialPSO::TextureSlot> sortedTextures(matPSO->textures);
    std::sort(sortedTextures.begin(), sortedTextures.end(),
        [](const MaterialPSO::TextureSlot& a, const MaterialPSO::TextureSlot& b) {
            return a.slot < b.slot;
        });
    for (const auto& texSlot : sortedTextures) {
        layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
    }

    xr_vector<MaterialPSO::SamplerInfo> allSamplers;
    xr_set<u32> addedSamplerSlots;
    for (const auto& samplerInfo : matPSO->samplers) {
        if (addedSamplerSlots.count(samplerInfo.slot))
            continue;
        allSamplers.push_back(samplerInfo);
        addedSamplerSlots.insert(samplerInfo.slot);
    }
    std::sort(allSamplers.begin(), allSamplers.end(),
        [](const MaterialPSO::SamplerInfo& a, const MaterialPSO::SamplerInfo& b) {
            return a.slot < b.slot;
        });
    for (const auto& samplerInfo : allSamplers) {
        layoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
    }


    return m_device->CreateBindingLayout(layoutDesc);
}


nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(MaterialPSO* matPSO)
{
    VERIFY(matPSO);
    VERIFY(matPSO->vsBindingLayout);

    bool combinedMode = (matPSO->psBindingLayout == nullptr);

    if (!combinedMode)
        VERIFY(matPSO->psBindingLayout);

    bool cacheValid = combinedMode
        ? (matPSO->vsBindingSet && !matPSO->needsBindingSetRebuild)
        : (matPSO->vsBindingSet && matPSO->psBindingSet && !matPSO->needsBindingSetRebuild);
    if (cacheValid)
        return matPSO->vsBindingSet;

    matPSO->needsBindingSetRebuild = false;


    struct TempBinding {
        u32 slot;
        nvrhi::IBuffer* buffer;
        shared_str name;
        bool isVCB;
    };
    xr_vector<TempBinding> vsBindings;

    for (const auto& vcbReq : matPSO->vcbRequirements) {
        fg::BufferHandle latestHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                vcbReq.name.c_str(), vcbReq.slot, vcbReq.size
            )
        );

        if (!latestHandle.IsValid()) {
            continue;
        }

        nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(latestHandle);
        if (!vcbBuffer) {
            continue;
        }

        vsBindings.push_back({vcbReq.slot, vcbBuffer, vcbReq.name, true});
    }

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Vertex) {
            if (cbInfo.nvrhiBuffer) {
                vsBindings.push_back({cbInfo.slot, cbInfo.nvrhiBuffer.Get(), cbInfo.name, false});
            }
        }
    }

    std::sort(vsBindings.begin(), vsBindings.end(), [](const TempBinding& a, const TempBinding& b) {
        return a.slot < b.slot;
    });

    nvrhi::BindingSetDesc vsBindingDesc;
    for (const auto& binding : vsBindings) {
        vsBindingDesc.bindings.push_back(
            nvrhi::BindingSetItem::ConstantBuffer(binding.slot, binding.buffer));
    }


    struct PSBinding {
        u32 slot;
        nvrhi::BindingSetItem item;
        shared_str name;
        enum Type { CB, Texture, Sampler } type;
    };
    xr_vector<PSBinding> psBindings;

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (cbInfo.nvrhiBuffer) {
                psBindings.push_back({cbInfo.slot,
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()),
                    cbInfo.name, PSBinding::CB});
            }
        }
    }

    bool allTexturesValid = true;
    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = texManager->GetNVRHITexture(texSlot.handle);

        if (nativeTex) {
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            psBindings.push_back({texSlot.slot,
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex,
                    nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, texDesc.dimension),
                "texture", PSBinding::Texture});
        } else {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Texture not loaded yet (slot t%u), cannot create binding set", texSlot.slot);
            return nullptr;
        }
    }

    for (const auto& samplerInfo : matPSO->samplers) {
        if (samplerInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (samplerInfo.nvrhiSampler) {
                psBindings.push_back({samplerInfo.slot,
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, samplerInfo.nvrhiSampler),
                    samplerInfo.name, PSBinding::Sampler});
            } else {
                psBindings.push_back({samplerInfo.slot,
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, nullptr),
                    "sampler_null", PSBinding::Sampler});
            }
        }
    }

    std::sort(psBindings.begin(), psBindings.end(), [](const PSBinding& a, const PSBinding& b) {
        if (a.type != b.type) return a.type < b.type;
        return a.slot < b.slot;
    });

    nvrhi::BindingSetDesc psBindingDesc;
    for (const auto& binding : psBindings) {
        psBindingDesc.bindings.push_back(binding.item);
    }

    if (combinedMode)
    {
        nvrhi::BindingSetDesc combinedDesc;

        xr_set<u32> addedCBSlots;
        for (const auto& binding : vsBindings) {
            combinedDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(binding.slot, binding.buffer));
            addedCBSlots.insert(binding.slot);
        }

        for (const auto& binding : psBindings) {
            if (binding.type == PSBinding::CB && addedCBSlots.count(binding.slot))
                continue;
            combinedDesc.bindings.push_back(binding.item);
        }

        nvrhi::BindingSetHandle combinedSet = m_device->CreateBindingSet(
            combinedDesc, matPSO->vsBindingLayout);

        if (!combinedSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create combined binding set");
            return nullptr;
        }

        matPSO->vsBindingSet = combinedSet;
        matPSO->psBindingSet = nullptr;
    }
    else
    {
        nvrhi::BindingSetHandle vsBindingSet = m_device->CreateBindingSet(
            vsBindingDesc, matPSO->vsBindingLayout);

        if (!vsBindingSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create VS binding set");
            return nullptr;
        }

        nvrhi::BindingSetHandle psBindingSet = m_device->CreateBindingSet(
            psBindingDesc, matPSO->psBindingLayout);

        if (!psBindingSet) {
            Msg("! [MaterialCache::GetOrCreateBindingSet] Failed to create PS binding set");
            return nullptr;
        }

        matPSO->vsBindingSet = vsBindingSet;
        matPSO->psBindingSet = psBindingSet;
    }

    if (!allTexturesValid) {
        matPSO->needsBindingSetRebuild = true;
    }

    return matPSO->vsBindingSet;
}

u32 MaterialCache::GetVertexFormatID(dxRender_Visual* visual)
{
    if (!visual)
        return 0;

    return 0;
}

#if defined(USE_DX11) && defined(XR_PLATFORM_WINDOWS)
static u32 GetFormatSize(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
        case DXGI_FORMAT_R32G32B32_FLOAT: return 12;
        case DXGI_FORMAT_R32G32_FLOAT: return 8;
        case DXGI_FORMAT_R32_FLOAT: return 4;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return 8;
        case DXGI_FORMAT_R16G16_FLOAT: return 4;
        case DXGI_FORMAT_R16_FLOAT: return 2;
        case DXGI_FORMAT_R8G8B8A8_UNORM: return 4;
        case DXGI_FORMAT_R8G8_UNORM: return 2;
        case DXGI_FORMAT_R8_UNORM: return 1;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return 8;
        case DXGI_FORMAT_R16G16_SNORM: return 4;
        case DXGI_FORMAT_R16_SNORM: return 2;
        case DXGI_FORMAT_R8G8B8A8_SNORM: return 4;
        case DXGI_FORMAT_R8G8_SNORM: return 2;
        case DXGI_FORMAT_R8_SNORM: return 1;
        case DXGI_FORMAT_R16G16B16A16_UINT: return 8;
        case DXGI_FORMAT_R16G16_UINT: return 4;
        case DXGI_FORMAT_R16_UINT: return 2;
        case DXGI_FORMAT_R8G8B8A8_UINT: return 4;
        case DXGI_FORMAT_R8G8_UINT: return 2;
        case DXGI_FORMAT_R8_UINT: return 1;
        case DXGI_FORMAT_R32G32B32A32_UINT: return 16;
        case DXGI_FORMAT_R32G32_UINT: return 8;
        case DXGI_FORMAT_R32_UINT: return 4;
        case DXGI_FORMAT_R16G16B16A16_SINT: return 8;
        case DXGI_FORMAT_R16G16_SINT: return 4;
        case DXGI_FORMAT_R16_SINT: return 2;
        case DXGI_FORMAT_R8G8B8A8_SINT: return 4;
        case DXGI_FORMAT_R8G8_SINT: return 2;
        case DXGI_FORMAT_R8_SINT: return 1;
        case DXGI_FORMAT_R32G32B32A32_SINT: return 16;
        case DXGI_FORMAT_R32G32_SINT: return 8;
        case DXGI_FORMAT_R32_SINT: return 4;
        default:
            return 4;
    }
}
#endif










MaterialPSO* MaterialCache::GetOrCreateUIPSO(
    IUIShader* uiShader,
    u32 elementIndex,
    nvrhi::IFramebuffer* framebuffer,
    fg::PrimitiveTopology topology)
{
    if (!uiShader || !framebuffer)
        return nullptr;

    fgUIShader* dxShader = static_cast<fgUIShader*>(uiShader);
    if (!dxShader || !dxShader->IsAlive())
        return nullptr;

    u64 shaderHash = 0;
    if (dxShader->m_vsHandle.Get() && dxShader->m_psHandle.Get()) {
        shaderHash = reinterpret_cast<uintptr_t>(dxShader->m_vsHandle.Get()) ^
                     (reinterpret_cast<uintptr_t>(dxShader->m_psHandle.Get()) << 1);
    }

    MaterialKey key;
    key.psoType = PSOType::UI;
    key.textureHash = shaderHash ^ (static_cast<u64>(topology) << 56);
    key.element = elementIndex;
    key.framebuffer = framebuffer;

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    m_stats.numCacheMisses++;
    m_stats.totalPSOCreations++;
    MaterialPSO* pso = CreateUIPSO(uiShader, framebuffer, topology);
    if (!pso)
        return nullptr;

    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}


MaterialPSO* MaterialCache::CreateUIPSO(
    IUIShader* uiShader,
    nvrhi::IFramebuffer* framebuffer,
    fg::PrimitiveTopology topology)
{
    auto pso = xr_make_unique<MaterialPSO>();

    fgUIShader* dxShader = static_cast<fgUIShader*>(uiShader);
    if (!dxShader || !dxShader->IsAlive()) {
        Msg("! [MaterialCache::CreateUIPSO] Stale or null uiShader pointer (%p)", dxShader);
        return nullptr;
    }

    if (!dxShader->m_vsHandle.Get() || !dxShader->m_psHandle.Get()) {
        Msg("! [MaterialCache::CreateUIPSO] Missing NVRHI shader handles in fgUIShader (VS=%p PS=%p)",
            dxShader->m_vsHandle.Get(), dxShader->m_psHandle.Get());
        return nullptr;
    }

    nvrhi::ShaderHandle nvrhiVS = dxShader->m_vsHandle;
    nvrhi::ShaderHandle nvrhiPS = dxShader->m_psHandle;

    if (dxShader->m_vsReflection) {
        pso->vsInputSignature = framegraph::ShaderReflector::GetVertexInputSignature(dxShader->m_vsReflection.get());
        pso->constantLayout = dxShader->m_vsReflection->constantLayout;

        for (const auto& cb : dxShader->m_vsReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Vertex;
            pso->constantBuffers.push_back(cbInfo);
        }
    }
    else {
        Msg("! [MaterialCache::CreateUIPSO] No VS reflection data available");
    }

    if (dxShader->m_psReflection) {
        for (const auto& cb : dxShader->m_psReflection->constantLayout.constantBuffers.buffers) {
            MaterialPSO::ConstantBufferInfo cbInfo;
            cbInfo.name = cb.name.c_str();
            cbInfo.slot = cb.slot;
            cbInfo.size = cb.size;
            cbInfo.stage = MaterialPSO::ShaderStage::Pixel;
            pso->constantBuffers.push_back(cbInfo);
        }

        for (const auto& samp : dxShader->m_psReflection->rtBindings.samplers) {
            MaterialPSO::SamplerInfo sampInfo;
            sampInfo.name = samp.name.c_str();
            sampInfo.slot = samp.slot;
            sampInfo.stage = MaterialPSO::ShaderStage::Pixel;

            nvrhi::SamplerDesc samplerDesc;
            if (strstr(samp.name.c_str(), "smp_base")) {
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.maxAnisotropy = 16;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Wrap;
            } else if (strstr(samp.name.c_str(), "smp_linear") || strstr(samp.name.c_str(), "smp_rtlinear")) {
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
            } else if (strstr(samp.name.c_str(), "smp_nofilter")) {
                samplerDesc.minFilter = false;
                samplerDesc.magFilter = false;
                samplerDesc.mipFilter = false;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
            } else {
                samplerDesc.minFilter = true;
                samplerDesc.magFilter = true;
                samplerDesc.mipFilter = true;
                samplerDesc.addressU = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressV = nvrhi::SamplerAddressMode::Wrap;
                samplerDesc.addressW = nvrhi::SamplerAddressMode::Wrap;
            }

            sampInfo.nvrhiSampler = m_device->GetNVRHIDevice()->createSampler(samplerDesc);
            if (!sampInfo.nvrhiSampler) {
                Msg("! [MaterialCache::CreateUIPSO] Failed to create sampler: %s", samp.name.c_str());
            }

            pso->samplers.push_back(sampInfo);
        }
    }

    xr_map<shared_str, nvrhi::BufferHandle> createdBuffers;
    for (auto& cbInfo : pso->constantBuffers) {
        auto it = createdBuffers.find(cbInfo.name);
        if (it != createdBuffers.end()) {
            cbInfo.nvrhiBuffer = it->second;
            continue;
        }

        nvrhi::BufferDesc bufferDesc;
        bufferDesc.byteSize = cbInfo.size;
        bufferDesc.isConstantBuffer = true;
        bufferDesc.debugName = make_string("UI_CB_%s", cbInfo.name.c_str()).c_str();
        bufferDesc.keepInitialState = true;
        bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = m_device->GetNVRHIDevice()->createBuffer(bufferDesc);
        if (!buffer) {
            Msg("! [MaterialCache::CreateUIPSO] Failed to create constant buffer: %s", cbInfo.name.c_str());
            continue;
        }

        cbInfo.nvrhiBuffer = buffer;
        createdBuffers[cbInfo.name] = buffer;
    }

    CreateBindingLayouts(pso.get());

    fg::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();
    psoDesc.pixelShader = nvrhiPS.Get();


    if (pso->vsInputSignature.elements.empty()) {
        Msg("! [MaterialCache::CreateUIPSO] No vertex input signature from shader reflection!");
        return nullptr;
    }

    struct VertexElement {
        nvrhi::Format format;
        u32 offset;
    };

    auto resolveUISemantic = [](const std::string& sem, u32 idx) -> std::optional<VertexElement> {
        if (sem == "POSITION" || sem == "POSITIONT") return VertexElement{nvrhi::Format::RGBA32_FLOAT, 0};
        if (sem == "COLOR")                          return VertexElement{nvrhi::Format::RGBA8_UNORM, 16};
        if (sem == "TEXCOORD") return VertexElement{nvrhi::Format::RG32_FLOAT, 20};
        if (sem == "TEXINDEX") return VertexElement{nvrhi::Format::R32_UINT,   28};
        return std::nullopt;
    };

    for (const auto& shaderElem : pso->vsInputSignature.elements) {
        std::string semantic = shaderElem.semanticName.c_str();
        auto resolved = resolveUISemantic(semantic, shaderElem.semanticIndex);
        if (!resolved) {
            Msg("! [MaterialCache::CreateUIPSO] Unknown UI vertex semantic: %s%u", semantic.c_str(), shaderElem.semanticIndex);
            continue;
        }

        fg::VertexAttribute attr;
        attr.semanticName = shaderElem.semanticName.c_str();
        attr.semanticIndex = shaderElem.semanticIndex;
        attr.format = resolved->format;
        attr.offset = resolved->offset;
        attr.bufferIndex = 0;
        attr.elementStride = sizeof(ui::UIVertex);

        psoDesc.vertexAttributes.push_back(attr);
    }
    pso->vertexStride = sizeof(ui::UIVertex);

    const nvrhi::FramebufferDesc& fbDesc = framebuffer->getDesc();
    psoDesc.renderTargetCount = static_cast<u32>(fbDesc.colorAttachments.size());
    for (u32 i = 0; i < fbDesc.colorAttachments.size() && i < 8; ++i) {
        if (fbDesc.colorAttachments[i].texture) {
            psoDesc.renderTargetFormats[i] = fbDesc.colorAttachments[i].texture->getDesc().format;
        }
    }

    bool hasDepth = (fbDesc.depthAttachment.texture != nullptr);
    if (hasDepth) {
        psoDesc.depthStencilFormat = fbDesc.depthAttachment.texture->getDesc().format;
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthFunc = fg::ComparisonFunc::Always;
        psoDesc.depthStencilState.depthWriteEnable = false;
        psoDesc.depthStencilState.stencilEnable = true;
    } else {
        psoDesc.depthStencilState.depthTestEnable = false;
        psoDesc.depthStencilState.depthWriteEnable = false;
        psoDesc.depthStencilState.stencilEnable = false;
    }

    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = fg::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = fg::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = fg::BlendFactor::One;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = fg::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = fg::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;

    psoDesc.primitiveTopology = topology;

    if (pso->vsBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->vsBindingLayout);
    }
    if (pso->psBindingLayout) {
        psoDesc.bindingLayouts.push_back(pso->psBindingLayout);
    }

    nvrhi::IBindingLayout* bindlessLayout = GEnv.Backend ? GEnv.Backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout) {
        psoDesc.bindingLayouts.push_back(nvrhi::BindingLayoutHandle(bindlessLayout));
    }

    if (psoDesc.bindingLayouts.empty()) {
        Msg("! [MaterialCache::CreateUIPSO] No binding layouts created from shader reflection!");
        return nullptr;
    }

    psoDesc.debugName = "UI_PSO";

    fg::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateUIPSO] No PSO cache");
        return nullptr;
    }

    fg::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to create pipeline state");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    nvrhi::IGraphicsPipeline* nativePipeline = nvrhiPSO->GetNativePipeline();
    if (nativePipeline) {
        const nvrhi::GraphicsPipelineDesc& actualDesc = nativePipeline->getDesc();
        const bool hadPS = (pso->psBindingLayout != nullptr);
        if (actualDesc.bindingLayouts.size() >= 1)
            pso->vsBindingLayout = actualDesc.bindingLayouts[0];
        if (hadPS && actualDesc.bindingLayouts.size() >= 2)
            pso->psBindingLayout = actualDesc.bindingLayouts[1];
    }

    return pso.release();
}



bool MaterialCache::IsFramebufferDependentKey(const MaterialKey& key)
{
    return key.psoType == PSOType::UI || key.psoType == PSOType::Depth;
}

void MaterialCache::ReleasePSOTextures(MaterialPSO* pso)
{
    if (!pso)
        return;

    resources::TextureManager* texMgr = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (!texMgr)
        return;

    for (auto& slot : pso->textures)
        if (slot.handle.IsValid())
            texMgr->Release(slot.handle);

    pso->textures.clear();
}

void MaterialCache::InvalidateFramebufferDependent()
{
    for (auto it = m_cache.begin(); it != m_cache.end();)
    {
        if (!IsFramebufferDependentKey(it->first))
        {
            ++it;
            continue;
        }

        ReleasePSOTextures(it->second.get());
        it = m_cache.erase(it);
    }

    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());
}

void MaterialCache::Clear()
{
    InvalidateFramebufferDependent();

    for (auto& [key, pso] : m_cache)
        ReleasePSOTextures(pso.get());

    m_cache.clear();
    m_detailScaleCache.clear();
    m_shaderHandles.clear();
    ++m_visualMaterialEpoch;
    m_materialIDByNames.clear();
    m_pendingMaterials.clear();
    m_stats = Stats{};
}





float MaterialCache::GetDetailScale(const shared_str& textureName)
{
    auto cacheIt = m_detailScaleCache.find(textureName.c_str());
    if (cacheIt != m_detailScaleCache.end()) {
        return cacheIt->second;
    }

    float scale = TextureDescr.GetDetailScale(textureName);

    m_detailScaleCache[textureName.c_str()] = scale;

    return scale;
}


u32 MaterialCache::RegisterBindlessMaterial(MaterialPSO* matPSO)
{
    using namespace fg::bindless;

    if (!matPSO)
        return UINT32_MAX;

    if (matPSO->bindlessMaterialID != UINT32_MAX)
        return matPSO->bindlessMaterialID;

    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    if (!texManager)
        return UINT32_MAX;

    MaterialData matData = {};
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = matPSO->detail_scale;
    matData.alphaRef = 0.5f;
    matData.flags = 0;
    matData.shaderVariant = 0;

    u32 materialID = materialBuffer.RegisterMaterial(matData);
    matPSO->bindlessMaterialID = materialID;

    return materialID;
}


bool MaterialCache::IsTerrainMaterial(dxRender_Visual* visual)
{
    if (!visual || !visual->shaderName.size())
        return false;
    return shader_info::IsTerrainShader(visual->shaderName.c_str());
}


u32 MaterialCache::PreRegisterTerrainMaterial(dxRender_Visual* visual)
{
    using namespace fg::bindless;

    if (!visual)
        return UINT32_MAX;

    shared_str shaderName = visual->shaderName;

    auto it = m_shaderToTerrainMaterialID.find(shaderName);
    if (it != m_shaderToTerrainMaterialID.end())
        return it->second;

    auto& terrainBuffer = TerrainMaterialBuffer::Instance();
    if (!terrainBuffer.IsInitialized()) {
        static bool s_warnOnce = false;
        if (!s_warnOnce) {
            Msg("! [MaterialCache] TerrainMaterialBuffer not initialized - terrain will render BLACK!");
            Msg("!   This usually means level loaded before FrameGraphRenderer::Initialize()");
            s_warnOnce = true;
        }
        return UINT32_MAX;
    }

    TerrainMaterialData matData = {};

    matData.baseAlbedoIndex = INVALID_TEXTURE_INDEX;
    matData.blendMaskIndex = INVALID_TEXTURE_INDEX;
    matData.detailR_Index = INVALID_TEXTURE_INDEX;
    matData.detailG_Index = INVALID_TEXTURE_INDEX;
    matData.detailB_Index = INVALID_TEXTURE_INDEX;
    matData.detailA_Index = INVALID_TEXTURE_INDEX;
    matData.normalR_Index = INVALID_TEXTURE_INDEX;
    matData.normalG_Index = INVALID_TEXTURE_INDEX;
    matData.normalB_Index = INVALID_TEXTURE_INDEX;
    matData.normalA_Index = INVALID_TEXTURE_INDEX;
    matData.pbrR_Index = INVALID_TEXTURE_INDEX;
    matData.pbrG_Index = INVALID_TEXTURE_INDEX;
    matData.pbrB_Index = INVALID_TEXTURE_INDEX;
    matData.pbrA_Index = INVALID_TEXTURE_INDEX;

    matData.flags = MAT_FLAG_TERRAIN;

    if (visual->textureName.size() > 0) {
        matData.detailScale = GetDetailScale(visual->textureName);
    } else {
        matData.detailScale = 4.0f;
    }

    u32 terrainMaterialID = terrainBuffer.RegisterMaterial(matData);

    m_shaderToTerrainMaterialID[shaderName] = terrainMaterialID;

    PendingTerrainMaterial pending;
    pending.terrainMaterialID = terrainMaterialID;
    pending.visual = visual;
    m_pendingTerrainMaterials.push_back(pending);

    static u32 logCount = 0;
    if (++logCount <= 5) {
        Msg("* [MaterialCache] PreRegisterTerrain: matID=%u visual=%p tex='%s' shader='%s'",
            terrainMaterialID, visual, visual->textureName.c_str(), visual->shaderName.c_str());
    }

    return terrainMaterialID;
}


void MaterialCache::FinalizePendingTerrainMaterials()
{
    using namespace fg::bindless;

    if (m_pendingTerrainMaterials.empty())
        return;

    auto& terrainBuffer = TerrainMaterialBuffer::Instance();
    if (!terrainBuffer.IsInitialized())
        return;

    if (!GEnv.Backend)
        return;

    u32 processedCount = 0;

    for (const auto& pending : m_pendingTerrainMaterials) {
        dxRender_Visual* visual = pending.visual;
        u32 terrainMaterialID = pending.terrainMaterialID;

        if (!visual || terrainMaterialID == UINT32_MAX)
            continue;

        const TerrainMaterialData* existingMat = terrainBuffer.GetMaterial(terrainMaterialID);
        if (!existingMat)
            continue;

        TerrainMaterialData matData = *existingMat;
        bool updated = false;
        xr_vector<xr_string> missingTextures;

        shader_info::TerrainDetailNames detailNames;
        bool hasTerrainDetail = shader_info::GetTerrainDetailNames(visual->shaderName.c_str(), detailNames);

        auto RegisterTexture = [&](const char* texName, const char* slotName, fg::TextureColorSpace colorSpace) -> u32 {
            if (!texName || !texName[0]) {
                missingTextures.push_back(xr_string(slotName) + ": (empty name)");
                return INVALID_TEXTURE_INDEX;
            }

            const u32 idx = AcquireMaterialTexture(texName, colorSpace);
            if (idx == INVALID_TEXTURE_INDEX)
                missingTextures.push_back(xr_string(slotName) + ": " + texName);
            return idx;
        };

        if (visual->textureName.size()) {
            u32 idx = RegisterTexture(visual->textureName.c_str(), "base", fg::TextureColorSpace::Srgb);
            if (idx != INVALID_TEXTURE_INDEX) {
                matData.baseAlbedoIndex = idx;
                updated = true;
            }
        }

        if (visual->textureName.size()) {
            xr_string maskName(visual->textureName.c_str());
            maskName += "_mask";
            u32 idx = RegisterTexture(maskName.c_str(), "mask", fg::TextureColorSpace::Linear);
            if (idx != INVALID_TEXTURE_INDEX) {
                matData.blendMaskIndex = idx;
                updated = true;
            }
        }

        const char* detailR = hasTerrainDetail ? detailNames.r : nullptr;
        const char* detailG = hasTerrainDetail ? detailNames.g : nullptr;
        const char* detailB = hasTerrainDetail ? detailNames.b : nullptr;
        const char* detailA = hasTerrainDetail ? detailNames.a : nullptr;

        {
            u32 idx = RegisterTexture(detailR, "detailR", fg::TextureColorSpace::Srgb);
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailR_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailG, "detailG", fg::TextureColorSpace::Srgb);
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailG_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailB, "detailB", fg::TextureColorSpace::Srgb);
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailB_Index = idx; updated = true; }
        }
        {
            u32 idx = RegisterTexture(detailA, "detailA", fg::TextureColorSpace::Srgb);
            if (idx != INVALID_TEXTURE_INDEX) { matData.detailA_Index = idx; updated = true; }
        }

        auto& texDescMgr = TextureDescr;
        if (detailR && detailR[0]) {
            shared_str bumpR = texDescMgr.GetBumpName(detailR);
            if (bumpR.size()) {
                u32 idx = RegisterTexture(bumpR.c_str(), "normalR", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalR_Index = idx; updated = true; }
            }
        }
        if (detailG && detailG[0]) {
            shared_str bumpG = texDescMgr.GetBumpName(detailG);
            if (bumpG.size()) {
                u32 idx = RegisterTexture(bumpG.c_str(), "normalG", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalG_Index = idx; updated = true; }
            }
        }
        if (detailB && detailB[0]) {
            shared_str bumpB = texDescMgr.GetBumpName(detailB);
            if (bumpB.size()) {
                u32 idx = RegisterTexture(bumpB.c_str(), "normalB", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalB_Index = idx; updated = true; }
            }
        }
        if (detailA && detailA[0]) {
            shared_str bumpA = texDescMgr.GetBumpName(detailA);
            if (bumpA.size()) {
                u32 idx = RegisterTexture(bumpA.c_str(), "normalA", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.normalA_Index = idx; updated = true; }
            }
        }

        if (detailR && detailR[0]) {
            shared_str pbrR = texDescMgr.GetPBRName(detailR);
            if (pbrR.size()) {
                u32 idx = RegisterTexture(pbrR.c_str(), "pbrR", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) {
                    matData.pbrR_Index = idx;
                    matData.flags |= MAT_FLAG_HAS_PBR_LAYER;
                    updated = true;
                }
            }
        }
        if (detailG && detailG[0]) {
            shared_str pbrG = texDescMgr.GetPBRName(detailG);
            if (pbrG.size()) {
                u32 idx = RegisterTexture(pbrG.c_str(), "pbrG", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrG_Index = idx; updated = true; }
            }
        }
        if (detailB && detailB[0]) {
            shared_str pbrB = texDescMgr.GetPBRName(detailB);
            if (pbrB.size()) {
                u32 idx = RegisterTexture(pbrB.c_str(), "pbrB", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrB_Index = idx; updated = true; }
            }
        }
        if (detailA && detailA[0]) {
            shared_str pbrA = texDescMgr.GetPBRName(detailA);
            if (pbrA.size()) {
                u32 idx = RegisterTexture(pbrA.c_str(), "pbrA", fg::TextureColorSpace::Linear);
                if (idx != INVALID_TEXTURE_INDEX) { matData.pbrA_Index = idx; updated = true; }
            }
        }

        if (updated) {
            terrainBuffer.UpdateMaterial(terrainMaterialID, matData);
            processedCount++;

            static u32 s_debugLogCount = 0;
            bool hasZeroIndex = (matData.baseAlbedoIndex == 0 || matData.blendMaskIndex == 0 ||
                                 matData.detailR_Index == 0 || matData.detailG_Index == 0 ||
                                 matData.normalR_Index == 0 || matData.pbrR_Index == 0);
            Msg("* [TerrainMat] id=%u base=%u mask=%u dR=%u dG=%u dB=%u dA=%u nR=%u nG=%u pR=%u%s",
                terrainMaterialID, matData.baseAlbedoIndex, matData.blendMaskIndex,
                matData.detailR_Index, matData.detailG_Index, matData.detailB_Index, matData.detailA_Index,
                matData.normalR_Index, matData.normalG_Index, matData.pbrR_Index,
                hasZeroIndex ? " [IDX 0!]" : "");
            s_debugLogCount++;
        }

        if (!missingTextures.empty()) {
            Msg("! [TerrainMaterial] matID=%u shader='%s' tex='%s' - missing %zu textures:",
                terrainMaterialID, visual->shaderName.c_str(), visual->textureName.c_str(),
                missingTextures.size());
            for (const auto& missing : missingTextures) {
                Msg("!   - %s", missing.c_str());
            }
        }
    }

    m_pendingTerrainMaterials.clear();

    static u32 s_finalizeCallCount = 0;
    s_finalizeCallCount++;

    if (processedCount > 0) {
        Msg("* [MaterialCache] Finalized %u terrain materials (call #%u, total registered: %u)",
            processedCount, s_finalizeCallCount, terrainBuffer.GetMaterialCount());
    }
}


u32 MaterialCache::PreRegisterBindlessMaterial(dxRender_Visual* visual)
{
    using namespace fg::bindless;

    if (!visual)
        return UINT32_MAX;

    if (visual->bindless_material_epoch == m_visualMaterialEpoch)
        return visual->bindless_material_id;

    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    const auto nameKey = std::make_pair(visual->shaderName, visual->textureName);
    const auto known = m_materialIDByNames.find(nameKey);
    if (known != m_materialIDByNames.end()) {
        visual->bindless_material_id = known->second;
        visual->bindless_material_epoch = m_visualMaterialEpoch;
        return known->second;
    }

    MaterialData matData = {};
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = 1.0f;
    matData.alphaRef = 0.5f;
    matData.flags = 0;
    matData.shaderVariant = 0;

    if (visual->textureName.size() > 0) {
        matData.detailScale = GetDetailScale(visual->textureName);
    }

    if (visual->shaderName.size() > 0)
    {
        const auto& matInfo = MaterialSystem::Instance().GetMaterialInfo(visual->shaderName.c_str(), visual->textureName.c_str());
        matData.alphaRef = matInfo.GetAlphaReference();
        matData.flags = matInfo.GetMaterialFlags() | MAT_FLAG_HAS_NORMAL;
        matData.shaderVariant = matInfo.shaderVariant;
        if (matInfo.alphaTest)
        {
            auto it = m_alphaRefByTexture.find(visual->textureName);
            if (it == m_alphaRefByTexture.end())
                m_alphaRefByTexture[visual->textureName] = matInfo.alphaRef;
            else
                it->second = std::max(it->second, matInfo.alphaRef);
        }
    }

    u32 materialID = materialBuffer.RegisterMaterial(matData);

    visual->bindless_material_id = materialID;
    visual->bindless_material_epoch = m_visualMaterialEpoch;
    m_materialIDByNames.emplace(nameKey, materialID);

    fg::TextureColorSpace diffuseColorSpace = fg::TextureColorSpace::Srgb;
    const auto* variant = ShaderVariantRegistry::Instance().GetVariantByIndex(matData.shaderVariant);
    if (variant && variant->distort && variant->textures.empty())
        diffuseColorSpace = fg::TextureColorSpace::Linear;

    PendingMaterial pending;
    pending.materialID = materialID;
    pending.textureName = visual->textureName;
    pending.diffuseColorSpace = diffuseColorSpace;
    m_pendingMaterials.push_back(pending);

    static u32 logCount = 0;
    if (++logCount <= 10) {
        Msg("* [MaterialCache] PreRegister: matID=%u visual=%p type=%u tex='%s' shader='%s' pending=%u",
            materialID, visual, visual->getType(), visual->textureName.c_str(), visual->shaderName.c_str(),
            static_cast<u32>(m_pendingMaterials.size()));
    }

    return materialID;
}

u32 MaterialCache::PreRegisterParticleMaterial(const shared_str& textureName, bool distortion)
{
    using namespace fg::bindless;

    if (!textureName.size() || !textureName[0])
        return UINT32_MAX;

    const fg::TextureColorSpace diffuseColorSpace = distortion ? fg::TextureColorSpace::Linear : fg::TextureColorSpace::Srgb;
    const auto key = std::make_pair(textureName, diffuseColorSpace);
    auto it = m_particleTextureToMaterialID.find(key);
    if (it != m_particleTextureToMaterialID.end())
        return it->second;

    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return UINT32_MAX;

    MaterialData matData = {};
    matData.diffuseIndex = INVALID_TEXTURE_INDEX;
    matData.normalIndex = INVALID_TEXTURE_INDEX;
    matData.detailIndex = INVALID_TEXTURE_INDEX;
    matData.pbrIndex = INVALID_TEXTURE_INDEX;
    matData.detailScale = 1.0f;
    matData.alphaRef = 0.01f / 255.0f;
    matData.flags = 0;
    matData.shaderVariant = 0;

    u32 materialID = materialBuffer.RegisterMaterial(matData);

    m_particleTextureToMaterialID[key] = materialID;

    PendingMaterial pending;
    pending.materialID = materialID;
    pending.textureName = textureName;
    pending.diffuseColorSpace = diffuseColorSpace;
    m_pendingMaterials.push_back(pending);

    Msg("* [MaterialCache] PreRegisterParticle: matID=%u tex='%s' pending=%u",
        materialID, textureName.c_str(), static_cast<u32>(m_pendingMaterials.size()));

    return materialID;
}


void MaterialCache::FinalizePendingMaterials()
{
    using namespace fg::bindless;

    if (m_pendingMaterials.empty()) {
        return;
    }

    auto& materialBuffer = MaterialBuffer::Instance();
    if (!materialBuffer.IsInitialized())
        return;

    if (!GEnv.Backend) {
        Msg("! [MaterialCache] Backend not available - cannot register bindless textures");
        return;
    }


    for (const auto& pending : m_pendingMaterials) {
        u32 materialID = pending.materialID;

        if (materialID == UINT32_MAX)
            continue;

        const MaterialData* existingMat = materialBuffer.GetMaterial(materialID);
        if (!existingMat)
            continue;

        MaterialData matData = *existingMat;
        bool updated = false;

        const shared_str& diffuseName = pending.textureName;
        if (!diffuseName.size() || !diffuseName[0])
            continue;

        {
            const u32 descriptorIndex = pending.diffuseColorSpace == fg::TextureColorSpace::Srgb
                ? AcquireAlbedoTexture(diffuseName.c_str())
                : AcquireMaterialTexture(diffuseName.c_str(), pending.diffuseColorSpace);
            if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                matData.diffuseIndex = descriptorIndex;
                updated = true;
            }
        }

        auto& texDescMgr = TextureDescr;
        shared_str bumpName = texDescMgr.GetBumpName(diffuseName);
        if (bumpName.size() && bumpName[0]) {
            const u32 descriptorIndex = AcquireMaterialTexture(bumpName.c_str(), fg::TextureColorSpace::Linear);
            if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                matData.normalIndex = descriptorIndex;
                matData.flags |= MAT_FLAG_HAS_NORMAL;
                updated = true;
            }
        }

        LPCSTR detailTexName = nullptr;
        if (texDescMgr.GetDetailTexture(diffuseName, detailTexName) && detailTexName && detailTexName[0]) {
            const u32 descriptorIndex = AcquireMaterialTexture(detailTexName, fg::TextureColorSpace::Srgb);
            if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                matData.detailIndex = descriptorIndex;
                matData.detailScale = texDescMgr.GetDetailScale(diffuseName);
                matData.flags |= MAT_FLAG_HAS_DETAIL;
                updated = true;
            }
        }

        shared_str pbrName = texDescMgr.GetPBRName(diffuseName);
        if (!pbrName.empty()) {
            const u32 descriptorIndex = AcquireMaterialTexture(pbrName.c_str(), fg::TextureColorSpace::Linear);
            if (descriptorIndex != INVALID_TEXTURE_INDEX) {
                matData.pbrIndex = descriptorIndex;
                matData.flags |= MAT_FLAG_HAS_PBR;
                updated = true;
            }
        }

        if (updated) {
            materialBuffer.UpdateMaterial(materialID, matData);
        }

        if (matData.shaderVariant > 0) {
            auto& registry = ShaderVariantRegistry::Instance();
            const auto* variant = registry.GetVariantByIndex(matData.shaderVariant);
            if (variant && !variant->textures.empty()) {
                auto& vtb = bindless::VariantTextureBuffer::Instance();
                bindless::VariantTextureData vtData;
                for (u32 i = 0; i < bindless::MAX_VARIANT_TEXTURE_SLOTS; i++)
                    vtData.tex[i] = INVALID_TEXTURE_INDEX;

                u32 slotIdx = 0;
                for (const auto& [slotName, texPath] : variant->textures) {
                    if (slotIdx >= bindless::MAX_VARIANT_TEXTURE_SLOTS) break;
                    if (texPath.c_str()[0] == '$') { slotIdx++; continue; }

                    const u32 idx = AcquireMaterialTexture(texPath.c_str(), fg::TextureColorSpace::Linear);
                    if (idx != INVALID_TEXTURE_INDEX)
                        vtData.tex[slotIdx] = idx;
                    slotIdx++;
                }
                vtb.SetVariantTextures(materialID, vtData);
            }
        }
    }

    m_pendingMaterials.clear();
}

nvrhi::ITexture* MaterialCache::GetNVRHITextureByName(const char* textureName)
{
    if (!textureName || !textureName[0])
        return nullptr;

    auto* texManager = m_resourceManager ? m_resourceManager->GetTextureManager() : nullptr;
    if (!texManager)
        return nullptr;

    auto cacheIt = m_textureHandleCache.find(textureName);
    if (cacheIt == m_textureHandleCache.end())
        cacheIt = m_textureHandleCache.emplace(textureName, texManager->LoadTexture(textureName, fg::TextureColorSpace::Srgb)).first;
    return texManager->GetNVRHITexture(cacheIt->second);
}

}
