#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"

#ifndef _EDITOR
#include "xrEngine/Render.h"
#endif

#include "xrEngine/tntQAVI.h"
#include "xrEngine/xrTheora_Surface.h"
#include "StateManager/dx10ShaderResourceStateCache.h"

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
    flags.bLoadedAsStaging = FALSE;
    m_material = 1.0f;
    bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_load);
}

CTexture::~CTexture()
{
    Unload();

    // release external reference
    RImplementation.Resources->_DeleteTexture(this);
}

void CTexture::surface_set(ID3DBaseTexture* surf)
{
    if (surf)
        surf->AddRef();
    _RELEASE(pSurface);
    _RELEASE(m_pSRView);

    pSurface = surf;

    if (pSurface)
    {
        desc_update();

        D3D_RESOURCE_DIMENSION type;
        pSurface->GetType(&type);
        if (D3D_RESOURCE_DIMENSION_TEXTURE2D == type)
        {
            D3D_SHADER_RESOURCE_VIEW_DESC ViewDesc;

            if (desc.MiscFlags & D3D_RESOURCE_MISC_TEXTURECUBE)
            {
                ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
                ViewDesc.TextureCube.MostDetailedMip = 0;
                ViewDesc.TextureCube.MipLevels = desc.MipLevels;
            }
            else
            {
                if (desc.SampleDesc.Count <= 1)
                {
                    ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                    ViewDesc.Texture2D.MostDetailedMip = 0;
                    ViewDesc.Texture2D.MipLevels = desc.MipLevels;
                }
                else
                {
                    ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DMS;
                    ViewDesc.Texture2D.MostDetailedMip = 0;
                    ViewDesc.Texture2D.MipLevels = desc.MipLevels;
                }
            }

            ViewDesc.Format = DXGI_FORMAT_UNKNOWN;

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

            // this would be supported by DX10.1 but is not needed for stalker // XXX: why?
            // if( ViewDesc.Format != DXGI_FORMAT_R24_UNORM_X8_TYPELESS )
            if ((desc.SampleDesc.Count <= 1) || (ViewDesc.Format != DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
                CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, &ViewDesc, &m_pSRView));
            else
                m_pSRView = 0;
        }
        else
            CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, NULL, &m_pSRView));
    }
}

ID3DBaseTexture* CTexture::surface_get()
{
    if (flags.bLoadedAsStaging)
        ProcessStaging();

    if (pSurface)
        pSurface->AddRef();
    return pSurface;
}

void CTexture::PostLoad()
{
    if (pTheora)
        bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_theora);
    else if (pAVI)
        bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_avi);
    else if (!seqDATA.empty())
        bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_seq);
    else
        bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_normal);
}

void CTexture::apply_load(u32 dwStage)
{
    if (!flags.bLoaded)
        Load();
    else
        PostLoad();
    bind(dwStage);
};

void CTexture::ProcessStaging()
{
    VERIFY(pSurface);
    VERIFY(flags.bLoadedAsStaging);

    ID3DBaseTexture* pTargetSurface = 0;

    D3D_RESOURCE_DIMENSION type;
    pSurface->GetType(&type);

    switch (type)
    {
    case D3D_RESOURCE_DIMENSION_TEXTURE2D:
    {
        ID3DTexture2D* T = (ID3DTexture2D*)pSurface;
        D3D_TEXTURE2D_DESC TexDesc;
        T->GetDesc(&TexDesc);
        TexDesc.Usage = D3D_USAGE_DEFAULT;
        TexDesc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        TexDesc.CPUAccessFlags = 0;

        T = 0;

        CHK_DX(HW.pDevice->CreateTexture2D(&TexDesc, // Texture desc
            NULL, // Initial data
            &T)); // [out] Texture

        pTargetSurface = T;
    }
    break;
    case D3D_RESOURCE_DIMENSION_TEXTURE3D:
    {
        ID3DTexture3D* T = (ID3DTexture3D*)pSurface;
        D3D_TEXTURE3D_DESC TexDesc;
        T->GetDesc(&TexDesc);
        TexDesc.Usage = D3D_USAGE_DEFAULT;
        TexDesc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        TexDesc.CPUAccessFlags = 0;

        T = 0;

        CHK_DX(HW.pDevice->CreateTexture3D(&TexDesc, // Texture desc
            NULL, // Initial data
            &T)); // [out] Texture

        pTargetSurface = T;
    }
    break;
    default: VERIFY(!"CTexture::ProcessStaging unsupported dimensions.");
    }

    HW.pContext->CopyResource(pTargetSurface, pSurface);
    /*
    for( int i=0; i<iNumSubresources; ++i)
    {
        HW.pDevice->CopySubresourceRegion(
            pTargetSurface,
            i,
            0,
            0,
            0,
            pSurface,
            i,
            0
            );
    }
    */

    flags.bLoadedAsStaging = FALSE;

    //	Check if texture was not copied _before_ it was converted.
    ULONG RefCnt = pSurface->Release();
    pSurface = 0;

    VERIFY(!RefCnt);

    surface_set(pTargetSurface);

    _RELEASE(pTargetSurface);
}

void CTexture::Apply(u32 dwStage)
{
    if (flags.bLoadedAsStaging)
        ProcessStaging();

    // if( !RImplementation.o.dx10_msaa )
    //   VERIFY( !((!pSurface)^(!m_pSRView)) );	//	Both present or both missing
    // else
    //{
    // if( ((!pSurface)^(!m_pSRView)) )
    //   return;
    //}

    if (dwStage < rstVertex) //	Pixel shader stage resources
    {
        // HW.pDevice->PSSetShaderResources(dwStage, 1, &m_pSRView);
        SRVSManager.SetPSResource(dwStage, m_pSRView);
    }
    else if (dwStage < rstGeometry) //	Vertex shader stage resources
    {
        // HW.pDevice->VSSetShaderResources(dwStage-rstVertex, 1, &m_pSRView);
        SRVSManager.SetVSResource(dwStage - rstVertex, m_pSRView);
    }
    else if (dwStage < rstHull) //	Geometry shader stage resources
    {
        // HW.pDevice->GSSetShaderResources(dwStage-rstGeometry, 1, &m_pSRView);
        SRVSManager.SetGSResource(dwStage - rstGeometry, m_pSRView);
    }
#ifdef USE_DX11
    else if (dwStage < rstDomain) //	Geometry shader stage resources
    {
        SRVSManager.SetHSResource(dwStage - rstHull, m_pSRView);
    }
    else if (dwStage < rstCompute) //	Geometry shader stage resources
    {
        SRVSManager.SetDSResource(dwStage - rstDomain, m_pSRView);
    }
    else if (dwStage < rstInvalid) //	Geometry shader stage resources
    {
        SRVSManager.SetCSResource(dwStage - rstCompute, m_pSRView);
    }
#endif
    else
        VERIFY("Invalid stage");
}

void CTexture::apply_theora(u32 dwStage)
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
        R_CHK(HW.pContext->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
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
        HW.pContext->Unmap(T2D, 0);
#else
        T2D->Unmap(0);
#endif
    }
    Apply(dwStage);
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
};
void CTexture::apply_avi(u32 dwStage)
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
        R_CHK(HW.pContext->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#else
        R_CHK(T2D->Map(0, D3D_MAP_WRITE_DISCARD, 0, &mapData));
#endif
        R_ASSERT(mapData.RowPitch == int(pAVI->m_dwWidth * 4));
        BYTE* ptr;
        pAVI->GetFrame(&ptr);
        CopyMemory(mapData.pData, ptr, pAVI->m_dwWidth * pAVI->m_dwHeight * 4);
// R_CHK	(T2D->UnlockRect(0));
#ifdef USE_DX11
        HW.pContext->Unmap(T2D, 0);
#else
        T2D->Unmap(0);
#endif
    }
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
    Apply(dwStage);
};
void CTexture::apply_seq(u32 dwStage)
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
    Apply(dwStage);
};
void CTexture::apply_normal(u32 dwStage)
{
    // CHK_DX(HW.pDevice->SetTexture(dwStage,pSurface));
    Apply(dwStage);
};

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
    if (0 != strstr(*cName, "$user$"))
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
        pTheora = new CTheoraSurface();
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
        pAVI = new CAviPlayerCustom();

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
        // pSurface = ::RImplementation.texture_load	(*cName,mem);
        pSurface = ::RImplementation.texture_load(*cName, mem, true);

        if (GetUsage() == D3D_USAGE_STAGING)
        {
            flags.bLoadedAsStaging = TRUE;
            bCreateView = false;
        }

        // Calc memory usage and preload into vid-mem
        if (pSurface)
        {
            // pSurface->SetPriority	(PRIORITY_NORMAL);
            flags.MemoryUsage = mem;
        }

        if (pSurface && bCreateView)
            CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, NULL, &m_pSRView));
    }

    PostLoad();
}

void CTexture::Unload()
{
#ifdef DEBUG
    string_path msg_buff;
    xr_sprintf(msg_buff, sizeof(msg_buff), "* Unloading texture [%s] pSurface RefCount=", cName.c_str());
#endif // DEBUG

    //.	if (flags.bLoaded)		Msg		("* Unloaded: %s",cName.c_str());

    flags.bLoaded = FALSE;
    flags.bLoadedAsStaging = FALSE;
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
        m_pSRView = 0;
    }

#ifdef DEBUG
    _SHOW_REF(msg_buff, pSurface);
#endif // DEBUG
    _RELEASE(pSurface);
    _RELEASE(m_pSRView);

    xr_delete(pAVI);
    xr_delete(pTheora);

    bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_load);
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

void CTexture::video_Pause(BOOL state)
{
    if (pTheora)
        pTheora->Pause(state);
}

void CTexture::video_Stop()
{
    if (pTheora)
        pTheora->Stop();
}

BOOL CTexture::video_IsPlaying() { return (pTheora) ? pTheora->IsPlaying() : FALSE; }
