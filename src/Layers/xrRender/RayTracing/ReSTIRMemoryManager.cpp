#include "stdafx.h"
#include "ReSTIRMemoryManager.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdint>

extern ENGINE_API int ps_r_rt_gi_half;

namespace xray::render::fg {

static constexpr int kSTBNSize = 128;
static constexpr int kSTBNDepth = 16;

static void GenerateBlueNoise128(xr_vector<u8>& dst)
{
    constexpr int N = kSTBNSize;
    constexpr int Count = N * N;
    dst.assign(Count, 0);
    xr_vector<float> energy(Count, 0.f);
    xr_vector<u8> occupied(Count, 0);
    auto addImpulse = [&](int cx, int cy, float sign) {
        for (int y = -16; y <= 16; ++y) {
            for (int x = -16; x <= 16; ++x) {
                const float d2 = float(x * x + y * y);
                const float g = expf(-d2 * 0.08f);
                const int ix = (cx + x + N * 4) % N;
                const int iy = (cy + y + N * 4) % N;
                energy[iy * N + ix] += sign * g;
            }
        }
    };
    for (int i = 0; i < Count; ++i) {
        int best = 0;
        float bestE = 1e30f;
        for (int p = 0; p < Count; ++p) {
            if (occupied[p])
                continue;
            if (energy[p] < bestE) {
                bestE = energy[p];
                best = p;
            }
        }
        occupied[best] = 1;
        dst[best] = u8((i * 255) / (Count - 1));
        addImpulse(best % N, best / N, 1.f);
    }
}

static void GenerateSTBN(xr_vector<u8>& dst)
{
    constexpr int N = kSTBNSize;
    constexpr int D = kSTBNDepth;
    constexpr int Slice = N * N;
    xr_vector<u8> slice;
    GenerateBlueNoise128(slice);
    dst.assign((size_t)Slice * D, 0);
    memcpy(dst.data(), slice.data(), (size_t)Slice);
    for (int z = 1; z < D; ++z) {
        const int sx = (z * 47 + 13) & (N - 1);
        const int sy = (z * 119 + 31) & (N - 1);
        u8* cur = dst.data() + (size_t)z * Slice;
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                const int srcx = (x + sx) ^ (z * 17);
                const int srcy = (y + sy) ^ (z * 41);
                cur[y * N + x] = slice[(srcy & (N - 1)) * N + (srcx & (N - 1))];
            }
        }
        const u8* prev = dst.data() + (size_t)(z - 1) * Slice;
        for (int i = 0; i < Slice; ++i) {
            if (abs((int)cur[i] - (int)prev[i]) < 8) {
                const int j = (i + 37 * z + 11) % Slice;
                const u8 tmp = cur[i];
                cur[i] = cur[j];
                cur[j] = tmp;
            }
        }
    }
}

static void RestirGiResolution(u32 width, u32 height, u32& giW, u32& giH)
{
    if (ps_r_rt_gi_half)
    {
        giW = std::max(1u, (width + 1u) / 2u);
        giH = std::max(1u, (height + 1u) / 2u);
    }
    else
    {
        giW = width;
        giH = height;
    }
}

ReSTIRMemoryManager& ReSTIRMemoryManager::Instance()
{
    static ReSTIRMemoryManager instance;
    return instance;
}

void ReSTIRMemoryManager::ClearBindingSets()
{
    m_temporalBS[0] = nullptr;
    m_temporalBS[1] = nullptr;
    m_spatialBS[0] = nullptr;
    m_spatialBS[1] = nullptr;
}

void ReSTIRMemoryManager::Init(nvrhi::IDevice* device)
{
    m_device = device;
    if (!m_placeholderBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "ReSTIR_PlaceholderBuf";
        desc.byteSize = 16;
        desc.canHaveRawViews = true;
        desc.canHaveUAVs = true;
        desc.structStride = 16;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_placeholderBuffer = device->createBuffer(desc);
    }
    if (!m_placeholderCube)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_PlaceholderCube";
        desc.width = 1;
        desc.height = 1;
        desc.dimension = nvrhi::TextureDimension::TextureCube;
        desc.arraySize = 6;
        desc.format = nvrhi::Format::RGBA8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_placeholderCube = device->createTexture(desc);
    }
    if (!m_placeholderTex)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_PlaceholderTex";
        desc.width = 1;
        desc.height = 1;
        desc.format = nvrhi::Format::R16_FLOAT;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_placeholderTex = device->createTexture(desc);
    }
    if (!m_placeholderColorTex)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_PlaceholderColor";
        desc.width = 1;
        desc.height = 1;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_placeholderColorTex = device->createTexture(desc);
    }
    if (!m_lightData)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "ReSTIR_LightData";
        desc.byteSize = RESTIR_MAX_LIGHTS * 128u;
        desc.structStride = 128;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_lightData = device->createBuffer(desc);
    }
    if (!m_placeholderTex3D)
    {
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_PlaceholderTex3D";
        desc.width = 1;
        desc.height = 1;
        desc.depth = 1;
        desc.dimension = nvrhi::TextureDimension::Texture3D;
        desc.format = nvrhi::Format::R8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_placeholderTex3D = device->createTexture(desc);
    }
    if (!m_blueNoise)
    {
        GenerateSTBN(m_blueNoisePixels);
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_STBN";
        desc.width = kSTBNSize;
        desc.height = kSTBNSize;
        desc.depth = kSTBNDepth;
        desc.dimension = nvrhi::TextureDimension::Texture3D;
        desc.format = nvrhi::Format::R8_UNORM;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_blueNoise = device->createTexture(desc);
        m_blueNoiseUploaded = false;
    }
}

void ReSTIRMemoryManager::Shutdown()
{
    DestroyResources();
    ClearBindingSets();
    m_lightData = nullptr;
    m_placeholderBuffer = nullptr;
    m_placeholderCube = nullptr;
    m_placeholderTex = nullptr;
    m_placeholderColorTex = nullptr;
    m_placeholderTex3D = nullptr;
    m_blueNoise = nullptr;
    m_blueNoisePixels.clear();
    m_blueNoiseUploaded = false;
    m_device = nullptr;
}

void ReSTIRMemoryManager::DestroyResources()
{
    if (!m_device)
        return;

    ClearBindingSets();
    for (int i = 0; i < 2; ++i) {
        m_reservoir[i] = nullptr;
        m_diReservoir[i] = nullptr;
        m_specReservoirA[i] = nullptr;
        m_specReservoirB[i] = nullptr;
        m_ptReservoirA[i] = nullptr;
        m_ptReservoirB[i] = nullptr;
    }
    m_ptDupMap = nullptr;
    m_pairingTex[0] = nullptr;
    m_pairingTex[1] = nullptr;
    m_pairingTex[2] = nullptr;
    m_blurTemp = nullptr;
    m_blurTempSpec = nullptr;
    m_histDiffuse = nullptr;
    m_histSpecular = nullptr;
    m_sunVis[0] = nullptr;
    m_sunVis[1] = nullptr;
    m_irradianceCache = nullptr;
    m_irradianceCacheSize = 0;
    m_directLighting = nullptr;
    m_noisyDiffuse = nullptr;
    m_noisySpecular = nullptr;
    m_hitDistance = nullptr;
    m_wetAccum = nullptr;
    m_skyOpen = nullptr;
    m_sunshafts = nullptr;
    m_sunshaftsHist = nullptr;
    m_ddgiAmbient = nullptr;
    m_ddgiProbes = nullptr;
    m_width = 0;
    m_height = 0;
    m_giWidth = 0;
    m_giHeight = 0;
    m_shaftWidth = 0;
    m_shaftHeight = 0;

    m_device->waitForIdle();
    m_device->runGarbageCollection();
}

void ReSTIRMemoryManager::CreatePersistent(u32 width, u32 height)
{
    u32 giW = width;
    u32 giH = height;
    RestirGiResolution(width, height, giW, giH);
    const u32 shaftW = std::max(1u, (width + 1u) / 2u);
    const u32 shaftH = std::max(1u, (height + 1u) / 2u);
    const u32 pixelCount = giW * giH;
    for (int i = 0; i < 2; ++i)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = i == 0 ? "ReSTIR_Reservoir_0" : "ReSTIR_Reservoir_1";
        desc.byteSize = (size_t)pixelCount * RESTIR_RESERVOIR_STRIDE;
        desc.structStride = RESTIR_RESERVOIR_STRIDE;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_reservoir[i] = m_device->createBuffer(desc);
    }

    auto makeUAVTex = [&](const char* name, nvrhi::Format fmt, u32 w, u32 h) -> nvrhi::TextureHandle {
        nvrhi::TextureDesc desc;
        desc.debugName = name;
        desc.width = w;
        desc.height = h;
        desc.format = fmt;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        nvrhi::TextureHandle tex = m_device->createTexture(desc);
        if (!tex)
            Msg("! [ReSTIR] Failed to create UAV texture '%s' %ux%u fmt=%d", name, w, h, (int)fmt);
        return tex;
    };

    m_directLighting = makeUAVTex("ReSTIR_DirectLighting", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_noisyDiffuse = makeUAVTex("ReSTIR_NoisyDiffuse", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_noisySpecular = makeUAVTex("ReSTIR_NoisySpecular", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_hitDistance = makeUAVTex("ReSTIR_HitDistance", nvrhi::Format::R16_FLOAT, giW, giH);
    m_wetAccum = makeUAVTex("ReSTIR_WetAccum", nvrhi::Format::R16_FLOAT, width, height);
    m_skyOpen = makeUAVTex("ReSTIR_SkyOpen", nvrhi::Format::R16_FLOAT, width, height);
    m_sunshafts = makeUAVTex("ReSTIR_Sunshafts", nvrhi::Format::RGBA16_FLOAT, shaftW, shaftH);
    m_sunshaftsHist = makeUAVTex("ReSTIR_SunshaftsHist", nvrhi::Format::RGBA16_FLOAT, shaftW, shaftH);
    m_ddgiAmbient = makeUAVTex("ReSTIR_DDGIAmbient", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_diReservoir[0] = makeUAVTex("ReSTIR_DI_0", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    m_diReservoir[1] = makeUAVTex("ReSTIR_DI_1", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    m_specReservoirA[0] = makeUAVTex("ReSTIR_SpecA_0", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    m_specReservoirA[1] = makeUAVTex("ReSTIR_SpecA_1", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    m_specReservoirB[0] = makeUAVTex("ReSTIR_SpecB_0", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    m_specReservoirB[1] = makeUAVTex("ReSTIR_SpecB_1", nvrhi::Format::RGBA32_FLOAT, giW, giH);
    if (!m_diReservoir[0])
        m_diReservoir[0] = makeUAVTex("ReSTIR_DI_0", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    if (!m_diReservoir[1])
        m_diReservoir[1] = makeUAVTex("ReSTIR_DI_1", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    if (!m_specReservoirA[0])
        m_specReservoirA[0] = makeUAVTex("ReSTIR_SpecA_0", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    if (!m_specReservoirA[1])
        m_specReservoirA[1] = makeUAVTex("ReSTIR_SpecA_1", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    if (!m_specReservoirB[0])
        m_specReservoirB[0] = makeUAVTex("ReSTIR_SpecB_0", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    if (!m_specReservoirB[1])
        m_specReservoirB[1] = makeUAVTex("ReSTIR_SpecB_1", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_blurTemp = makeUAVTex("ReSTIR_BlurTemp", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_blurTempSpec = makeUAVTex("ReSTIR_BlurTempSpec", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_histDiffuse = makeUAVTex("ReSTIR_HistDiffuse", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_histSpecular = makeUAVTex("ReSTIR_HistSpecular", nvrhi::Format::RGBA16_FLOAT, giW, giH);
    m_sunVis[0] = makeUAVTex("ReSTIR_SunVis_0", nvrhi::Format::R16_FLOAT, giW, giH);
    m_sunVis[1] = makeUAVTex("ReSTIR_SunVis_1", nvrhi::Format::R16_FLOAT, giW, giH);
    m_ptReservoirA[0] = makeUAVTex("ReSTIR_PTA_0", nvrhi::Format::RGBA32_UINT, giW, giH);
    m_ptReservoirA[1] = makeUAVTex("ReSTIR_PTA_1", nvrhi::Format::RGBA32_UINT, giW, giH);
    m_ptReservoirB[0] = makeUAVTex("ReSTIR_PTB_0", nvrhi::Format::RGBA32_UINT, giW, giH);
    m_ptReservoirB[1] = makeUAVTex("ReSTIR_PTB_1", nvrhi::Format::RGBA32_UINT, giW, giH);
    m_ptDupMap = makeUAVTex("ReSTIR_PTDup", nvrhi::Format::R8_UNORM, giW, giH);
    {
        auto makePair = [&](const char* name, int dim, int shuffles) {
            nvrhi::TextureDesc desc;
            desc.debugName = name;
            desc.width = (u32)dim;
            desc.height = (u32)dim;
            desc.format = nvrhi::Format::RG8_SNORM;
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            nvrhi::TextureHandle tex = m_device->createTexture(desc);
            xr_vector<int> idx((size_t)dim * dim);
            for (int i = 0; i < dim * dim; ++i)
                idx[i] = i;
            u32 rng = 0xA341316Cu + (u32)shuffles * 747796405u;
            auto next = [&]() {
                rng = rng * 1664525u + 1013904223u;
                return rng;
            };
            for (int s = 0; s < shuffles; ++s) {
                for (int y = 0; y + 1 < dim; y += 2) {
                    for (int x = 0; x + 1 < dim; x += 2) {
                        int p[4] = { y * dim + x, y * dim + x + 1, (y + 1) * dim + x, (y + 1) * dim + x + 1 };
                        for (int k = 3; k > 0; --k) {
                            int j = (int)(next() % (u32)(k + 1));
                            std::swap(p[k], p[j]);
                        }
                        int a = idx[p[0]], b = idx[p[1]], c = idx[p[2]], d = idx[p[3]];
                        idx[p[0]] = b; idx[p[1]] = a; idx[p[2]] = d; idx[p[3]] = c;
                    }
                }
            }
            xr_vector<int8_t> pix((size_t)dim * dim * 2, 0);
            for (int y = 0; y < dim; ++y) {
                for (int x = 0; x < dim; ++x) {
                    int i = y * dim + x;
                    int p = idx[i];
                    int px = p % dim;
                    int py = p / dim;
                    int dx = std::clamp(px - x, -127, 127);
                    int dy = std::clamp(py - y, -127, 127);
                    pix[(size_t)i * 2] = (int8_t)dx;
                    pix[(size_t)i * 2 + 1] = (int8_t)dy;
                }
            }
            if (tex) {
                nvrhi::CommandListHandle cmd = m_device->createCommandList();
                cmd->open();
                cmd->writeTexture(tex, 0, 0, pix.data(), (size_t)dim * 2);
                cmd->close();
                m_device->executeCommandList(cmd);
            }
            return tex;
        };
        m_pairingTex[0] = makePair("ReSTIR_Pair254", 254, 8);
        m_pairingTex[1] = makePair("ReSTIR_Pair230", 230, 6);
        m_pairingTex[2] = makePair("ReSTIR_Pair210", 210, 5);
    }

    {
        nvrhi::TextureDesc desc;
        desc.debugName = "ReSTIR_DDGIProbes";
        desc.width = 16;
        desc.height = 8;
        desc.depth = 16;
        desc.dimension = nvrhi::TextureDimension::Texture3D;
        desc.format = nvrhi::Format::RGBA16_FLOAT;
        desc.isUAV = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_ddgiProbes = m_device->createTexture(desc);
    }

    m_width = width;
    m_height = height;
    m_giWidth = giW;
    m_giHeight = giH;
    m_shaftWidth = shaftW;
    m_shaftHeight = shaftH;
    m_resetHistory = true;
    Msg("* [ReSTIR] Persistent resources full %ux%u gi %ux%u%s shafts %ux%u (lights max %u)",
        width, height, giW, giH, ps_r_rt_gi_half ? " half" : " full", shaftW, shaftH, RESTIR_MAX_LIGHTS);
}

void ReSTIRMemoryManager::Ensure(u32 width, u32 height)
{
    if (!m_device || width == 0 || height == 0)
        return;
    u32 giW = width;
    u32 giH = height;
    RestirGiResolution(width, height, giW, giH);
    const u32 shaftW = std::max(1u, (width + 1u) / 2u);
    const u32 shaftH = std::max(1u, (height + 1u) / 2u);
    const bool sizeOk = m_reservoir[0] && m_width == width && m_height == height
        && m_giWidth == giW && m_giHeight == giH
        && m_shaftWidth == shaftW && m_shaftHeight == shaftH;
    const bool complete = sizeOk && m_diReservoir[0] && m_specReservoirA[0] && m_specReservoirB[0]
        && m_blurTemp && m_sunshafts && m_sunshaftsHist && m_directLighting && m_skyOpen
        && m_sunVis[0] && m_sunVis[1];
    if (complete)
        return;

    DestroyResources();
    CreatePersistent(width, height);
}

nvrhi::IBuffer* ReSTIRMemoryManager::GetReservoirBuffer(u32 index) const
{
    return m_reservoir[index & 1];
}

void ReSTIRMemoryManager::RequestHistoryReset()
{
    m_resetHistory = true;
}

bool ReSTIRMemoryManager::ConsumeHistoryReset()
{
    const bool reset = m_resetHistory;
    m_resetHistory = false;
    return reset;
}

void ReSTIRMemoryManager::ClearHistoryTargets(nvrhi::ICommandList* cmd)
{
    if (!cmd)
        return;
    const nvrhi::Color z(0.f);
    if (m_histDiffuse)
        cmd->clearTextureFloat(m_histDiffuse, nvrhi::AllSubresources, z);
    if (m_histSpecular)
        cmd->clearTextureFloat(m_histSpecular, nvrhi::AllSubresources, z);
    if (m_ddgiAmbient)
        cmd->clearTextureFloat(m_ddgiAmbient, nvrhi::AllSubresources, z);
    if (m_ddgiProbes)
        cmd->clearTextureFloat(m_ddgiProbes, nvrhi::AllSubresources, z);
    if (m_diReservoir[0])
        cmd->clearTextureFloat(m_diReservoir[0], nvrhi::AllSubresources, z);
    if (m_diReservoir[1])
        cmd->clearTextureFloat(m_diReservoir[1], nvrhi::AllSubresources, z);
    if (m_noisyDiffuse)
        cmd->clearTextureFloat(m_noisyDiffuse, nvrhi::AllSubresources, z);
    if (m_noisySpecular)
        cmd->clearTextureFloat(m_noisySpecular, nvrhi::AllSubresources, z);
    if (m_irradianceCache)
        cmd->clearBufferUInt(m_irradianceCache, 0);
    if (m_reservoir[0])
        cmd->clearBufferUInt(m_reservoir[0], 0);
    if (m_reservoir[1])
        cmd->clearBufferUInt(m_reservoir[1], 0);
    if (m_sunVis[0])
        cmd->clearTextureFloat(m_sunVis[0], nvrhi::AllSubresources, z);
    if (m_sunVis[1])
        cmd->clearTextureFloat(m_sunVis[1], nvrhi::AllSubresources, z);
    if (m_sunshaftsHist)
        cmd->clearTextureFloat(m_sunshaftsHist, nvrhi::AllSubresources, z);
    if (m_sunshafts)
        cmd->clearTextureFloat(m_sunshafts, nvrhi::AllSubresources, z);
    if (m_skyOpen)
        cmd->clearTextureFloat(m_skyOpen, nvrhi::AllSubresources, z);
}

void ReSTIRMemoryManager::EnsureBlueNoiseUploaded(nvrhi::ICommandList* cmd)
{
    if (m_blueNoiseUploaded || !cmd || !m_blueNoise ||
        m_blueNoisePixels.size() < (size_t)kSTBNSize * kSTBNSize * kSTBNDepth)
        return;
    cmd->writeTexture(m_blueNoise, 0, 0, m_blueNoisePixels.data(), kSTBNSize,
        (size_t)kSTBNSize * kSTBNSize);
    m_blueNoiseUploaded = true;
}

void ReSTIRMemoryManager::EnsureIrradianceCache(u32 entries)
{
    if (!m_device)
        return;
    if (m_irradianceCache && m_irradianceCacheSize == entries)
        return;
    nvrhi::BufferDesc desc;
    desc.debugName = "RTGI_IrradianceCache";
    const u32 count = entries > 0 ? entries : 1u;
    desc.byteSize = (size_t)count * 16u;
    desc.structStride = 16;
    desc.canHaveUAVs = true;
    desc.canHaveRawViews = true;
    desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    desc.keepInitialState = true;
    m_irradianceCache = m_device->createBuffer(desc);
    m_irradianceCacheSize = entries;
}

void ReSTIRMemoryManager::UploadLights(nvrhi::ICommandList* cmd, const void* lights, u32 lightCount, u32 strideBytes)
{
    if (!cmd || !m_lightData || !lights || strideBytes == 0)
        return;
    u32 count = lightCount;
    if (count > RESTIR_MAX_LIGHTS)
        count = RESTIR_MAX_LIGHTS;
    if (count == 0)
        return;
    cmd->writeBuffer(m_lightData, lights, (size_t)count * strideBytes);
}

}
