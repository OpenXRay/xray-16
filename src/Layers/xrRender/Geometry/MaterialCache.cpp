#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/Shader.h"
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
#include "Layers/xrRender/FrameGraph/IPass.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#include "Layers/xrRenderDX11/StateManager/dx11SamplerStateCache.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"
#include "../Externals/nvrhi/src/common/dxgi-format.h"
#include "../Externals/nvrhi/src/d3d11/d3d11-backend.h"
#endif

namespace xray::render {

using namespace xray::render::RENDER_NAMESPACE;


nvrhi::Format ConvertDxgiFormatToNvrhi(DXGI_FORMAT dxgiFormat) {
    for (uint32_t i = 0; i < uint32_t(nvrhi::Format::COUNT); i++) {
        nvrhi::Format nvrhiFormat = static_cast<nvrhi::Format>(i);
        const nvrhi::DxgiFormatMapping& mapping = nvrhi::getDxgiFormatMapping(nvrhiFormat);

        if (mapping.resourceFormat == dxgiFormat ||
            mapping.srvFormat == dxgiFormat ||
            mapping.rtvFormat == dxgiFormat) {
            return nvrhiFormat;
        }
    }

    return nvrhi::Format::UNKNOWN;
}

MaterialCache::MaterialCache(
    ng::RenderDevice* device,
    resources::FGResourceManager* resourceManager,
    framegraph::VolatileConstantBufferPool* vcbPool)
    : m_device(device)
    , m_resourceManager(resourceManager)
    , m_vcbPool(vcbPool)
{
    VERIFY(m_device);
    VERIFY(m_resourceManager);
}

MaterialCache::~MaterialCache() {
    Clear();
}

MaterialPSO* MaterialCache::GetOrCreatePSO(
    dxRender_Visual* visual,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg)
{
    if (!visual) {
        Msg("! [MaterialCache::GetOrCreatePSO] Visual is NULL");
        return nullptr;
    }

    Shader* shader = nullptr;

    if (visual->shader && visual->shader._get()) {
        shader = visual->shader._get();
    }
    else if (visual->shaderName.c_str() && visual->shaderName.size() > 0 &&
             visual->textureName.c_str() && visual->textureName.size() > 0) {

        Msg("* [MaterialCache::GetOrCreatePSO] Compiling shader on-demand: '%s' with texture '%s'",
            visual->shaderName.c_str(), visual->textureName.c_str());

        visual->shader.create(visual->shaderName.c_str(), visual->textureName.c_str());
        shader = visual->shader._get();

        if (!shader) {
            Msg("! [MaterialCache::GetOrCreatePSO] Failed to compile shader '%s' for visual type %u",
                visual->shaderName.c_str(), visual->getType());
            return nullptr;
        }
    } else {
        Msg("! [MaterialCache::GetOrCreatePSO] Visual has no shader or shader name (type %u)", visual->getType());
        return nullptr;
    }

    ShaderElement* elem = shader->E[0]._get();
    if (!elem) {
        Msg("! [MaterialCache::GetOrCreatePSO] Shader has no element E[0] (visual type %u)", visual->getType());
        return nullptr;
    }

    if (elem->passes.empty()) {
        Msg("! [MaterialCache::GetOrCreatePSO] Shader element E[0] has no passes (visual type %u)", visual->getType());
        return nullptr;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        Msg("! [MaterialCache::GetOrCreatePSO] Shader element E[0] pass[0] is NULL (visual type %u)", visual->getType());
        return nullptr;
    }

    u64 textureHash = ComputeTextureHash(pass);
    u64 stateHash = ComputeStateHash(pass);

    MaterialKey key(shader, textureHash, stateHash);

    const char* shaderName = "unknown";
    if (pass->vs._get() && pass->vs._get()->cName.c_str()) {
        shaderName = pass->vs._get()->cName.c_str();
    }

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    m_stats.numCacheMisses++;

    Msg("* [MaterialCache::GetOrCreatePSO] Creating PSO for shader '%s' (cache miss)", shaderName);

    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg);
    if (!pso) {
        Msg("! [MaterialCache::GetOrCreatePSO] CreatePSO failed for shader '%s'", shaderName);
        return nullptr;
    }

    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

MaterialPSO* MaterialCache::CreatePSO(
    dxRender_Visual* visual,
    ShaderElement* elem,
    SPass* pass,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg)
{
    auto pso = xr_make_unique<MaterialPSO>();

    pso->pass = pass;

    ExtractTextures(pass, pso.get());

    if (pass && pass->T && !pass->T->empty()) {
        const shared_str& baseTexName = (*pass->T)[0].second;
        if (baseTexName.c_str() && baseTexName[0]) {
            pso->detail_scale = GetDetailScale(baseTexName);
        }
    }

    if (!ExtractShaders(pass, pso.get())) {
        return nullptr;
    }

    ExtractSamplers(pass, pso.get());

    CreateBindingLayouts(pso.get());
    if (!pso->vsBindingLayout || !pso->psBindingLayout) {
        return nullptr;
    }

    nvrhi::ShaderHandle nvrhiVS = GetOrCreateShaderVS(pso->vertexShader);
    if (!nvrhiVS) {
        return nullptr;
    }

    nvrhi::ShaderHandle nvrhiPS = GetOrCreateShaderPS(pso->pixelShader);
    if (!nvrhiPS) {
        return nullptr;
    }

    if (m_vcbPool) {
        const auto& vsCBs = framegraph::ShaderReflector::GetConstantBuffers(
            pso->vertexShader->reflection);
        for (const auto& cbInfo : vsCBs.buffers) {
            framegraph::VolatileConstantBufferPool::CBLayout layout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            );

            ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(layout);

            MaterialPSO::VCBRequirement req;
            req.slot = cbInfo.slot;
            req.size = cbInfo.size;
            req.name = cbInfo.name;
            req.vcbHandle = vcbHandle;
            pso->vcbRequirements.push_back(req);
        }

        const auto& psCBs = framegraph::ShaderReflector::GetConstantBuffers(
            pso->pixelShader->reflection);
        for (const auto& cbInfo : psCBs.buffers) {
            framegraph::VolatileConstantBufferPool::CBLayout layout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            );

            ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(layout);

            MaterialPSO::VCBRequirement req;
            req.slot = cbInfo.slot;
            req.size = cbInfo.size;
            req.name = cbInfo.name;
            req.vcbHandle = vcbHandle;
            pso->vcbRequirements.push_back(req);
        }
    }

    if (pso->vertexShader && pso->vertexShader->reflection) {
        const auto& vsLayout = pso->vertexShader->reflection->constantLayout;

        Msg("  [MaterialCache] VS constantLayout has %u constants", vsLayout.constants.size());

        pso->constantLayout.constantBuffers = vsLayout.constantBuffers;
        pso->constantLayout.constants = vsLayout.constants;
    }

    if (pso->pixelShader && pso->pixelShader->reflection) {
        const auto& psLayout = pso->pixelShader->reflection->constantLayout;

        Msg("  [MaterialCache] PS constantLayout has %u constants", psLayout.constants.size());

        for (const auto& psConstant : psLayout.constants) {
            bool isDuplicate = false;
            for (const auto& vsConstant : pso->constantLayout.constants) {
                if (vsConstant.name == psConstant.name) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                pso->constantLayout.constants.push_back(psConstant);
            }
        }
    }

    Msg("  [MaterialCache] Final PSO constantLayout has %u constants total", pso->constantLayout.constants.size());

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = nvrhiVS.Get();
    psoDesc.pixelShader = nvrhiPS.Get();

    if (!ValidateVertexLayoutCompatibility(visual, pso.get())) {
        Msg("! [MaterialCache::CreatePSO] Vertex layout mismatch for shader '%s' - geometry doesn't provide required attributes",
            pso->debugName.c_str());
        return nullptr;
    }

    SetupVertexAttributes(visual, pso.get(), psoDesc);

    SetupRenderStates(pass, psoDesc);

    SetupRenderTargets(pso.get(), outputs, fg, psoDesc);

    psoDesc.debugName = pso->debugName;

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = nullptr;

    try {
        nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    }
    catch (const std::exception& e) {
        Msg("! [MaterialCache::CreatePSO] PSO creation failed for shader '%s': %s",
            pso->debugName.c_str(), e.what());
        return nullptr;
    }
    catch (...) {
        Msg("! [MaterialCache::CreatePSO] PSO creation failed for shader '%s': Unknown exception",
            pso->debugName.c_str());
        return nullptr;
    }

    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreatePSO] PSO creation returned NULL for shader '%s'",
            pso->debugName.c_str());
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    m_stats.totalPSOCreations++;

    return pso.release();
}

void MaterialCache::ExtractTextures(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

    matPSO->textures.clear();

    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    VERIFY(texManager);

    STextureList* texList = pass->T._get();

    if (!texList || texList->empty()) {
        Msg("  [ExtractTextures] No textures bound for this shader");
        return;
    }

    Msg("  [ExtractTextures] Loading %u textures from pass", texList->size());

    for (size_t i = 0; i < texList->size(); i++) {
        const auto& texPair = (*texList)[i];
        u32 stage = texPair.first;
        const shared_str& textureName = texPair.second;

        if (!textureName || !textureName[0]) {
            continue;
        }

        if (xr_strlen(textureName.c_str()) > 6 && 0 == strncmp(textureName.c_str(), "$user$", 6))
        {
            Msg("~ [ExtractTextures] Skipping render target texture: %s (slot=%u) - RT resolution not yet implemented",
                textureName.c_str(), stage);
            continue;
        }

        auto cacheIt = m_textureHandleCache.find(textureName.c_str());
        if (cacheIt != m_textureHandleCache.end()) {
            MaterialPSO::TextureSlot texSlot;
            texSlot.slot = stage;
            texSlot.handle = cacheIt->second;
            matPSO->textures.push_back(texSlot);
            continue;
        }

        resources::TextureHandle resourceHandle = texManager->LoadTexture(
            textureName.c_str(),
            resources::TexturePriority::High
        );

        if (!resourceHandle.IsValid()) {
            Msg("! [MaterialCache] Failed to load texture: %s", textureName.c_str());
            continue;
        }

        m_textureHandleCache[textureName.c_str()] = resourceHandle;

        MaterialPSO::TextureSlot texSlot;
        texSlot.slot = stage;
        texSlot.handle = resourceHandle;
        matPSO->textures.push_back(texSlot);

        Msg("  [ExtractTextures] Loaded texture '%s' at slot=%u", textureName, stage);
    }

    Msg("  [ExtractTextures] Total textures loaded: %u", matPSO->textures.size());
}

bool MaterialCache::ExtractShaders(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

    SVS* vs = pass->vs._get();
    if (!vs) {
        return false;
    }
    matPSO->vertexShader = vs;

    SPS* ps = pass->ps._get();
    if (!ps) {
        return false;
    }
    matPSO->pixelShader = ps;

    matPSO->debugName = vs->cName;

    Msg("  [MaterialCache] VS '%s' reflection pointer: %p", vs->cName.c_str(), vs->reflection);
    if (vs->reflection) {
        Msg("  [MaterialCache] VS reflection has %u input elements", vs->reflection->vertexInputSignature.elements.size());
        matPSO->vsInputSignature = framegraph::ShaderReflector::GetVertexInputSignature(
            vs->reflection
        );
    } else {
        Msg("! [MaterialCache] VS '%s' has NULL reflection!", vs->cName.c_str());
    }

    if (ps->reflection) {
        matPSO->rtBindings = framegraph::ShaderReflector::GetRTBindings(
            ps->reflection
        );

        matPSO->rtBindings.shaderName = ps->cName;
    }

    matPSO->constantBuffers.clear();

    auto extractCBsFromReflection = [&](const framegraph::ExtractedReflection* reflection,
                                       const char* shaderName,
                                       MaterialPSO::ShaderStage stage) {
        if (!reflection) {
            Msg("! [MaterialCache] No reflection data for %s shader", shaderName);
            return;
        }

        Msg("  [MaterialCache] Extracting %u CBs from %s shader reflection",
            reflection->constantBuffers.buffers.size(), shaderName);

        for (const auto& cbReflection : reflection->constantBuffers.buffers) {
            bool found = false;
            for (const auto& existing : matPSO->constantBuffers) {
                if (existing.slot == cbReflection.slot && existing.stage == stage) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                bool isPerObjectCB = (cbReflection.name == "$Globals");

                nvrhi::BufferHandle bufferHandle;
                nvrhi::BufferDesc nvrhiDesc;
                nvrhiDesc.byteSize = cbReflection.size;
                nvrhiDesc.isConstantBuffer = true;
                nvrhiDesc.debugName = isPerObjectCB ? "XRay_PerObject_CB" : "FrameGraph_Global_CB";
                nvrhiDesc.keepInitialState = false;
                nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

                bufferHandle = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);
                if (bufferHandle) {
                    Msg("  [MaterialCache] Created CB from reflection: name=%s, slot=%d, size=%d bytes, stage=%s",
                        cbReflection.name.c_str(),
                        cbReflection.slot,
                        cbReflection.size,
                        stage == MaterialPSO::ShaderStage::Vertex ? "VS" : "PS");

                    MaterialPSO::ConstantBufferInfo cbInfo;
                    cbInfo.slot = cbReflection.slot;
                    cbInfo.stage = stage;
                    cbInfo.nvrhiBuffer = bufferHandle;
                    cbInfo.size = cbReflection.size;
                    cbInfo.isPerObject = isPerObjectCB;
                    cbInfo.name = cbReflection.name;

                    matPSO->constantBuffers.push_back(cbInfo);
                } else {
                    Msg("! [MaterialCache] FAILED to create CB: name=%s, slot=%d",
                        cbReflection.name.c_str(), cbReflection.slot);
                }
            }
        }
    };

    extractCBsFromReflection(vs->reflection, vs->cName.c_str(), MaterialPSO::ShaderStage::Vertex);

    extractCBsFromReflection(ps->reflection, ps->cName.c_str(), MaterialPSO::ShaderStage::Pixel);

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            matPSO->perObjectCBSize = cbInfo.size;
            break;
        }
    }

    if (matPSO->perObjectCBSize == 0) {
        matPSO->perObjectCBSize = 256;
    }

    return true;
}

void MaterialCache::ExtractSamplers(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

#if defined(USE_DX11)
    SState* xrState = pass->state._get();
    if (!xrState || !xrState->state) {
        return;
    }

    dx11State* d3dState = static_cast<dx11State*>(xrState->state);
    if (!d3dState) {
        return;
    }

    const auto& psSamplers = d3dState->GetPSSamplers();


    auto createNVRHISampler = [&](ID3DSamplerState* d3dSampler) -> nvrhi::SamplerHandle {
        D3D11_SAMPLER_DESC samplerDesc;
        d3dSampler->GetDesc(&samplerDesc);

        nvrhi::SamplerDesc nvrhiDesc;
        nvrhiDesc.setMinFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);
        nvrhiDesc.setMagFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);
        nvrhiDesc.setMipFilter(samplerDesc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT);

        auto convertAddressMode = [](D3D11_TEXTURE_ADDRESS_MODE mode) {
            switch (mode) {
                case D3D11_TEXTURE_ADDRESS_WRAP: return nvrhi::SamplerAddressMode::Wrap;
                case D3D11_TEXTURE_ADDRESS_CLAMP: return nvrhi::SamplerAddressMode::Clamp;
                case D3D11_TEXTURE_ADDRESS_MIRROR: return nvrhi::SamplerAddressMode::Mirror;
                case D3D11_TEXTURE_ADDRESS_BORDER: return nvrhi::SamplerAddressMode::Border;
                default: return nvrhi::SamplerAddressMode::Wrap;
            }
        };

        nvrhiDesc.setAddressU(convertAddressMode(samplerDesc.AddressU));
        nvrhiDesc.setAddressV(convertAddressMode(samplerDesc.AddressV));
        nvrhiDesc.setAddressW(convertAddressMode(samplerDesc.AddressW));
        nvrhiDesc.setMaxAnisotropy(static_cast<float>(samplerDesc.MaxAnisotropy));
        nvrhiDesc.setMipBias(samplerDesc.MipLODBias);

        return m_device->GetNativeDevice()->createSampler(nvrhiDesc);
    };

    auto createDefaultSampler = [&]() -> nvrhi::SamplerHandle {
        nvrhi::SamplerDesc nvrhiDesc;
        nvrhiDesc.setMinFilter(true);
        nvrhiDesc.setMagFilter(true);
        nvrhiDesc.setMipFilter(true);
        nvrhiDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        nvrhiDesc.setMaxAnisotropy(8.0f);

        return m_device->GetNativeDevice()->createSampler(nvrhiDesc);
    };

    for (const auto& samplerDecl : matPSO->rtBindings.samplers) {
        u32 slot = samplerDecl.slot;
        const char* samplerName = samplerDecl.name.c_str();

        MaterialPSO::SamplerInfo samplerInfo;
        samplerInfo.slot = slot;
        samplerInfo.stage = MaterialPSO::ShaderStage::Pixel;
        samplerInfo.name = samplerName;

        if (slot < psSamplers.size()) {
            auto samplerHandle = psSamplers[slot];
            if (samplerHandle != dx11SamplerStateCache::hInvalidHandle) {
                ID3DSamplerState* d3dSampler = SSManager.GetSamplerState(samplerHandle);
                if (d3dSampler) {
                    samplerInfo.nvrhiSampler = createNVRHISampler(d3dSampler);
                }
            }
        }

        if (!samplerInfo.nvrhiSampler) {
            samplerInfo.nvrhiSampler = createDefaultSampler();
        }

        if (samplerInfo.nvrhiSampler) {
            matPSO->samplers.push_back(samplerInfo);
        } else {
        }
    }

#endif
}

void MaterialCache::CreateBindingLayouts(MaterialPSO* matPSO)
{
    VERIFY(matPSO);

    matPSO->vsBindingLayout = CreateStageBindingLayout(
        matPSO, MaterialPSO::ShaderStage::Vertex, nvrhi::ShaderType::Vertex);

    matPSO->psBindingLayout = CreateStageBindingLayout(
        matPSO, MaterialPSO::ShaderStage::Pixel, nvrhi::ShaderType::Pixel);

    if (matPSO->vsBindingLayout && matPSO->psBindingLayout) {
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

    u32 cbCount = 0;
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == stage) {
            if (cbInfo.isPerObject) {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
            } else {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
            }
            cbCount++;
        }
    }

    u32 texCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        for (const auto& texSlot : matPSO->textures) {
            layoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(texSlot.slot));
            texCount++;
        }
    }

    u32 samplerCount = 0;
    if (stage == MaterialPSO::ShaderStage::Pixel) {
        for (const auto& samplerInfo : matPSO->samplers) {
            if (samplerInfo.stage == stage) {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::Sampler(samplerInfo.slot));
                samplerCount++;
            }
        }
    }


    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        return nullptr;
    }

    return layout;
}

nvrhi::BindingSetHandle MaterialCache::GetOrCreateBindingSet(
    MaterialPSO* matPSO,
    nvrhi::IBuffer* perObjectVCB,
    SPass* pass)
{
    VERIFY(matPSO);
    VERIFY(perObjectVCB);
    VERIFY(matPSO->vsBindingLayout);
    VERIFY(matPSO->psBindingLayout);

    if (matPSO->vsBindingSet && matPSO->psBindingSet) {
        return matPSO->vsBindingSet;
    }

    nvrhi::BindingSetDesc vsBindingDesc;

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Vertex) {
            if (cbInfo.isPerObject) {
                vsBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, perObjectVCB));
                Msg("  [GetOrCreateBindingSet] VS: Added per-object CB at slot %d", cbInfo.slot);
            } else {
                if (cbInfo.nvrhiBuffer) {
                    vsBindingDesc.bindings.push_back(
                        nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
                    Msg("  [GetOrCreateBindingSet] VS: Added global CB '%s' at slot %d (size=%d)",
                        cbInfo.name.empty() ? "EMPTY" : cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
                } else {
                    Msg("! [GetOrCreateBindingSet] VS: CB '%s' at slot %d has NULL buffer!",
                        cbInfo.name.empty() ? "EMPTY" : cbInfo.name.c_str(), cbInfo.slot);
                }
            }
        }
    }

    nvrhi::BindingSetDesc psBindingDesc;

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (cbInfo.isPerObject) {
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, perObjectVCB));
            } else {
                if (cbInfo.nvrhiBuffer) {
                    psBindingDesc.bindings.push_back(
                        nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
                }
            }
        }
    }

    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
    Msg("  [GetOrCreateBindingSet] Adding %u textures to PS binding set", matPSO->textures.size());
    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = texManager->GetNVRHITexture(texSlot.handle);

        if (nativeTex) {
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            Msg("    [GetOrCreateBindingSet] Adding texture to slot t%u (valid texture, %ux%u)",
                texSlot.slot, texDesc.width, texDesc.height);
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex,
                    nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, texDesc.dimension));
        } else {
            Msg("!   [GetOrCreateBindingSet] Texture at slot t%u is NULL!", texSlot.slot);
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nullptr));
        }
    }

    for (const auto& samplerInfo : matPSO->samplers) {
        if (samplerInfo.stage == MaterialPSO::ShaderStage::Pixel) {
            if (samplerInfo.nvrhiSampler) {
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, samplerInfo.nvrhiSampler));
            } else {
                psBindingDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(samplerInfo.slot, nullptr));
            }
        }
    }

    matPSO->vsBindingSet = m_device->CreateBindingSet(
        vsBindingDesc,
        matPSO->vsBindingLayout);

    if (!matPSO->vsBindingSet) {
        return nullptr;
    }

    matPSO->psBindingSet = m_device->CreateBindingSet(
        psBindingDesc,
        matPSO->psBindingLayout);

    if (!matPSO->psBindingSet) {
        return nullptr;
    }


    return matPSO->vsBindingSet;
}

u64 MaterialCache::ComputeTextureHash(SPass* pass)
{
    if (!pass)
        return 0;

    STextureList* texList = pass->T._get();
    if (!texList || texList->empty())
        return 0;

    u32 hash = 0;
    for (const auto& texPair : *texList) {
        const shared_str& textureName = texPair.second;
        if (textureName.c_str() && textureName[0]) {
            hash = crc32(textureName.c_str(), xr_strlen(textureName.c_str()), hash);
        }
    }

    return static_cast<u64>(hash);
}

u64 MaterialCache::ComputeStateHash(SPass* pass)
{
    if (!pass)
        return 0;

    void* statePtr = pass->state._get();
    u32 hash = crc32(&statePtr, sizeof(statePtr), 0);

    return static_cast<u64>(hash);
}

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

bool MaterialCache::ValidateVertexLayoutCompatibility(dxRender_Visual* visual, MaterialPSO* matPSO)
{
    if (!visual || !matPSO) return false;

    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        default:
            return true;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return true;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return true;

    SDeclaration* decl = geom->dcl._get();

    if (!matPSO->vsInputSignature.elements.empty()) {
        Msg("  [MaterialCache::Validate] Geometry provides %u vertex attributes:", decl->dx11_dcl_code.size());
        for (size_t i = 0; i < decl->dx11_dcl_code.size(); ++i) {
            const auto& d3dElem = decl->dx11_dcl_code[i];
            if (d3dElem.SemanticName) {
                Msg("    [%u] %s%d", i, d3dElem.SemanticName, d3dElem.SemanticIndex);
            } else {
                Msg("    [%u] (null semantic)", i);
            }
        }

        Msg("  [MaterialCache::Validate] Shader expects %u vertex attributes:", matPSO->vsInputSignature.elements.size());
        for (const auto& elem : matPSO->vsInputSignature.elements) {
            Msg("    - %s%d (format %d)", elem.semanticName.c_str(), elem.semanticIndex, (int)elem.format);
        }

        for (const auto& shaderElem : matPSO->vsInputSignature.elements) {
            bool found = false;
            for (const auto& d3dElem : decl->dx11_dcl_code) {
                if (!d3dElem.SemanticName)
                    continue;

                if (xr_strcmp(d3dElem.SemanticName, shaderElem.semanticName.c_str()) == 0 &&
                    d3dElem.SemanticIndex == shaderElem.semanticIndex) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                Msg("! [MaterialCache] Vertex layout validation FAILED: shader expects '%s%d' but geometry doesn't provide it",
                    shaderElem.semanticName.c_str(), shaderElem.semanticIndex);
                return false;
            }
        }
    }

    return true;
}

void MaterialCache::SetupVertexAttributes(dxRender_Visual* visual, MaterialPSO* matPSO, ng::PipelineStateDesc& psoDesc)
{
    psoDesc.vertexAttributes.clear();

    IRender_Mesh* meshVisual = nullptr;
    switch (visual->getType()) {
        case MT_NORMAL:
            meshVisual = static_cast<Fvisual*>(visual);
            break;
        case MT_PROGRESSIVE:
            meshVisual = static_cast<FProgressive*>(visual);
            break;
        case MT_TREE_ST:
        case MT_TREE_PM:
            meshVisual = static_cast<FTreeVisual*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_ST:
            meshVisual = static_cast<CSkeletonX_ST*>(visual);
            break;
        case MT_SKELETON_GEOMDEF_PM:
            meshVisual = static_cast<CSkeletonX_PM*>(visual);
            break;
        default:
            return;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return;

    SDeclaration* decl = geom->dcl._get();

    if (!matPSO || matPSO->vsInputSignature.elements.empty()) {
    } else {
    }

    std::map<u32, u32> bufferStrides;


    for (const auto& d3dElem : decl->dx11_dcl_code) {
        if (!d3dElem.SemanticName)
            continue;

        u32 slot = d3dElem.InputSlot;
        u32 elemSize = GetFormatSize(d3dElem.Format);

        u32 endOffset = d3dElem.AlignedByteOffset + elemSize;

        const char* formatName = "UNKNOWN";
        if (d3dElem.Format == DXGI_FORMAT_R8G8B8A8_UNORM) formatName = "R8G8B8A8_UNORM";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32_FLOAT) formatName = "R32G32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32B32_FLOAT) formatName = "R32G32B32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R32G32B32A32_FLOAT) formatName = "R32G32B32A32_FLOAT";
        else if (d3dElem.Format == DXGI_FORMAT_R16G16_SINT) formatName = "R16G16_SINT";

            d3dElem.SemanticName, d3dElem.SemanticIndex,

        bufferStrides[slot] = std::max(bufferStrides[slot], endOffset);
    }

    if (matPSO && !matPSO->vsInputSignature.elements.empty()) {

        for (u32 shaderIdx = 0; shaderIdx < matPSO->vsInputSignature.elements.size(); ++shaderIdx) {
            const auto& shaderElem = matPSO->vsInputSignature.elements[shaderIdx];


            const D3D11_INPUT_ELEMENT_DESC* matchingDeclElem = nullptr;
            for (const auto& d3dElem : decl->dx11_dcl_code) {
                if (!d3dElem.SemanticName)
                    continue;

                if (xr_strcmp(d3dElem.SemanticName, shaderElem.semanticName.c_str()) == 0 &&
                    d3dElem.SemanticIndex == shaderElem.semanticIndex) {
                    matchingDeclElem = &d3dElem;
                    break;
                }
            }

            if (!matchingDeclElem) {
                continue;
            }

            ng::VertexAttribute attr;
            attr.semanticName = matchingDeclElem->SemanticName;
            attr.semanticIndex = matchingDeclElem->SemanticIndex;
            attr.format = ConvertDxgiFormatToNvrhi(matchingDeclElem->Format);
            attr.offset = matchingDeclElem->AlignedByteOffset;
            attr.bufferIndex = matchingDeclElem->InputSlot;
            attr.isInstanced = (matchingDeclElem->InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA);
            attr.elementStride = bufferStrides[matchingDeclElem->InputSlot];

            psoDesc.vertexAttributes.push_back(attr);
        }
    }
}

void MaterialCache::SetupRenderStates(SPass* pass, ng::PipelineStateDesc& psoDesc)
{
    VERIFY(pass);

    SState* xrState = pass->state._get();
    if (!xrState || !xrState->state) {
        psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
        psoDesc.rasterizerState.fillMode = ng::FillMode::Solid;
        psoDesc.depthStencilState.depthTestEnable = true;
        psoDesc.depthStencilState.depthWriteEnable = true;
        psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
        psoDesc.blendState.renderTargets[0].blendEnable = false;
        return;
    }


#if defined(USE_DX11)
    dx11State* d3dState = static_cast<dx11State*>(xrState->state);

    ID3DRasterizerState* rasterizerState = d3dState->GetRasterizerState();
    if (rasterizerState) {
        D3D11_RASTERIZER_DESC rsDesc;
        rasterizerState->GetDesc(&rsDesc);

        psoDesc.rasterizerState.cullMode = ConvertCullMode(rsDesc.CullMode);
        psoDesc.rasterizerState.fillMode = ConvertFillMode(rsDesc.FillMode);
        psoDesc.rasterizerState.frontCounterClockwise = rsDesc.FrontCounterClockwise;
        psoDesc.rasterizerState.depthClipEnable = rsDesc.DepthClipEnable;
        psoDesc.rasterizerState.depthBias = rsDesc.DepthBias;
        psoDesc.rasterizerState.slopeScaledDepthBias = rsDesc.SlopeScaledDepthBias;
        psoDesc.rasterizerState.scissorEnable = rsDesc.ScissorEnable;
    }

    ID3DDepthStencilState* depthStencilState = d3dState->GetDepthStencilState();
    if (depthStencilState) {
        D3D11_DEPTH_STENCIL_DESC dsDesc;
        depthStencilState->GetDesc(&dsDesc);

        psoDesc.depthStencilState.depthTestEnable = dsDesc.DepthEnable;
        psoDesc.depthStencilState.depthWriteEnable = (dsDesc.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL);
        psoDesc.depthStencilState.depthFunc = ConvertComparisonFunc(dsDesc.DepthFunc);
        psoDesc.depthStencilState.stencilEnable = dsDesc.StencilEnable;

        if (dsDesc.StencilEnable) {
            psoDesc.depthStencilState.stencilReadMask = dsDesc.StencilReadMask;
            psoDesc.depthStencilState.stencilWriteMask = dsDesc.StencilWriteMask;

            psoDesc.depthStencilState.frontFace.failOp = ConvertStencilOp(dsDesc.FrontFace.StencilFailOp);
            psoDesc.depthStencilState.frontFace.depthFailOp = ConvertStencilOp(dsDesc.FrontFace.StencilDepthFailOp);
            psoDesc.depthStencilState.frontFace.passOp = ConvertStencilOp(dsDesc.FrontFace.StencilPassOp);
            psoDesc.depthStencilState.frontFace.compareFunc = ConvertComparisonFunc(dsDesc.FrontFace.StencilFunc);

            psoDesc.depthStencilState.backFace.failOp = ConvertStencilOp(dsDesc.BackFace.StencilFailOp);
            psoDesc.depthStencilState.backFace.depthFailOp = ConvertStencilOp(dsDesc.BackFace.StencilDepthFailOp);
            psoDesc.depthStencilState.backFace.passOp = ConvertStencilOp(dsDesc.BackFace.StencilPassOp);
            psoDesc.depthStencilState.backFace.compareFunc = ConvertComparisonFunc(dsDesc.BackFace.StencilFunc);
        }
    }

    ID3DBlendState* blendState = d3dState->GetBlendState();
    if (blendState) {
        D3D11_BLEND_DESC blendDesc;
        blendState->GetDesc(&blendDesc);


        psoDesc.blendState.alphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;

        if (blendDesc.IndependentBlendEnable) {
            for (int i = 0; i < 3; ++i) {
                const D3D11_RENDER_TARGET_BLEND_DESC& rtBlend = blendDesc.RenderTarget[i];

                psoDesc.blendState.renderTargets[i].blendEnable = rtBlend.BlendEnable;
                psoDesc.blendState.renderTargets[i].srcBlend = ConvertBlendFactor(rtBlend.SrcBlend);
                psoDesc.blendState.renderTargets[i].dstBlend = ConvertBlendFactor(rtBlend.DestBlend);
                psoDesc.blendState.renderTargets[i].blendOp = ConvertBlendOp(rtBlend.BlendOp);
                psoDesc.blendState.renderTargets[i].srcBlendAlpha = ConvertBlendFactor(rtBlend.SrcBlendAlpha);
                psoDesc.blendState.renderTargets[i].dstBlendAlpha = ConvertBlendFactor(rtBlend.DestBlendAlpha);
                psoDesc.blendState.renderTargets[i].blendOpAlpha = ConvertBlendOp(rtBlend.BlendOpAlpha);
                psoDesc.blendState.renderTargets[i].writeMask = ConvertColorWriteMask(rtBlend.RenderTargetWriteMask);
            }
        } else {
            const D3D11_RENDER_TARGET_BLEND_DESC& rtBlend = blendDesc.RenderTarget[0];

            for (int i = 0; i < 3; ++i) {
                psoDesc.blendState.renderTargets[i].blendEnable = rtBlend.BlendEnable;
                psoDesc.blendState.renderTargets[i].srcBlend = ConvertBlendFactor(rtBlend.SrcBlend);
                psoDesc.blendState.renderTargets[i].dstBlend = ConvertBlendFactor(rtBlend.DestBlend);
                psoDesc.blendState.renderTargets[i].blendOp = ConvertBlendOp(rtBlend.BlendOp);
                psoDesc.blendState.renderTargets[i].srcBlendAlpha = ConvertBlendFactor(rtBlend.SrcBlendAlpha);
                psoDesc.blendState.renderTargets[i].dstBlendAlpha = ConvertBlendFactor(rtBlend.DestBlendAlpha);
                psoDesc.blendState.renderTargets[i].blendOpAlpha = ConvertBlendOp(rtBlend.BlendOpAlpha);
                psoDesc.blendState.renderTargets[i].writeMask = ConvertColorWriteMask(rtBlend.RenderTargetWriteMask);
            }
        }
    } else {
    }
#endif
}

void MaterialCache::SetupRenderTargets(
    MaterialPSO* matPSO,
    const framegraph::DefaultOutputLayout& outputs,
    const framegraph::FrameGraph& fg,
    ng::PipelineStateDesc& psoDesc)
{
    nvrhi::ITexture* albedoTex = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normalTex = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* materialTex = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depthTex = fg.GetPhysicalTexture(outputs.depth);

    if (!albedoTex || !normalTex || !materialTex || !depthTex) {
        psoDesc.renderTargetCount = 3;
        psoDesc.renderTargetFormats[0] = nvrhi::Format::RGBA8_SNORM;
        psoDesc.renderTargetFormats[1] = nvrhi::Format::RGBA8_UNORM;
        psoDesc.renderTargetFormats[2] = nvrhi::Format::R16_UINT;
        psoDesc.depthStencilFormat = nvrhi::Format::D24S8;
        return;
    }

    psoDesc.renderTargetCount = static_cast<u32>(matPSO->rtBindings.outputRTs.size());

    for (u32 i = 0; i < 8; i++) {
        psoDesc.renderTargetFormats[i] = nvrhi::Format::UNKNOWN;
    }

    using RTSemantic = framegraph::ShaderRTBindings::RTSemantic;

    for (const auto& outputRT : matPSO->rtBindings.outputRTs) {
        u32 slot = outputRT.slot;
        nvrhi::Format format = nvrhi::Format::UNKNOWN;
        const char* rtName = "Unknown";

        switch (outputRT.semantic) {
            case RTSemantic::Normal:
                format = normalTex->getDesc().format;
                rtName = "Normal";
                break;

            case RTSemantic::Albedo:
                format = albedoTex->getDesc().format;
                rtName = "Albedo";
                break;

            case RTSemantic::Material:
                format = materialTex->getDesc().format;
                rtName = "Material";
                break;

            case RTSemantic::Position:
                continue;

            case RTSemantic::Emissive:
                continue;

            case RTSemantic::Accumulator:
                continue;

            default:
                continue;
        }

        psoDesc.renderTargetFormats[slot] = format;
    }

    psoDesc.depthStencilFormat = depthTex->getDesc().format;
}

MaterialPSO* MaterialCache::GetOrCreateUIPSO(
    Shader* shader,
    u32 elementIndex,
    nvrhi::IFramebuffer* framebuffer)
{
    if (!shader || !framebuffer)
        return nullptr;

    ShaderElement* elem = shader->E[elementIndex]._get();
    if (!elem || elem->passes.empty())
        return nullptr;

    SPass* pass = elem->passes[0]._get();
    if (!pass)
        return nullptr;

    MaterialKey key;
    key.isUIPSO = true;
    key.shader = shader;
    key.element = elementIndex;
    key.framebuffer = framebuffer;

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_stats.numCacheHits++;
        return it->second.get();
    }

    m_stats.numCacheMisses++;
    m_stats.totalPSOCreations++;

    MaterialPSO* pso = CreateUIPSO(shader, elem, pass, framebuffer);
    if (!pso)
        return nullptr;

    m_cache[key] = xr_unique_ptr<MaterialPSO>(pso);
    m_stats.numCachedPSOs = static_cast<u32>(m_cache.size());

    return pso;
}

MaterialPSO* MaterialCache::CreateUIPSO(
    Shader* shader,
    ShaderElement* elem,
    SPass* pass,
    nvrhi::IFramebuffer* framebuffer)
{
    auto pso = xr_make_unique<MaterialPSO>();
    pso->pass = pass;

    ExtractTextures(pass, pso.get());

    if (!ExtractShaders(pass, pso.get())) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to extract shaders");
        return nullptr;
    }

    ExtractSamplers(pass, pso.get());

    Msg("  [MaterialCache::CreateUIPSO] Extracted %zu textures, %zu samplers",
        pso->textures.size(), pso->samplers.size());

    if (pso->vertexShader && pso->vertexShader->reflection) {
        const auto& vsLayout = pso->vertexShader->reflection->constantLayout;
        Msg("  [MaterialCache::CreateUIPSO] VS constantLayout has %u constants", vsLayout.constants.size());

        pso->constantLayout.constantBuffers = vsLayout.constantBuffers;
        pso->constantLayout.constants = vsLayout.constants;
    }

    if (pso->pixelShader && pso->pixelShader->reflection) {
        const auto& psLayout = pso->pixelShader->reflection->constantLayout;
        Msg("  [MaterialCache::CreateUIPSO] PS constantLayout has %u constants", psLayout.constants.size());

        for (const auto& psConstant : psLayout.constants) {
            bool isDuplicate = false;
            for (const auto& vsConstant : pso->constantLayout.constants) {
                if (vsConstant.name == psConstant.name) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                pso->constantLayout.constants.push_back(psConstant);
            }
        }
    }

    Msg("  [MaterialCache::CreateUIPSO] Final PSO constantLayout has %u constants total", pso->constantLayout.constants.size());

    nvrhi::ShaderHandle nvrhiVS = GetOrCreateShaderVS(pso->vertexShader);
    nvrhi::ShaderHandle nvrhiPS = GetOrCreateShaderPS(pso->pixelShader);

    if (!nvrhiVS || !nvrhiPS) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to get shader handles");
        return nullptr;
    }

    CreateBindingLayouts(pso.get());

    ng::PipelineStateDesc psoDesc;
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

    std::map<std::string, VertexElement> uiVertexLayout;
    uiVertexLayout["POSITIONT"] = {nvrhi::Format::RGBA32_FLOAT, 0};
    uiVertexLayout["COLOR"]      = {nvrhi::Format::RGBA8_UNORM, 16};
    uiVertexLayout["TEXCOORD"]   = {nvrhi::Format::RG32_FLOAT, 20};

    for (const auto& shaderElem : pso->vsInputSignature.elements) {
        std::string semantic = shaderElem.semanticName.c_str();

        auto it = uiVertexLayout.find(semantic);
        if (it == uiVertexLayout.end()) {
            Msg("! [MaterialCache::CreateUIPSO] Unknown UI vertex semantic: %s", semantic.c_str());
            continue;
        }

        ng::VertexAttribute attr;
        attr.semanticName = shaderElem.semanticName.c_str();
        attr.semanticIndex = shaderElem.semanticIndex;
        attr.format = it->second.format;
        attr.offset = it->second.offset;
        attr.bufferIndex = 0;
        attr.elementStride = 0;

        psoDesc.vertexAttributes.push_back(attr);
    }

    u32 calculatedStride = 0;
    for (const auto& attr : psoDesc.vertexAttributes) {
        if (attr.bufferIndex == 0) {
            const nvrhi::FormatInfo& formatInfo = nvrhi::getFormatInfo(attr.format);
            u32 formatSize = formatInfo.bytesPerBlock;
            u32 endOffset = attr.offset + formatSize;
            calculatedStride = std::max(calculatedStride, endOffset);
        }
    }

    for (auto& attr : psoDesc.vertexAttributes) {
        attr.elementStride = calculatedStride;
    }

    pso->vertexStride = calculatedStride;
    Msg("  [MaterialCache::CreateUIPSO] Calculated vertex stride: %u bytes", pso->vertexStride);

    for (size_t i = 0; i < psoDesc.vertexAttributes.size(); ++i) {
        const auto& attr = psoDesc.vertexAttributes[i];
        Msg("  [MaterialCache::CreateUIPSO] Vertex attribute[%zu]: %s%d -> format=%d, offset=%u, stride=%u",
            i, attr.semanticName, attr.semanticIndex, (int)attr.format, attr.offset, attr.elementStride);
    }

    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Always;
    psoDesc.depthStencilState.depthWriteEnable = false;
    psoDesc.depthStencilState.stencilEnable = true;

    psoDesc.blendState.renderTargets[0].blendEnable = true;
    psoDesc.blendState.renderTargets[0].srcBlend = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlend = ng::BlendFactor::InvSrcAlpha;
    psoDesc.blendState.renderTargets[0].srcBlendAlpha = ng::BlendFactor::SrcAlpha;
    psoDesc.blendState.renderTargets[0].dstBlendAlpha = ng::BlendFactor::InvSrcAlpha;

    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    psoDesc.rasterizerState.frontCounterClockwise = false;
    psoDesc.rasterizerState.scissorEnable = true;

    const nvrhi::FramebufferDesc& fbDesc = framebuffer->getDesc();
    for (u32 i = 0; i < fbDesc.colorAttachments.size() && i < 8; ++i) {
        if (fbDesc.colorAttachments[i].texture) {
            psoDesc.renderTargetFormats[i] = fbDesc.colorAttachments[i].texture->getDesc().format;
        }
    }
    if (fbDesc.depthAttachment.texture) {
        psoDesc.depthStencilFormat = fbDesc.depthAttachment.texture->getDesc().format;
    }

    psoDesc.debugName = "UI_PSO";

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache::CreateUIPSO] No PSO cache");
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache::CreateUIPSO] Failed to create pipeline state");
        return nullptr;
    }

    pso->pso = nvrhiPSO;

    return pso.release();
}

void MaterialCache::Clear()
{
    m_cache.clear();
    m_textureHandleCache.clear();
    m_detailScaleCache.clear();
    m_shaderHandles.clear();
    m_stats = Stats{};
}

nvrhi::ShaderHandle MaterialCache::GetOrCreateShaderVS(SVS* vs)
{
    if (!vs) {
        return nullptr;
    }

    xr_string vsKeyStr = xr_string("VS_") + vs->cName.c_str();
    shared_str vsKey = vsKeyStr.c_str();

    auto it = m_shaderHandles.find(vsKey);
    if (it != m_shaderHandles.end()) {
        return it->second;
    }

    nvrhi::ShaderHandle vsHandle;

    if (vs->nvrhiShader)
    {
        vsHandle = vs->nvrhiShader;
    }
    else if (vs->bytecode)
    {
        nvrhi::ShaderDesc desc(nvrhi::ShaderType::Vertex);
        desc.debugName = vs->cName.c_str();
        desc.entryName = "main";

        vsHandle = m_device->GetNVRHIDevice()->createShader(
            desc,
            vs->bytecode->GetBufferPointer(),
            vs->bytecode->GetBufferSize());
    }
    else
    {
        Msg("! [MaterialCache] ERROR: VS '%s' has no nvrhiShader and no bytecode", vs->cName.c_str());
        return nullptr;
    }

    if (!vsHandle) {
        Msg("! [MaterialCache] ERROR: Failed to get/create VS '%s'", vs->cName.c_str());
        return nullptr;
    }

    m_shaderHandles[vsKey] = vsHandle;
    return vsHandle;
}

nvrhi::ShaderHandle MaterialCache::GetOrCreateShaderPS(SPS* ps)
{
    if (!ps) {
        return nullptr;
    }

    xr_string psKeyStr = xr_string("PS_") + ps->cName.c_str();
    shared_str psKey = psKeyStr.c_str();

    auto it = m_shaderHandles.find(psKey);
    if (it != m_shaderHandles.end()) {
        return it->second;
    }

    nvrhi::ShaderHandle psHandle;

    if (ps->nvrhiShader)
    {
        psHandle = ps->nvrhiShader;
    }
    else if (ps->bytecode)
    {
        nvrhi::ShaderDesc desc(nvrhi::ShaderType::Pixel);
        desc.debugName = ps->cName.c_str();
        desc.entryName = "main";

        psHandle = m_device->GetNVRHIDevice()->createShader(
            desc,
            ps->bytecode->GetBufferPointer(),
            ps->bytecode->GetBufferSize());
    }
    else
    {
        Msg("! [MaterialCache] ERROR: PS '%s' has no nvrhiShader and no bytecode", ps->cName.c_str());
        return nullptr;
    }

    if (!psHandle) {
        Msg("! [MaterialCache] ERROR: Failed to get/create PS '%s'", ps->cName.c_str());
        return nullptr;
    }

    m_shaderHandles[psKey] = psHandle;
    return psHandle;
}

float MaterialCache::GetDetailScale(const shared_str& textureName)
{
    auto cacheIt = m_detailScaleCache.find(textureName.c_str());
    if (cacheIt != m_detailScaleCache.end()) {
        return cacheIt->second;
    }

    float scale = RImplementation.Resources->m_textures_description.GetDetailScale(textureName);

    m_detailScaleCache[textureName.c_str()] = scale;

    return scale;
}

}
