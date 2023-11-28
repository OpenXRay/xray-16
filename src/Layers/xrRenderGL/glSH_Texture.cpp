#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"

#ifdef XR_PLATFORM_WINDOWS // TODO
#include "xrEngine/tntQAVI.h"
#endif
#include "xrEngine/xrTheora_Surface.h"

#define PRIORITY_HIGH   12
#define PRIORITY_NORMAL 8
#define PRIORITY_LOW    4

void resptrcode_texture::create(LPCSTR _name)
{
    _set(RImplementation.Resources->_CreateTexture(_name));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTexture::CTexture()
{
    pBuffer = 0;
    pAVI = nullptr;
    pTheora = nullptr;
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

void CTexture::surface_set(BaseTextureHandle surf)
{
    if (surf)
        surf->AddRef();

    _RELEASE(pSurface);

    pSurface = surf;
}

void CTexture::PostLoad()
{
    if (pTheora) bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_theora);
    else if (pAVI) bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_avi);
    else if (!seqDATA.empty()) bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_seq);
    else bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_normal);
}

void CTexture::apply_load(CBackend& cmd_list, u32 dwStage)
{
    CHK_GL(glActiveTexture(GL_TEXTURE0 + dwStage));
    if (!flags.bLoaded) Load();
    else PostLoad();
    bind(cmd_list, dwStage);
};

void CTexture::apply_theora(CBackend& cmd_list, u32 dwStage)
{
    CHK_GL(glActiveTexture(GL_TEXTURE0 + dwStage));
    CHK_GL(glBindTexture(pSurface->type, pSurface->handle));

    if (pTheora->Update(m_play_time != 0xFFFFFFFF ? m_play_time : Device.dwTimeContinual))
    {
        u32* pBits;
        u32 _w = pTheora->Width(true);
        u32 _h = pTheora->Height(true);

        // Clear and map buffer for writing
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pBuffer);
        CHK_GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, _w * _h * 4, nullptr, GL_STREAM_DRAW)); // Invalidate buffer
        CHK_GL(pBits = (u32*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

        // Write to the buffer and copy it to the texture
        int _pos = 0;
        pTheora->DecompressFrame(pBits, 0, _pos);
        CHK_GL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
        CHK_GL(glTexSubImage2D(pSurface->type, 0, 0, 0, _w, _h, GL_BGRA, GL_UNSIGNED_BYTE, nullptr));

        // Unmap the buffer to restore normal texture functionality
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
};

void CTexture::apply_avi(CBackend& cmd_list, u32 dwStage) const
{
    CHK_GL(glActiveTexture(GL_TEXTURE0 + dwStage));
    CHK_GL(glBindTexture(pSurface->type, pSurface->handle));

#ifdef XR_PLATFORM_WINDOWS // TODO
    if (pAVI->NeedUpdate())
    {
        // AVI
        u8* ptr{};
        pAVI->GetFrame(&ptr);
        CHK_GL(glTexSubImage2D(pSurface->type, 0, 0, 0, m_width, m_height,
            GL_RGBA, GL_UNSIGNED_BYTE, ptr));
    }
#endif
};

void CTexture::apply_seq(CBackend& cmd_list, u32 dwStage)
{
    // SEQ
    u32 frame = Device.dwTimeContinual / seqMSPF; //Device.dwTimeGlobal
    u32 frame_data = seqDATA.size();
    if (flags.seqCycles)
    {
        u32 frame_id = frame % (frame_data * 2);
        if (frame_id >= frame_data) frame_id = frame_data - 1 - frame_id % frame_data;
        pSurface = seqDATA[frame_id];
    }
    else
    {
        u32 frame_id = frame % frame_data;
        pSurface = seqDATA[frame_id];
    }

    CHK_GL(glActiveTexture(GL_TEXTURE0 + dwStage));
    CHK_GL(glBindTexture(pSurface->type, pSurface->handle));
};

void CTexture::apply_normal(CBackend& cmd_list, u32 dwStage) const
{
    CHK_GL(glActiveTexture(GL_TEXTURE0 + dwStage));
    CHK_GL(glBindTexture(pSurface->type, pSurface->handle));
};

void CTexture::Preload()
{
    m_bumpmap = RImplementation.Resources->m_textures_description.GetBumpName(cName);
    m_material = RImplementation.Resources->m_textures_description.GetMaterial(cName);
}

void CTexture::Load()
{
    flags.bLoaded = true;
    desc_cache = nullptr;
    if (pSurface) return;

    flags.bUser = false;
    flags.MemoryUsage = 0;
    if (nullptr == *cName)
        return;

    // we need to check only the beginning of the string,
    // so let's use strncmp instead of strstr.
    if (0 == strncmp(cName.c_str(), "$user$", sizeof("$user$") - 1))
    {
        flags.bUser = true;
    }

    if ((0 == xr_stricmp(*cName, "$null")) || flags.bUser)
    {
        pSurface = xr_new<XR_GL_TEXTURE_BASE>();
        pSurface->type = GL_TEXTURE_2D;
        pSurface->handle = 0;
        pSurface->AddRef();
        return;
    }

    Preload();

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
            GLuint pTexture = 0;
            u32 _w = pTheora->Width(false);
            u32 _h = pTheora->Height(false);

            glGenBuffers(1, &pBuffer);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pBuffer);
            CHK_GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, flags.MemoryUsage, nullptr, GL_STREAM_DRAW));
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            pSurface = XR_GL_TEXTURE_BASE::create(GL_TEXTURE_2D);
            glBindTexture(pSurface->type, pSurface->handle);
            CHK_GL(glTexStorage2D(pSurface->type, 1, GL_RGBA8, _w, _h));

            GLenum err = glGetError();
            if (err != GL_NO_ERROR)
            {
                Msg("Invalid video stream: 0x%x", err);
                xr_delete(pTheora);
                _RELEASE(pSurface);
            }
        }
    }
    else if (FS.exist(fn, "$game_textures$", *cName, ".avi"))
    {
#ifdef XR_PLATFORM_WINDOWS // TODO
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

            // Create pixel buffer object
            glGenBuffers(1, &pBuffer);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pBuffer);
            CHK_GL(glBufferData(GL_PIXEL_UNPACK_BUFFER, flags.MemoryUsage, nullptr, GL_STREAM_DRAW));

            // Now create texture to copy PBO into
            pSurface = XR_GL_TEXTURE_BASE::create(GL_TEXTURE_2D);
            glBindTexture(pSurface->type, pSurface->handle);
            CHK_GL(glTexStorage2D(pSurface->type, 1, GL_RGBA8, pAVI->m_dwWidth, pAVI->m_dwHeight));

            if (glGetError() != GL_NO_ERROR)
            {
                FATAL("Invalid video stream");
                xr_delete(pAVI);
                _RELEASE(pSurface);
            }
        }
#endif
    }
    else if (FS.exist(fn, "$game_textures$", *cName, ".seq"))
    {
        // Sequence
        string256 buffer;
        IReader* _fs = FS.r_open(fn);

        flags.seqCycles = FALSE;
        _fs->r_string(buffer, sizeof buffer);
        if (0 == xr_stricmp(buffer, "cycled"))
        {
            flags.seqCycles = TRUE;
            _fs->r_string(buffer, sizeof buffer);
        }
        u32 fps = atoi(buffer);
        seqMSPF = 1000 / fps;

        while (!_fs->eof())
        {
            _fs->r_string(buffer, sizeof buffer);
            _Trim(buffer);
            if (buffer[0])
            {
                // Load another texture
                u32 mem = 0;
                pSurface = RImplementation.texture_load(buffer, mem);
                if (pSurface->is_valid())
                {
                    // pSurface->SetPriority	(PRIORITY_LOW);
                    seqDATA.push_back(pSurface);
                    flags.MemoryUsage += mem;
                }
            }
        }
        pSurface = nullptr;
        FS.r_close(_fs);
    }
    else
    {
        // Normal texture
        u32 mem = 0;
        pSurface = RImplementation.texture_load(*cName, mem);

        // Calc memory usage and preload into vid-mem
        if (pSurface && pSurface->is_valid())
        {
            // pSurface->SetPriority	(PRIORITY_NORMAL);
            flags.MemoryUsage = mem;
        }
    }

    PostLoad();
}

void CTexture::Unload()
{
    if (!flags.bLoaded)
    {
        return;
    }

    flags.bLoaded = FALSE;
    if (!seqDATA.empty())
    {
        for (auto& tex_handle : seqDATA)
        {
            tex_handle->Release();
        }
        seqDATA.clear();
        pSurface = nullptr; // the pointer is actually a copy of pointer to one of seqDATA elems which is freed already
    }

    _RELEASE(pSurface);
    CHK_GL(glDeleteBuffers(1, &pBuffer));

#ifdef XR_PLATFORM_WINDOWS
    xr_delete(pAVI);
#endif
    xr_delete(pTheora);

    bind = fastdelegate::FastDelegate2<CBackend&,u32>(this, &CTexture::apply_load);
}

void CTexture::desc_update()
{
    desc_cache = pSurface;
    if (pSurface->is_valid()
        && (GL_TEXTURE_2D == pSurface->type || GL_TEXTURE_2D_MULTISAMPLE == pSurface->type))
    {
        glBindTexture(pSurface->type, pSurface->handle);
        CHK_GL(glGetTexLevelParameteriv(pSurface->type, 0, GL_TEXTURE_WIDTH, &m_width));
        CHK_GL(glGetTexLevelParameteriv(pSurface->type, 0, GL_TEXTURE_HEIGHT, &m_height));
    }
}

void CTexture::video_Play(BOOL looped, u32 _time)
{
    if (pTheora) pTheora->Play(looped, _time != 0xFFFFFFFF ? (m_play_time = _time) : Device.dwTimeContinual);
}

void CTexture::video_Pause(BOOL state) const
{
    if (pTheora) pTheora->Pause(state);
}

void CTexture::video_Stop() const
{
    if (pTheora) pTheora->Stop();
}

BOOL CTexture::video_IsPlaying() const
{
    return pTheora ? pTheora->IsPlaying() : FALSE;
}
