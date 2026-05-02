#include "stdafx.h"

#include "SH_Texture.h"
#include "ResourceManager/DDSLoader.h"
#include "RenderContext/RenderDevice.h"
#include "r_FrameGraphRenderer.h"

#include "xrEngine/tntQAVI.h"
#include "xrEngine/xrTheora_Surface.h"

namespace xray::render::fg
{
void fix_texture_name(pstr fn)
{
    pstr _ext = strext(fn);
    if (_ext && (!xr_stricmp(_ext, ".tga") || !xr_stricmp(_ext, ".dds") || !xr_stricmp(_ext, ".bmp") ||
                 !xr_stricmp(_ext, ".ogm")))
    {
        *_ext = 0;
    }
}

namespace
{
nvrhi::TextureHandle nvrhi_texture_load(pcstr name, u32& mem)
{
    mem = 0;

    auto* renderDevice = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
    if (!renderDevice)
        return {};

    auto* device = renderDevice->GetNVRHIDevice();
    if (!device)
        return {};

    resources::DDSData ddsData;
    if (!resources::DDSLoader::LoadFromFile(name, ddsData) || !ddsData.isValid || ddsData.mipLevels.empty())
        return {};

    if (ddsData.type != resources::DDSData::TextureType::Static)
        return {};

    nvrhi::TextureDesc texDesc;
    texDesc.width = ddsData.desc.width;
    texDesc.height = ddsData.desc.height;
    texDesc.depth = ddsData.desc.depth;
    texDesc.arraySize = ddsData.desc.arraySize;
    texDesc.mipLevels = ddsData.desc.mipLevels;
    texDesc.format = ddsData.desc.format;
    texDesc.dimension = (ddsData.desc.type == resources::TextureDesc::TextureCube)
        ? nvrhi::TextureDimension::TextureCube
        : nvrhi::TextureDimension::Texture2D;
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;
    texDesc.debugName = name;

    nvrhi::TextureHandle tex = device->createTexture(texDesc);
    if (!tex)
        return {};

    const u32 mipsPerSlice = texDesc.mipLevels;
    for (u32 slice = 0; slice < texDesc.arraySize; ++slice)
    {
        for (u32 mip = 0; mip < mipsPerSlice; ++mip)
        {
            const auto& ml = ddsData.mipLevels[slice * mipsPerSlice + mip];
            renderDevice->UploadTextureDataToNVRHI(tex, slice, mip, ml.data, ml.size, ml.rowPitch, ml.slicePitch);
        }
    }

    mem = static_cast<u32>(ddsData.totalDataSize);
    return tex;
}

nvrhi::TextureHandle make_video_texture(u32 width, u32 height, pcstr name)
{
    auto* device = GEnv.Render ? GEnv.Render->GetRenderDevice()->GetNVRHIDevice() : nullptr;
    if (!device)
        return {};

    nvrhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA8_UNORM;
    desc.dimension = nvrhi::TextureDimension::Texture2D;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.debugName = name;
    return device->createTexture(desc);
}
}

void resptrcode_texture::create(LPCSTR _name) { _set(RImplementation.Resources->_CreateTexture(_name)); }

CTexture::CTexture()
    : pAVI(nullptr)
    , pTheora(nullptr)
    , m_material(1.0f)
    , m_play_time(0)
{
    flags.bLoaded = false;
    flags.bUser = false;
    flags.seqCycles = FALSE;
    flags.MemoryUsage = 0;
}

CTexture::~CTexture()
{
    Unload();
    RImplementation.Resources->_DeleteTexture(this);
}

void CTexture::surface_set(nvrhi::TextureHandle tex)
{
    nvrhiTexture = tex;
    desc_cache = nullptr;
}

void CTexture::desc_update()
{
    desc_cache = nvrhiTexture.Get();
    if (nvrhiTexture)
    {
        const auto& d = nvrhiTexture->getDesc();
        m_width = d.width;
        m_height = d.height;
    }
    else
    {
        m_width = 0;
        m_height = 0;
    }
}

void CTexture::PostLoad() {}

void CTexture::set_slice(int slice)
{
    curr_slice = slice;
}

void CTexture::Preload()
{
    m_bumpmap = TextureDescr.GetBumpName(cName);
    m_material = TextureDescr.GetMaterial(cName);
    m_metallic = TextureDescr.GetMetallicName(cName);
    m_roughness = TextureDescr.GetRoughnessName(cName);
    m_ao = TextureDescr.GetAOName(cName);
    m_parallax = TextureDescr.GetParallaxName(cName);
}

void CTexture::Load()
{
    flags.bLoaded = true;
    desc_cache = nullptr;
    if (nvrhiTexture)
        return;

    flags.bUser = false;
    flags.MemoryUsage = 0;
    if (0 == xr_stricmp(cName.c_str(), "$null"))
        return;
    if (0 == strncmp(cName.c_str(), "$user$", sizeof("$user$") - 1))
    {
        flags.bUser = true;
        return;
    }

    ZoneScoped;

    Preload();

    string_path fn;
    if (FS.exist(fn, "$game_textures$", cName.c_str(), ".ogm"))
    {
        pTheora = xr_new<CTheoraSurface>();
        m_play_time = 0xFFFFFFFF;

        if (!pTheora->Load(fn))
        {
            xr_delete(pTheora);
            FATAL("Can't open video stream");
        }
        else
        {
            const u32 w = pTheora->Width(false);
            const u32 h = pTheora->Height(false);
            flags.MemoryUsage = pTheora->Width(true) * pTheora->Height(true) * 4;
            pTheora->Play(TRUE, Device.dwTimeContinual);
            nvrhiTexture = make_video_texture(w, h, cName.c_str());
            if (!nvrhiTexture)
            {
                FATAL("Invalid video stream");
                xr_delete(pTheora);
            }
        }
    }
    else if (FS.exist(fn, "$game_textures$", cName.c_str(), ".avi"))
    {
        pAVI = xr_new<CAviPlayerCustom>();
        if (!pAVI->Load(fn))
        {
            xr_delete(pAVI);
            FATAL("Can't open video stream");
        }
        else
        {
            flags.MemoryUsage = pAVI->m_dwWidth * pAVI->m_dwHeight * 4;
            nvrhiTexture = make_video_texture(pAVI->m_dwWidth, pAVI->m_dwHeight, cName.c_str());
            if (!nvrhiTexture)
            {
                FATAL("Invalid video stream");
                xr_delete(pAVI);
            }
        }
    }
    else if (FS.exist(fn, "$game_textures$", cName.c_str(), ".seq"))
    {
        string256 buffer;
        IReader* _fs = FS.r_open(fn);

        flags.seqCycles = FALSE;
        _fs->r_string(buffer, sizeof(buffer));
        if (0 == xr_stricmp(buffer, "cycled"))
        {
            flags.seqCycles = TRUE;
            _fs->r_string(buffer, sizeof(buffer));
        }
        const u32 fps = atoi(buffer);
        seqMSPF = 1000 / fps;

        while (!_fs->eof())
        {
            _fs->r_string(buffer, sizeof(buffer));
            _Trim(buffer);
            if (buffer[0])
            {
                u32 mem = 0;
                nvrhi::TextureHandle frameTex = nvrhi_texture_load(buffer, mem);
                if (frameTex)
                {
                    seqNvrhiTextures.push_back(frameTex);
                    flags.MemoryUsage += mem;
                }
            }
        }
        FS.r_close(_fs);
    }
    else
    {
        u32 mem = 0;
        nvrhiTexture = nvrhi_texture_load(cName.c_str(), mem);
        if (nvrhiTexture)
        {
            flags.MemoryUsage = mem;
            desc_update();
        }
    }

    PostLoad();
}

void CTexture::Unload()
{
    ZoneScoped;
    flags.bLoaded = FALSE;

    seqNvrhiTextures.clear();
    nvrhiTexture = nullptr;
    desc_cache = nullptr;

    xr_delete(pAVI);
    xr_delete(pTheora);
}

void CTexture::video_Play(BOOL looped, u32 _time)
{
    if (pTheora)
        pTheora->Play(looped, (_time != 0xFFFFFFFF) ? (m_play_time = _time) : Device.dwTimeContinual);
}

void CTexture::video_Pause(BOOL state) const
{
    if (pTheora)
        pTheora->Pause(state);
}

void CTexture::video_Stop() const
{
    if (pTheora)
        pTheora->Stop();
}

BOOL CTexture::video_IsPlaying() const
{
    return (pTheora) ? pTheora->IsPlaying() : FALSE;
}

ImTextureID CTexture::GetImTextureID()
{
    if (!flags.bLoaded)
        Load();
    return reinterpret_cast<ImTextureID>(nvrhiTexture.Get());
}
}
