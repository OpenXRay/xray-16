#include "stdafx.h"
#include "MaterialCache.h"
#include "Layers/xrRender/FrameGraphPasses/GBufferPass.h"
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

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#include "Layers/xrRenderDX11/StateManager/dx11SamplerStateCache.h"
#include "Layers/xrRenderDX11/dx11ConstantBuffer.h"
#include "../Externals/nvrhi/src/common/dxgi-format.h"
#include "../Externals/nvrhi/src/d3d11/d3d11-backend.h"
#endif

namespace xray::render {

using namespace passes;
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

    Msg("! [MaterialCache] Failed to convert DXGI_FORMAT %u to nvrhi::Format", dxgiFormat);
    return nvrhi::Format::UNKNOWN;
}

MaterialCache::MaterialCache(ng::RenderDevice* device, framegraph::VolatileConstantBufferPool* vcbPool)
    : m_device(device)
    , m_vcbPool(vcbPool)
{
    VERIFY(m_device);
}

MaterialCache::~MaterialCache() {
    Clear();
}

MaterialPSO* MaterialCache::GetOrCreatePSO(
    dxRender_Visual* visual,
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg)
{
    if (!visual) {
        Msg("! [MaterialCache] NULL visual passed to GetOrCreatePSO");
        return nullptr;
    }

    if (!visual->shader || !visual->shader._get()) {
        Msg("! [MaterialCache] Visual '%s' has no shader", visual->dbg_name.c_str());
        return nullptr;
    }

    Shader* shader = visual->shader._get();

    ShaderElement* elem = shader->E[0]._get();
    if (!elem) {
        Msg("! [MaterialCache] Shader has no element E[0] (deferred mode)");
        return nullptr;
    }

    if (elem->passes.empty()) {
        Msg("! [MaterialCache] ShaderElement has no passes");
        return nullptr;
    }

    SPass* pass = elem->passes[0]._get();
    if (!pass) {
        Msg("! [MaterialCache] First pass is NULL");
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
    Msg("! [MaterialCache] Cache MISS - creating PSO for '%s' (hash: tex=0x%llX, state=0x%llX) [hits=%u, misses=%u]",
        shaderName, textureHash, stateHash, m_stats.numCacheHits, m_stats.numCacheMisses);

    MaterialPSO* pso = CreatePSO(visual, elem, pass, outputs, fg);
    if (!pso) {
        Msg("! [MaterialCache] Failed to create PSO");
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
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg)
{
    auto pso = xr_make_unique<MaterialPSO>();

    pso->pass = pass;

    ExtractTextures(pass, pso.get());

    if (pass && pass->T && !pass->T->empty()) {
        CTexture* baseTex = (*pass->T)[0].second._get();
        if (baseTex && baseTex->cName.size()) {
            pso->detail_scale = GetDetailScale(baseTex->cName);
        }
    }

    if (!ExtractShaders(pass, pso.get())) {
        Msg("! [MaterialCache] Failed to extract shaders");
        return nullptr;
    }

    ExtractSamplers(pass, pso.get());

    CreateBindingLayouts(pso.get());
    if (!pso->vsBindingLayout || !pso->psBindingLayout) {
        Msg("! [MaterialCache] Failed to create binding layouts");
        return nullptr;
    }

    if (!pso->vertexShader->bytecode) {
        Msg("! [MaterialCache] Vertex shader has no bytecode");
        return nullptr;
    }

    ng::ShaderHandle vsHandle = m_device->CreateShader(
        ng::ShaderStage::Vertex,
        pso->vertexShader->bytecode->GetBufferPointer(),
        pso->vertexShader->bytecode->GetBufferSize(),
        pso->vertexShader->cName.c_str());

    if (!vsHandle.IsValid()) {
        Msg("! [MaterialCache] Failed to create vertex shader");
        return nullptr;
    }

    if (!pso->pixelShader->bytecode) {
        Msg("! [MaterialCache] Pixel shader has no bytecode");
        m_device->DestroyShader(vsHandle);
        return nullptr;
    }

    ng::ShaderHandle psHandle = m_device->CreateShader(
        ng::ShaderStage::Pixel,
        pso->pixelShader->bytecode->GetBufferPointer(),
        pso->pixelShader->bytecode->GetBufferSize(),
        pso->pixelShader->cName.c_str());

    if (!psHandle.IsValid()) {
        Msg("! [MaterialCache] Failed to create pixel shader");
        m_device->DestroyShader(vsHandle);
        return nullptr;
    }

    ng::RCShader* rcVS = m_device->GetShader(vsHandle);
    if (!rcVS) {
        Msg("! [MaterialCache] Failed to get VS from handle");
        m_device->DestroyShader(vsHandle);
        m_device->DestroyShader(psHandle);
        return nullptr;
    }

    ng::RCShader* rcPS = m_device->GetShader(psHandle);
    if (!rcPS) {
        Msg("! [MaterialCache] Failed to get PS from handle");
        m_device->DestroyShader(vsHandle);
        m_device->DestroyShader(psHandle);
        return nullptr;
    }

    if (m_vcbPool) {
        auto vsCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(pso->vertexShader->bytecode);
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

        auto psCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(pso->pixelShader->bytecode);
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

    ng::PipelineStateDesc psoDesc;
    psoDesc.vertexShader = rcVS;
    psoDesc.pixelShader = rcPS;

    SetupVertexAttributes(visual, pso.get(), psoDesc);

    SetupRenderStates(pass, psoDesc);

    SetupRenderTargets(pso.get(), outputs, fg, psoDesc);

    psoDesc.debugName = pso->debugName;

    ng::PipelineStateCache* psoCache = m_device->GetPipelineCache();
    if (!psoCache) {
        Msg("! [MaterialCache] Pipeline cache is NULL");
        return nullptr;
    }

    ng::PipelineState* nvrhiPSO = psoCache->GetOrCreate(psoDesc);
    if (!nvrhiPSO) {
        Msg("! [MaterialCache] Failed to create pipeline state");
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

    STextureList* texList = pass->T._get();
    if (!texList) {
        return;
    }

    for (size_t i = 0; i < texList->size(); i++) {
        const auto& texPair = (*texList)[i];
        u32 stage = texPair.first;
        const ref_texture& texRef = texPair.second;

        CTexture* tex = texRef._get();
        if (!tex) {
            Msg("! [MaterialCache] Texture at stage %u is NULL", stage);
            continue;
        }

        auto cacheIt = m_textureWrapperCache.find(tex->cName.c_str());
        if (cacheIt != m_textureWrapperCache.end()) {
            Msg("  [MaterialCache] Texture wrapper cache HIT for '%s'", tex->cName.c_str());
            MaterialPSO::TextureSlot texSlot;
            texSlot.slot = stage;
            texSlot.handle = cacheIt->second;
            matPSO->textures.push_back(texSlot);
            continue;
        }

        ID3DShaderResourceView* srv = tex->get_SRView();
        if (!srv) {
            Msg("! [MaterialCache] Texture '%s' has NULL SRV", tex->cName.c_str());
            continue;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srv->GetDesc(&srvDesc);

        ID3D11Resource* d3dResource = nullptr;
        srv->GetResource(&d3dResource);
        if (!d3dResource) {
            Msg("! [MaterialCache] SRV has NULL resource");
            continue;
        }

        ID3D11Texture2D* d3dTex2D = nullptr;
        HRESULT hr = d3dResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&d3dTex2D);

        if (FAILED(hr) || !d3dTex2D) {
            Msg("! [MaterialCache] Texture '%s' is not a 2D texture", tex->cName.c_str());
            d3dResource->Release();
            continue;
        }

        D3D11_TEXTURE2D_DESC d3dDesc;
        d3dTex2D->GetDesc(&d3dDesc);
        d3dTex2D->Release();

        bool hasBindSRV = (d3dDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0;
        if (!hasBindSRV) {
            Msg("! [MaterialCache] WARNING: Texture '%s' does NOT have D3D11_BIND_SHADER_RESOURCE flag! BindFlags=0x%x",
                tex->cName.c_str(), d3dDesc.BindFlags);
        }

        if (srvDesc.Format != d3dDesc.Format) {
            Msg("! [MaterialCache] Format mismatch for texture '%s': Resource=0x%x (%u), SRV=0x%x (%u)",
                tex->cName.c_str(),
                d3dDesc.Format, d3dDesc.Format,
                srvDesc.Format, srvDesc.Format);
        }

        ng::RenderDevice::TextureDesc texDesc;
        texDesc.width = d3dDesc.Width;
        texDesc.height = d3dDesc.Height;
        texDesc.mipLevels = d3dDesc.MipLevels;
        texDesc.arraySize = d3dDesc.ArraySize;

        if (d3dDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) {
            texDesc.dimension = ng::RenderDevice::TextureDesc::TextureCube;
        }
        else if (d3dDesc.ArraySize > 1) {
            texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2DArray;
        }
        else {
            texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
        }

        texDesc.format = ConvertDxgiFormatToNvrhi(d3dDesc.Format);
        texDesc.isRenderTarget = false;
        texDesc.isUAV = false;
        texDesc.debugName = tex->cName.c_str();

        Msg("  [MaterialCache] Wrapping texture '%s' with format %u", tex->cName.c_str(), texDesc.format);
        ng::TextureHandle nvrhiTex = m_device->CreateTextureFromD3D11(d3dResource, texDesc);
        d3dResource->Release();

        if (!nvrhiTex.IsValid()) {
            Msg("! [MaterialCache] Failed to wrap texture '%s'", tex->cName.c_str());
            continue;
        }

        Msg("  [MaterialCache] Successfully wrapped texture '%s' at slot %u", tex->cName.c_str(), stage);

        m_textureWrapperCache[tex->cName.c_str()] = nvrhiTex;

        MaterialPSO::TextureSlot texSlot;
        texSlot.slot = stage;
        texSlot.handle = nvrhiTex;
        matPSO->textures.push_back(texSlot);
    }
}

bool MaterialCache::ExtractShaders(SPass* pass, MaterialPSO* matPSO)
{
    VERIFY(pass);
    VERIFY(matPSO);

    SVS* vs = pass->vs._get();
    if (!vs) {
        Msg("! [MaterialCache] Pass has NULL vertex shader");
        return false;
    }
    matPSO->vertexShader = vs;

    SPS* ps = pass->ps._get();
    if (!ps) {
        Msg("! [MaterialCache] Pass has NULL pixel shader");
        return false;
    }
    matPSO->pixelShader = ps;

    matPSO->debugName = vs->cName;

    if (vs->sh && vs->bytecode) {
        Msg("! [MaterialCache] Extracting VS input signature from '%s'", vs->cName.c_str());

        matPSO->vsInputSignature = framegraph::ShaderReflector::AnalyzeVertexShader(
            vs->sh,
            vs->bytecode
        );

        Msg("! [MaterialCache] VS input signature extracted: %u elements in shader-expected order",
            matPSO->vsInputSignature.elements.size());
    } else {
        Msg("! [MaterialCache] WARNING: Cannot extract VS input signature - no bytecode available!");
    }

    if (ps->sh && ps->bytecode) {
        Msg("! [MaterialCache] Performing shader reflection on PS '%s'", ps->cName.c_str());

        matPSO->rtBindings = framegraph::ShaderReflector::AnalyzePixelShader(
            ps->sh,
            ps->bytecode
        );

        matPSO->rtBindings.shaderName = ps->cName;

        Msg("! [MaterialCache] Shader reflection complete:");
        Msg("!   Phase: %d", (int)matPSO->rtBindings.phase);
        Msg("!   Input textures: %u", matPSO->rtBindings.inputTextures.size());
        Msg("!   Output RTs: %u", matPSO->rtBindings.outputRTs.size());
    } else {
        Msg("! [MaterialCache] Cannot perform shader reflection - no bytecode available");
    }

    matPSO->constantBuffers.clear();

    auto extractCBsFromShader = [&](R_constant_table& constTable, const char* shaderName, MaterialPSO::ShaderStage stage) {
        for (u32 contextIdx = 0; contextIdx < R__NUM_CONTEXTS; contextIdx++) {
            const auto& cbTable = constTable.m_CBTable[contextIdx];

            if (cbTable.empty())
                continue;

            Msg("  [MaterialCache] Shader '%s' context %u has %u CBs",
                shaderName, contextIdx, cbTable.size());

            for (const auto& cbRecord : cbTable) {
                u32 encodedSlot = cbRecord.first;
                dx11ConstantBuffer* cb = cbRecord.second._get();

                u32 shaderType = dx11ConstantBuffer::DecodeShaderType(encodedSlot);
                u32 bindingSlot = dx11ConstantBuffer::DecodeBindingSlot(encodedSlot);
                const char* shaderTypeName = dx11ConstantBuffer::GetShaderTypeName(shaderType);

                Msg("  [MaterialCache]   Checking encoded slot 0x%02X (%s b%u) in context %u",
                    encodedSlot, shaderTypeName, bindingSlot, contextIdx);

                if (cb) {
                    ID3DBuffer* d3dBuffer = cb->GetBuffer();
                    if (d3dBuffer) {
                        D3D11_BUFFER_DESC bufDesc;
                        d3dBuffer->GetDesc(&bufDesc);

                        Msg("  [MaterialCache]   CB at %s b%u: %u bytes",
                            shaderTypeName, bindingSlot, bufDesc.ByteWidth);

                    bool found = false;
                    for (const auto& existing : matPSO->constantBuffers) {
                        if (existing.slot == bindingSlot && existing.stage == stage) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        const char* cbName = cb->GetBufferName();
                        bool isPerObjectCB = (cbName && xr_strcmp(cbName, "$Globals") == 0);
                        nvrhi::BufferHandle bufferHandle;

                        if (isPerObjectCB) {
                            nvrhi::BufferDesc nvrhiDesc;
                            nvrhiDesc.byteSize = bufDesc.ByteWidth;
                            nvrhiDesc.isConstantBuffer = true;
                            nvrhiDesc.debugName = "XRay_PerObject_CB";
                            nvrhiDesc.keepInitialState = true;
                            nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
                            bufferHandle = m_device->GetNativeDevice()->createHandleForNativeBuffer(
                                nvrhi::ObjectTypes::D3D11_Buffer,
                                nvrhi::Object(d3dBuffer),
                                nvrhiDesc);
                        } else {
                            nvrhi::BufferDesc nvrhiDesc;
                            nvrhiDesc.byteSize = bufDesc.ByteWidth;
                            nvrhiDesc.isConstantBuffer = true;
                            nvrhiDesc.debugName = "FrameGraph_Global_CB";
                            nvrhiDesc.keepInitialState = false;
                            nvrhiDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

                            bufferHandle = m_device->GetNativeDevice()->createBuffer(nvrhiDesc);
                        }

                        if (bufferHandle) {
                            MaterialPSO::ConstantBufferInfo cbInfo;
                            cbInfo.slot = bindingSlot;
                            cbInfo.stage = stage;
                            cbInfo.nvrhiBuffer = bufferHandle;
                            cbInfo.size = bufDesc.ByteWidth;
                            cbInfo.isPerObject = isPerObjectCB;
                            cbInfo.name = cbName;

                            #if 0
                            if (bindingSlot > 0) {
                                void* xrayData = cb->GetBufferData();
                                u32 xraySize = cb->GetBufferSize();

                                if (xrayData && xraySize > 0 && xraySize == bufDesc.ByteWidth) {
                                    cbInfo.initialData.resize(xraySize);
                                    memcpy(cbInfo.initialData.data(), xrayData, xraySize);
                                    Msg("  [MaterialCache]   Extracted %u bytes of global CB data", xraySize);
                                }
                            }
                            #endif

                            matPSO->constantBuffers.push_back(cbInfo);

                            Msg("  [MaterialCache]   SUCCESS: Added CB to PSO at %s b%u: %u bytes (isPerObject=%d)",
                                shaderTypeName, bindingSlot, cbInfo.size, cbInfo.isPerObject);
                        } else {
                            Msg("! [MaterialCache]   FAILED: Could not wrap D3D11 buffer at %s b%u (encoded=0x%02X)",
                                shaderTypeName, bindingSlot, encodedSlot);
                        }
                    }
                }
            }
            }
        }
    };

    extractCBsFromShader(vs->constants, vs->cName.c_str(), MaterialPSO::ShaderStage::Vertex);

    extractCBsFromShader(ps->constants, ps->cName.c_str(), MaterialPSO::ShaderStage::Pixel);

    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.isPerObject) {
            matPSO->perObjectCBSize = cbInfo.size;
            break;
        }
    }

    if (matPSO->perObjectCBSize == 0) {
        Msg("! [MaterialCache] No slot 0 CB found, using default 256 bytes");
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
        Msg("! [MaterialCache] Pass has NULL state - cannot extract samplers");
        return;
    }

    dx11State* d3dState = static_cast<dx11State*>(xrState->state);
    if (!d3dState) {
        Msg("! [MaterialCache] dx11State is NULL - cannot extract samplers");
        return;
    }

    const auto& psSamplers = d3dState->GetPSSamplers();

    Msg("  [MaterialCache]   X-Ray has %u PS sampler slots (valid)", psSamplers.size());

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
        nvrhiDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
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
                    Msg("  [MaterialCache]   ✓ Sampler '%s' at s%u (from X-Ray state)",
                        samplerName, slot);
                }
            }
        }

        if (!samplerInfo.nvrhiSampler) {
            samplerInfo.nvrhiSampler = createDefaultSampler();
            Msg("  [MaterialCache]   ⚠ Sampler '%s' at s%u (using default - not bound in X-Ray)",
                samplerName, slot);
        }

        if (samplerInfo.nvrhiSampler) {
            matPSO->samplers.push_back(samplerInfo);
        } else {
            Msg("! [MaterialCache]   ❌ Failed to create sampler '%s' at s%u",
                samplerName, slot);
        }
    }

    Msg("  [MaterialCache] Extracted %u samplers", matPSO->samplers.size());
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
        Msg("! [MaterialCache] Created per-stage binding layouts:");
        Msg("!   VS Layout: %p", matPSO->vsBindingLayout.Get());
        Msg("!   PS Layout: %p", matPSO->psBindingLayout.Get());
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
    Msg("! [MaterialCache] Creating %s binding layout...", stageName);

    u32 cbCount = 0;
    for (const auto& cbInfo : matPSO->constantBuffers) {
        if (cbInfo.stage == stage) {
            if (cbInfo.isPerObject) {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
                Msg("!   + Volatile CB at slot %u (%u bytes) - %s", cbInfo.slot, cbInfo.size, cbInfo.name.c_str());
            } else {
                layoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
                Msg("!   + CB at slot %u (%u bytes) - %s", cbInfo.slot, cbInfo.size, cbInfo.name.c_str());
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

    Msg("!   Total: %u CBs, %u textures, %u samplers", cbCount, texCount, samplerCount);

    nvrhi::BindingLayoutHandle layout = m_device->CreateBindingLayout(layoutDesc);
    if (!layout) {
        Msg("! [MaterialCache] Failed to create %s binding layout", stageName);
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
            } else {
                if (cbInfo.nvrhiBuffer) {
                    vsBindingDesc.bindings.push_back(
                        nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.nvrhiBuffer.Get()));
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

    for (const auto& texSlot : matPSO->textures) {
        nvrhi::ITexture* nativeTex = m_device->GetNativeTexture(texSlot.handle);

        if (nativeTex) {
            const nvrhi::TextureDesc& texDesc = nativeTex->getDesc();
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(texSlot.slot, nativeTex,
                    nvrhi::Format::UNKNOWN, nvrhi::AllSubresources, texDesc.dimension));
        } else {
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
    Msg("  [MaterialCache] PS: Binding set descriptor now has %u total bindings", psBindingDesc.bindings.size());

    Msg("  [MaterialCache] Creating VS binding set (will be cached)");
    matPSO->vsBindingSet = m_device->CreateBindingSet(
        vsBindingDesc,
        matPSO->vsBindingLayout);

    if (!matPSO->vsBindingSet) {
        Msg("! [MaterialCache] Failed to create VS binding set");
        return nullptr;
    }

    Msg("  [MaterialCache] Creating PS binding set (will be cached)");
    matPSO->psBindingSet = m_device->CreateBindingSet(
        psBindingDesc,
        matPSO->psBindingLayout);

    if (!matPSO->psBindingSet) {
        Msg("! [MaterialCache] Failed to create PS binding set");
        return nullptr;
    }

    Msg("  [MaterialCache] Successfully created and cached VS and PS binding sets");

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
        CTexture* tex = texPair.second._get();
        if (tex) {
            void* texPtr = tex;
            hash = crc32(&texPtr, sizeof(texPtr), hash);
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
            Msg("! [MaterialCache] Unknown DXGI format size: %d", format);
            return 4;
    }
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
            Msg("! [MaterialCache] Unknown visual type %d for vertex layout extraction", visual->getType());
            return;
    }

    if (!meshVisual || !meshVisual->rm_geom || !meshVisual->rm_geom._get())
        return;

    SGeometry* geom = meshVisual->rm_geom._get();
    if (!geom->dcl || !geom->dcl._get())
        return;

    SDeclaration* decl = geom->dcl._get();

    if (!matPSO || matPSO->vsInputSignature.elements.empty()) {
        Msg("! [MaterialCache] ERROR: No VS input signature available! Falling back to vertex decl order (WRONG!)");
    } else {
        Msg("! [MaterialCache] Using VS input signature to build input layout in shader-expected order");
    }

    std::map<u32, u32> bufferStrides;

    Msg("! [MaterialCache] Computing vertex stride for visual '%s'", visual->dbg_name.c_str());
    Msg("!   Declaration has %u elements", (u32)decl->dx11_dcl_code.size());

    for (const auto& d3dElem : decl->dx11_dcl_code) {
        if (!d3dElem.SemanticName)
            continue;

        u32 slot = d3dElem.InputSlot;
        u32 elemSize = GetFormatSize(d3dElem.Format);

        u32 endOffset = d3dElem.AlignedByteOffset + elemSize;

        Msg("!   Element: %s%d @ slot %u, offset %u, format %u, size %u → endOffset %u",
            d3dElem.SemanticName, d3dElem.SemanticIndex,
            slot, d3dElem.AlignedByteOffset, d3dElem.Format, elemSize, endOffset);

        bufferStrides[slot] = std::max(bufferStrides[slot], endOffset);
    }

    for (const auto& [slot, stride] : bufferStrides) {
        Msg("!   → Buffer slot %u: stride = %u bytes", slot, stride);
    }

    if (matPSO && !matPSO->vsInputSignature.elements.empty()) {
        Msg("! [MaterialCache] Building input layout in shader-expected order (%u elements)",
            matPSO->vsInputSignature.elements.size());

        for (u32 shaderIdx = 0; shaderIdx < matPSO->vsInputSignature.elements.size(); ++shaderIdx) {
            const auto& shaderElem = matPSO->vsInputSignature.elements[shaderIdx];

            Msg("!   Shader element [%u]: %s%d",
                shaderIdx, shaderElem.semanticName.c_str(), shaderElem.semanticIndex);

            const D3D11_INPUT_ELEMENT_DESC* matchingDeclElem = nullptr;
            for (const auto& d3dElem : decl->dx11_dcl_code) {
                if (!d3dElem.SemanticName)
                    continue;

                if (xr_strcmp(d3dElem.SemanticName, shaderElem.semanticName.c_str()) == 0 &&
                    d3dElem.SemanticIndex == shaderElem.semanticIndex) {
                    matchingDeclElem = &d3dElem;
                    Msg("!     → Matched to vertex decl: %s%d (offset=%u)",
                        d3dElem.SemanticName, d3dElem.SemanticIndex, d3dElem.AlignedByteOffset);
                    break;
                }
            }

            if (!matchingDeclElem) {
                Msg("! [MaterialCache] WARNING: Shader expects %s%d but vertex decl doesn't have it!",
                    shaderElem.semanticName.c_str(), shaderElem.semanticIndex);
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

            Msg("!   [%u] %s%d: offset=%u, format=%u (from decl), stride=%u",
                shaderIdx, attr.semanticName, attr.semanticIndex,
                attr.offset, (u32)matchingDeclElem->Format, attr.elementStride);
        }

        Msg("! [MaterialCache] Input layout created with %u elements in shader-expected order",
            psoDesc.vertexAttributes.size());
    }

    for (const auto& [slot, stride] : bufferStrides) {
        Msg("  [MaterialCache] Vertex buffer slot %u: stride = %u bytes", slot, stride);
    }
}

namespace {
    ng::CullMode ConvertCullMode(D3D11_CULL_MODE d3dCull) {
        switch (d3dCull) {
            case D3D11_CULL_NONE: return ng::CullMode::None;
            case D3D11_CULL_FRONT: return ng::CullMode::Front;
            case D3D11_CULL_BACK: return ng::CullMode::Back;
            default: return ng::CullMode::Back;
        }
    }

    ng::FillMode ConvertFillMode(D3D11_FILL_MODE d3dFill) {
        switch (d3dFill) {
            case D3D11_FILL_WIREFRAME: return ng::FillMode::Wireframe;
            case D3D11_FILL_SOLID: return ng::FillMode::Solid;
            default: return ng::FillMode::Solid;
        }
    }

    ng::ComparisonFunc ConvertComparisonFunc(D3D11_COMPARISON_FUNC d3dFunc) {
        switch (d3dFunc) {
            case D3D11_COMPARISON_NEVER: return ng::ComparisonFunc::Never;
            case D3D11_COMPARISON_LESS: return ng::ComparisonFunc::Less;
            case D3D11_COMPARISON_EQUAL: return ng::ComparisonFunc::Equal;
            case D3D11_COMPARISON_LESS_EQUAL: return ng::ComparisonFunc::LessEqual;
            case D3D11_COMPARISON_GREATER: return ng::ComparisonFunc::Greater;
            case D3D11_COMPARISON_NOT_EQUAL: return ng::ComparisonFunc::NotEqual;
            case D3D11_COMPARISON_GREATER_EQUAL: return ng::ComparisonFunc::GreaterEqual;
            case D3D11_COMPARISON_ALWAYS: return ng::ComparisonFunc::Always;
            default: return ng::ComparisonFunc::Less;
        }
    }

    ng::BlendFactor ConvertBlendFactor(D3D11_BLEND d3dBlend) {
        switch (d3dBlend) {
            case D3D11_BLEND_ZERO: return ng::BlendFactor::Zero;
            case D3D11_BLEND_ONE: return ng::BlendFactor::One;
            case D3D11_BLEND_SRC_COLOR: return ng::BlendFactor::SrcColor;
            case D3D11_BLEND_INV_SRC_COLOR: return ng::BlendFactor::InvSrcColor;
            case D3D11_BLEND_SRC_ALPHA: return ng::BlendFactor::SrcAlpha;
            case D3D11_BLEND_INV_SRC_ALPHA: return ng::BlendFactor::InvSrcAlpha;
            case D3D11_BLEND_DEST_ALPHA: return ng::BlendFactor::DstAlpha;
            case D3D11_BLEND_INV_DEST_ALPHA: return ng::BlendFactor::InvDstAlpha;
            case D3D11_BLEND_DEST_COLOR: return ng::BlendFactor::DstColor;
            case D3D11_BLEND_INV_DEST_COLOR: return ng::BlendFactor::InvDstColor;
            case D3D11_BLEND_SRC_ALPHA_SAT: return ng::BlendFactor::SrcAlphaSat;
            case D3D11_BLEND_BLEND_FACTOR: return ng::BlendFactor::BlendFactor;
            case D3D11_BLEND_INV_BLEND_FACTOR: return ng::BlendFactor::InvBlendFactor;
            default: return ng::BlendFactor::One;
        }
    }

    ng::BlendOp ConvertBlendOp(D3D11_BLEND_OP d3dOp) {
        switch (d3dOp) {
            case D3D11_BLEND_OP_ADD: return ng::BlendOp::Add;
            case D3D11_BLEND_OP_SUBTRACT: return ng::BlendOp::Subtract;
            case D3D11_BLEND_OP_REV_SUBTRACT: return ng::BlendOp::RevSubtract;
            case D3D11_BLEND_OP_MIN: return ng::BlendOp::Min;
            case D3D11_BLEND_OP_MAX: return ng::BlendOp::Max;
            default: return ng::BlendOp::Add;
        }
    }

    ng::ColorWriteMask ConvertColorWriteMask(u8 d3dMask) {
        ng::ColorWriteMask mask = ng::ColorWriteMask::None;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_RED)   mask = mask | ng::ColorWriteMask::Red;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_GREEN) mask = mask | ng::ColorWriteMask::Green;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_BLUE)  mask = mask | ng::ColorWriteMask::Blue;
        if (d3dMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) mask = mask | ng::ColorWriteMask::Alpha;
        return mask;
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

            psoDesc.depthStencilState.frontFace.compareFunc = ConvertComparisonFunc(dsDesc.FrontFace.StencilFunc);
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
    }
#endif
}

void MaterialCache::SetupRenderTargets(
    MaterialPSO* matPSO,
    const passes::GBufferOutputs& outputs,
    const xray::render::framegraph::FrameGraph& fg,
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
                Msg("! [MaterialCache] Warning: Shader writes Position RT (slot %u)", slot);
                continue;

            case RTSemantic::Emissive:
                Msg("! [MaterialCache] Warning: Shader writes Emissive RT (slot %u)", slot);
                continue;

            case RTSemantic::Accumulator:
                Msg("! [MaterialCache] Warning: Shader writes Accumulator RT (slot %u)", slot);
                continue;

            default:
                Msg("! [MaterialCache] Unknown RT semantic for slot %u", slot);
                continue;
        }

        psoDesc.renderTargetFormats[slot] = format;
        Msg("! [MaterialCache] RT slot %u → %s (format %u)", slot, rtName, (u32)format);
    }

    psoDesc.depthStencilFormat = depthTex->getDesc().format;
}

void MaterialCache::Clear()
{
    m_cache.clear();
    m_textureWrapperCache.clear();
    m_detailScaleCache.clear();
    m_stats = Stats{};
    Msg("* [MaterialCache] Cleared PSO cache and texture wrapper cache");
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
