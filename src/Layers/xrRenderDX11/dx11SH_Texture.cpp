#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"

#include "xrEngine/tntQAVI.h"
#include "xrEngine/xrTheora_Surface.h"
#include "StateManager/dx11ShaderResourceStateCache.h"

#define PRIORITY_HIGH 12
#define PRIORITY_NORMAL 8
#define PRIORITY_LOW 4

void resptrcode_texture::create(LPCSTR _name) { _set(RImplementation.Resources->_CreateTexture(_name)); }
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTexture::CTexture()
{
    pSurface = NULL;
    m_pSRView = NULL;
    pAVI = NULL;
    pTheora = NULL;
    desc_cache = 0;
    seqMSPF = 0;
    flags.MemoryUsage = 0;
    flags.bLoaded = false;
    flags.bUser = false;
    flags.seqCycles = FALSE;
    m_material = 1.0f;
    bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_load);
}

CTexture::~CTexture()
{
    Unload();

    // release external reference
    RImplementation.Resources->_DeleteTexture(this);
}

void CTexture::surface_set(ID3DBaseTexture* surf)
{
#if 0//def DEBUG
    string_path msg_buff;
    xr_sprintf(msg_buff, sizeof(msg_buff), "* Changing texture [%s] current pSurface RefCount =", cName.c_str());
    _SHOW_REF(msg_buff, pSurface);
#endif // DEBUG

    if (surf)
        surf->AddRef();
    _RELEASE(pSurface);

    pSurface = surf;

    if (pSurface)
    {
        desc_update();

        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        if (D3D_RESOURCE_DIMENSION_TEXTURE2D == type)
        {
            D3D_SHADER_RESOURCE_VIEW_DESC ViewDesc{};

            if (desc.MiscFlags & D3D_RESOURCE_MISC_TEXTURECUBE)
            {
                ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
                ViewDesc.TextureCube.MostDetailedMip = 0;
                ViewDesc.TextureCube.MipLevels = desc.MipLevels;
            }
            else
            {
                const bool isArray = desc.ArraySize > 1;
                if (desc.SampleDesc.Count <= 1)
                {
                    ViewDesc.ViewDimension = isArray ? D3D_SRV_DIMENSION_TEXTURE2DARRAY : D3D_SRV_DIMENSION_TEXTURE2D;
                    if (isArray)
                    {
                        ViewDesc.Texture2DArray.MipLevels = desc.MipLevels;
                        ViewDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    }
                    else
                    {
                        ViewDesc.Texture2D.MipLevels = desc.MipLevels;
                    }
                }
                else
                {
                    ViewDesc.ViewDimension = isArray ? D3D_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D_SRV_DIMENSION_TEXTURE2DMS;
                    if (isArray)
                    {
                        ViewDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                    }
                }
            }

            switch (desc.Format)
            {
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                ViewDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;

            case DXGI_FORMAT_R24G8_TYPELESS:
                ViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;

            case DXGI_FORMAT_R32_TYPELESS:
                ViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
                break;

            case DXGI_FORMAT_R16_TYPELESS:
                ViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
                break;
            }

            _RELEASE(srv_all);
            CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, &ViewDesc, &srv_all));

            srv_per_slice.resize(desc.ArraySize);
            for (u32 id = 0; id < desc.ArraySize; ++id)
            {
                _RELEASE(srv_per_slice[id]);

                if (desc.SampleDesc.Count <= 1)
                {
                    ViewDesc.Texture2DArray.ArraySize = 1;
                    ViewDesc.Texture2DArray.FirstArraySlice = id;
                }
                else
                {
                    ViewDesc.Texture2DMSArray.ArraySize = 1;
                    ViewDesc.Texture2DMSArray.FirstArraySlice = id;
                }
                CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, &ViewDesc, &srv_per_slice[id]));
            }
            set_slice(-1);
        }
        else
        {
            _RELEASE(m_pSRView);
            CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, NULL, &m_pSRView));
        }
    }
}

ID3DBaseTexture* CTexture::surface_get() const
{
    if (pSurface)
        pSurface->AddRef();
    return pSurface;
}

void CTexture::PostLoad()
{
    if (pTheora)
        bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_theora);
    else if (pAVI)
        bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_avi);
    else if (!seqDATA.empty())
        bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_seq);
    else
        bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_normal);
}

void CTexture::apply_load(CBackend& cmd_list, u32 dwStage)
{
    if (!flags.bLoaded)
        Load();
    else
        PostLoad();
    bind(cmd_list, dwStage);
};

void CTexture::Apply(CBackend& cmd_list, u32 dwStage) const
{
    // if( !RImplementation.o.msaa )
    //   VERIFY( !((!pSurface)^(!m_pSRView)) );	//	Both present or both missing
    // else
    //{
    // if( ((!pSurface)^(!m_pSRView)) )
    //   return;
    //}

    if (dwStage < rstVertex) //	Pixel shader stage resources
    {
        // HW.pDevice->PSSetShaderResources(dwStage, 1, &m_pSRView);
        cmd_list.SRVSManager.SetPSResource(dwStage, m_pSRView);
    }
    else if (dwStage < rstGeometry) //	Vertex shader stage resources
    {
        // HW.pDevice->VSSetShaderResources(dwStage-rstVertex, 1, &m_pSRView);
        cmd_list.SRVSManager.SetVSResource(dwStage - rstVertex, m_pSRView);
    }
    else if (dwStage < rstHull) //	Geometry shader stage resources
    {
        // HW.pDevice->GSSetShaderResources(dwStage-rstGeometry, 1, &m_pSRView);
        cmd_list.SRVSManager.SetGSResource(dwStage - rstGeometry, m_pSRView);
    }
    else if (dwStage < rstDomain) //	Geometry shader stage resources
    {
        cmd_list.SRVSManager.SetHSResource(dwStage - rstHull, m_pSRView);
    }
    else if (dwStage < rstCompute) //	Geometry shader stage resources
    {
        cmd_list.SRVSManager.SetDSResource(dwStage - rstDomain, m_pSRView);
    }
    else if (dwStage < rstInvalid) //	Geometry shader stage resources
    {
        cmd_list.SRVSManager.SetCSResource(dwStage - rstCompute, m_pSRView);
    }
    else
        VERIFY("Invalid stage");
}

void CTexture::apply_theora(CBackend& cmd_list, u32 dwStage)
{
    if (pTheora->Update(m_play_time != 0xFFFFFFFF ? m_play_time : Device.dwTimeContinual))
    {
        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        R_ASSERT(D3D_RESOURCE_DIMENSION_TEXTURE2D == type);
        ID3DTexture2D* T2D = (ID3DTexture2D*)pSurface;
        D3D_MAPPED_TEXTURE2D mapData;
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = pTheora->Width(true);
        rect.bottom = pTheora->Height(true);

        u32 _w = pTheora->Width(false);

// R_CHK				(T2D->LockRect(0,&R,&rect,0));
#ifdef USE_DX11
        R_CHK(HW.get_context(cmd_list.context_id)->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#else
        R_CHK(T2D->Map(0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#endif
        // R_ASSERT			(R.Pitch == int(pTheora->Width(false)*4));
        R_ASSERT(mapData.RowPitch == int(pTheora->Width(false) * 4));
        int _pos = 0;
        pTheora->DecompressFrame((u32*)mapData.pData, _w - rect.right, _pos);
        VERIFY(u32(_pos) == rect.bottom * _w);
// R_CHK				(T2D->UnlockRect(0));
#ifdef USE_DX11
        HW.get_context(cmd_list.context_id)->Unmap(T2D, 0);
#else
        T2D->Unmap(0);
#endif
    }
    Apply(cmd_list, dwStage);
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
};
void CTexture::apply_avi(CBackend& cmd_list, u32 dwStage) const
{
    if (pAVI->NeedUpdate())
    {
        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        R_ASSERT(D3D_RESOURCE_DIMENSION_TEXTURE2D == type);
        ID3DTexture2D* T2D = (ID3DTexture2D*)pSurface;
        D3D_MAPPED_TEXTURE2D mapData;

// AVI
// R_CHK	(T2D->LockRect(0,&R,NULL,0));
#ifdef USE_DX11
        R_CHK(HW.get_context(CHW::IMM_CTX_ID)->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#else
        R_CHK(T2D->Map(0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#endif
        R_ASSERT(mapData.RowPitch == int(pAVI->m_dwWidth * 4));
        u8* ptr{};
        pAVI->GetFrame(&ptr);
        CopyMemory(mapData.pData, ptr, pAVI->m_dwWidth * pAVI->m_dwHeight * 4);
// R_CHK	(T2D->UnlockRect(0));
#ifdef USE_DX11
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(T2D, 0);
#else
        T2D->Unmap(0);
#endif
    }
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
    Apply(cmd_list, dwStage);
};
void CTexture::apply_seq(CBackend& cmd_list, u32 dwStage)
{
    // SEQ
    u32 frame = Device.dwTimeContinual / seqMSPF; // Device.dwTimeGlobal
    u32 frame_data = seqDATA.size();
    if (flags.seqCycles)
    {
        u32 frame_id = frame % (frame_data * 2);
        if (frame_id >= frame_data)
            frame_id = (frame_data - 1) - (frame_id % frame_data);
        pSurface = seqDATA[frame_id];
        m_pSRView = m_seqSRView[frame_id];
    }
    else
    {
        u32 frame_id = frame % frame_data;
        pSurface = seqDATA[frame_id];
        m_pSRView = m_seqSRView[frame_id];
    }
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
    Apply(cmd_list, dwStage);
};
void CTexture::apply_normal(CBackend& cmd_list, u32 dwStage) const
{
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
    Apply(cmd_list, dwStage);
};

void CTexture::set_slice(int slice)
{
    m_pSRView = (slice < 0) ? srv_all : srv_per_slice[slice];
    curr_slice = slice;
}

void CTexture::Preload()
{
    m_bumpmap = RImplementation.Resources->m_textures_description.GetBumpName(cName);
    m_material = RImplementation.Resources->m_textures_description.GetMaterial(cName);
}

void CTexture::Load()
{
    flags.bLoaded = true;
    desc_cache = 0;
    if (pSurface)
        return;

    flags.bUser = false;
    flags.MemoryUsage = 0;
    if (0 == xr_stricmp(*cName, "$null"))
        return;
    // we need to check only the beginning of the string,
    // so let's use strncmp instead of strstr.
    if (0 == strncmp(cName.c_str(), "$user$", sizeof("$user$") - 1))
    {
        flags.bUser = true;
        return;
    }

    Preload();

    bool bCreateView = true;

    // Check for OGM
    string_path fn;
    if (FS.exist(fn, "$game_textures$", *cName, ".ogm"))
    {
        // AVI
        pTheora = xr_new<CTheoraSurface>();
        m_play_time = 0xFFFFFFFF;

        if (!pTheora->Load(fn))
        {
            xr_delete(pTheora);
            FATAL("Can't open video stream");
        }
        else
        {
            flags.MemoryUsage = pTheora->Width(true) * pTheora->Height(true) * 4;
            pTheora->Play(TRUE, Device.dwTimeContinual);

            // Now create texture
            ID3DTexture2D* pTexture = 0;
            u32 _w = pTheora->Width(false);
            u32 _h = pTheora->Height(false);

            //			HRESULT hrr = HW.pDevice->CreateTexture(
            //				_w, _h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL );
            D3D_TEXTURE2D_DESC desc;
            desc.Width = _w;
            desc.Height = _h;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D_USAGE_DYNAMIC;
            desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            HRESULT hrr = HW.pDevice->CreateTexture2D(&desc, 0, &pTexture);

            pSurface = pTexture;
            if (FAILED(hrr))
            {
                FATAL("Invalid video stream");
                R_CHK(hrr);
                xr_delete(pTheora);
                pSurface = 0;
                m_pSRView = 0;
            }
            else
            {
                CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, 0, &m_pSRView));
            }
        }
    }
    else if (FS.exist(fn, "$game_textures$", *cName, ".avi"))
    {
        // AVI
        pAVI = xr_new<CAviPlayerCustom>();

        if (!pAVI->Load(fn))
        {
            xr_delete(pAVI);
            FATAL("Can't open video stream");
        }
        else
        {
            flags.MemoryUsage = pAVI->m_dwWidth * pAVI->m_dwHeight * 4;

            // Now create texture
            ID3DTexture2D* pTexture = 0;
            // HRESULT hrr = HW.pDevice->CreateTexture(
            // pAVI->m_dwWidth,pAVI->m_dwHeight,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,
            //	&pTexture,NULL
            //	);
            D3D_TEXTURE2D_DESC desc;
            desc.Width = pAVI->m_dwWidth;
            desc.Height = pAVI->m_dwHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D_USAGE_DYNAMIC;
            desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            HRESULT hrr = HW.pDevice->CreateTexture2D(&desc, 0, &pTexture);

            pSurface = pTexture;
            if (FAILED(hrr))
            {
                FATAL("Invalid video stream");
                R_CHK(hrr);
                xr_delete(pAVI);
                pSurface = 0;
                m_pSRView = 0;
            }
            else
            {
                CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, 0, &m_pSRView));
            }
        }
    }
    else if (FS.exist(fn, "$game_textures$", *cName, ".seq"))
    {
        // Sequence
        string256 buffer;
        IReader* _fs = FS.r_open(fn);

        flags.seqCycles = FALSE;
        _fs->r_string(buffer, sizeof(buffer));
        if (0 == xr_stricmp(buffer, "cycled"))
        {
            flags.seqCycles = TRUE;
            _fs->r_string(buffer, sizeof(buffer));
        }
        u32 fps = atoi(buffer);
        seqMSPF = 1000 / fps;

        while (!_fs->eof())
        {
            _fs->r_string(buffer, sizeof(buffer));
            _Trim(buffer);
            if (buffer[0])
            {
                // Load another texture
                u32 mem = 0;
                pSurface = ::RImplementation.texture_load(buffer, mem);
                if (pSurface)
                {
                    // pSurface->SetPriority	(PRIORITY_LOW);
                    seqDATA.push_back(pSurface);
                    m_seqSRView.push_back(0);
                    HW.pDevice->CreateShaderResourceView(seqDATA.back(), NULL, &m_seqSRView.back());
                    flags.MemoryUsage += mem;
                }
            }
        }
        pSurface = 0;
        FS.r_close(_fs);
    }
    else
    {
        // Normal texture
        u32 mem = 0;
        pSurface = ::RImplementation.texture_load(*cName, mem);

        // Calc memory usage and preload into vid-mem
        if (pSurface)
        {
            // pSurface->SetPriority	(PRIORITY_NORMAL);
            flags.MemoryUsage = mem;
            CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, NULL, &m_pSRView));
        }
    }

#ifdef DEBUG
    if (pSurface)
    {
        pSurface->SetPrivateData(WKPDID_D3DDebugObjectName, cName.size(), cName.c_str());
    }
#endif

    PostLoad();
}

void CTexture::Unload()
{
#ifdef DEBUG
    string_path msg_buff;
    xr_sprintf(msg_buff, sizeof(msg_buff), "* Unloading texture [%s] pSurface RefCount =", cName.c_str());
    _SHOW_REF(msg_buff, pSurface);
#endif // DEBUG

    //.	if (flags.bLoaded)		Msg		("* Unloaded: %s",cName.c_str());

    flags.bLoaded = FALSE;
    if (!seqDATA.empty())
    {
        for (u32 I = 0; I < seqDATA.size(); I++)
        {
            _RELEASE(seqDATA[I]);
            _RELEASE(m_seqSRView[I]);
        }
        seqDATA.clear();
        m_seqSRView.clear();
        pSurface = 0;
    }

    _RELEASE(pSurface);
    _RELEASE(srv_all);
    for (auto& srv : srv_per_slice)
    {
        _RELEASE(srv);
    }


    xr_delete(pAVI);
    xr_delete(pTheora);

    bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_load);
}

void CTexture::desc_update()
{
    desc_cache = pSurface;
    if (pSurface)
    {
        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        if (D3D_RESOURCE_DIMENSION_TEXTURE2D == type)
        {
            ID3DTexture2D* T = (ID3DTexture2D*)pSurface;
            T->GetDesc(&desc);
            m_width = desc.Width;
            m_height = desc.Height;
        }
    }
}

D3D_USAGE CTexture::GetUsage()
{
    D3D_USAGE res = D3D_USAGE_DEFAULT;

    if (pSurface)
    {
        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        switch (type)
        {
        case D3D_RESOURCE_DIMENSION_TEXTURE1D:
        {
            ID3DTexture1D* T = (ID3DTexture1D*)pSurface;
            D3D_TEXTURE1D_DESC descr;
            T->GetDesc(&descr);
            res = descr.Usage;
        }
        break;

        case D3D_RESOURCE_DIMENSION_TEXTURE2D:
        {
            ID3DTexture2D* T = (ID3DTexture2D*)pSurface;
            D3D_TEXTURE2D_DESC descr;
            T->GetDesc(&descr);
            res = descr.Usage;
        }
        break;

        case D3D_RESOURCE_DIMENSION_TEXTURE3D:
        {
            ID3DTexture3D* T = (ID3DTexture3D*)pSurface;
            D3D_TEXTURE3D_DESC descr;
            T->GetDesc(&descr);
            res = descr.Usage;
        }
        break;

        default: VERIFY(!"Unknown texture format???");
        }
    }

    return res;
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
