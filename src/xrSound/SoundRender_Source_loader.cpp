#include "stdafx.h"

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

//SEEK_SET 0 File beginning
//SEEK_CUR 1 Current file pointer position
//SEEK_END 2 End-of-file
int ov_seek_func(void* datasource, s64 offset, int whence)
{
    switch (whence)
    {
    case SEEK_SET: ((IReader*)datasource)->seek((int)offset); break;
    case SEEK_CUR: ((IReader*)datasource)->advance((int)offset); break;
    case SEEK_END: ((IReader*)datasource)->seek((int)offset + ((IReader*)datasource)->length()); break;
    }
    return 0;
}
size_t ov_read_func(void* ptr, size_t size, size_t nmemb, void* datasource)
{
    IReader* F = (IReader*)datasource;
    size_t exist_block = _max(0ul, iFloor(F->elapsed() / (float)size));
    size_t read_block = std::min(exist_block, nmemb);
    F->r(ptr, read_block * size);
    return read_block;
}
int ov_close_func(void* datasource) { return 0; }
long ov_tell_func(void* datasource)
{
    const auto file = static_cast<IReader*>(datasource);
    return static_cast<long>(file->tell());
}

void CSoundRender_Source::decompress(u32 line, OggVorbis_File* ovf)
{
    VERIFY(ovf);
    // decompression of one cache-line
    u32 line_size = SoundRender->cache.get_linesize();
    u32 buf_offs = (line * line_size) / (m_wformat.wBitsPerSample / 8) / m_wformat.nChannels;
    u32 left_file = dwBytesTotal - buf_offs;
    u32 left = (u32)std::min(left_file, line_size);

    // seek
    u32 cur_pos = u32(ov_pcm_tell(ovf));
    if (cur_pos != buf_offs)
        ov_pcm_seek(ovf, buf_offs);

    // decompress
    const auto dest = SoundRender->cache.get_dataptr(CAT, line);
    if (m_wformat.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        i_decompress(ovf, static_cast<float*>(dest), left);
    else
        i_decompress(ovf, static_cast<char*>(dest), left);
}

bool CSoundRender_Source::LoadWave(pcstr pName, bool crashOnError)
{
    pname = pName;

    // Load file into memory and parse WAV-format
    OggVorbis_File ovf;
    ov_callbacks ovc = {ov_read_func, ov_seek_func, ov_close_func, ov_tell_func};
    IReader* wave = FS.r_open(pname.c_str());
    R_ASSERT3(wave && wave->length(), "Can't open wave file:", pname.c_str());
    ov_open_callbacks(wave, &ovf, nullptr, 0, ovc);

    vorbis_info* ovi = ov_info(&ovf, -1);
    // verify
    R_ASSERT3_CURE(ovi, "Invalid source info:", pName, !crashOnError,
    {
        ov_clear(&ovf);
        FS.r_close(wave);
        return false;
    });
    R_ASSERT3_CURE(ovi->rate == 44100, "Invalid source rate:", pName, !crashOnError,
    {
        ov_clear(&ovf);
        FS.r_close(wave);
        return false;
    });

#ifdef DEBUG
    if (ovi->channels == 2)
        Msg("stereo sound source [%s]", pName);
#endif

    ZeroMemory(&m_wformat, sizeof(WAVEFORMATEX));

    m_wformat.nSamplesPerSec = (ovi->rate); // 44100;
    m_wformat.nChannels = u16(ovi->channels);

    if (SoundRender->supports_float_pcm)
    {
        m_wformat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        m_wformat.wBitsPerSample = 32;
    }
    else
    {
        m_wformat.wFormatTag = WAVE_FORMAT_PCM;
        m_wformat.wBitsPerSample = 16;
    }

    m_wformat.nBlockAlign = m_wformat.wBitsPerSample / 8 * m_wformat.nChannels;
    m_wformat.nAvgBytesPerSec = m_wformat.nSamplesPerSec * m_wformat.nBlockAlign;

    s64 pcm_total = ov_pcm_total(&ovf, -1);
    dwBytesTotal = u32(pcm_total * m_wformat.nBlockAlign);
    fTimeTotal = s_f_def_source_footer + dwBytesTotal / float(m_wformat.nAvgBytesPerSec);

    vorbis_comment* ovm = ov_comment(&ovf, -1);
    if (ovm->comments)
    {
        IReader F(ovm->user_comments[0], ovm->comment_lengths[0]);
        u32 vers = F.r_u32();
        if (vers == 0x0001)
        {
            m_fMinDist = F.r_float();
            m_fMaxDist = F.r_float();
            m_fBaseVolume = 1.f;
            m_uGameType = F.r_u32();
            m_fMaxAIDist = m_fMaxDist;
        }
        else if (vers == 0x0002)
        {
            m_fMinDist = F.r_float();
            m_fMaxDist = F.r_float();
            m_fBaseVolume = F.r_float();
            m_uGameType = F.r_u32();
            m_fMaxAIDist = m_fMaxDist;
        }
        else if (vers == OGG_COMMENT_VERSION)
        {
            m_fMinDist = F.r_float();
            m_fMaxDist = F.r_float();
            m_fBaseVolume = F.r_float();
            m_uGameType = F.r_u32();
            m_fMaxAIDist = F.r_float();
        }
        else
        {
#ifndef MASTER_GOLD
            Log("! Invalid ogg-comment version, file: ", pName);
#endif
        }
    }
    else
    {
#ifndef MASTER_GOLD
        Log("! Missing ogg-comment, file: ", pName);
#endif
    }

    R_ASSERT3_CURE(m_fMaxAIDist >= 0.1f && m_fMaxDist >= 0.1f, "Invalid max distance.", pName, !crashOnError,
    {
        ov_clear(&ovf);
        FS.r_close(wave);
        return false;
    });

    ov_clear(&ovf);
    FS.r_close(wave);
    return true;
}

bool CSoundRender_Source::load(pcstr name, bool replaceWithNoSound /*= true*/, bool crashOnError /*= true*/)
{
    string_path fn, N;
    xr_strcpy(N, name);
#ifdef XR_PLATFORM_WINDOWS
    xr_strlwr(N);
#endif

    if (strext(N))
        *strext(N) = 0;

    fname = N;

    strconcat(sizeof(fn), fn, N, ".ogg");
    if (!FS.exist("$level$", fn))
        FS.update_path(fn, "$game_sounds$", fn);

    bool soundExist = FS.exist(fn);
    if (!soundExist && replaceWithNoSound)
    {
        Msg("! Can't find sound '%s'", name);
        FS.update_path(fn, "$game_sounds$", "$no_sound.ogg");
        soundExist = FS.exist(fn);
    }

    if (soundExist)
    {
        if (!LoadWave(fn, crashOnError))
            return false;
        SoundRender->cache.cat_create(CAT, dwBytesTotal);
    }

    return soundExist;
}

void CSoundRender_Source::unload()
{
    SoundRender->cache.cat_destroy(CAT);
    fTimeTotal = 0.0f;
    dwBytesTotal = 0;
}
