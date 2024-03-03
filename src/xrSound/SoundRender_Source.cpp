#include "stdafx.h"

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

CSoundRender_Source::CSoundRender_Source()
{
    m_fMinDist = 1.f;
    m_fMaxDist = 300.f;
    m_fMaxAIDist = 300.f;
    m_fBaseVolume = 1.f;
    m_uGameType = 0;
}

CSoundRender_Source::~CSoundRender_Source() { unload(); }

namespace
{
bool ov_can_continue_read(long res)
{
    switch (res)
    {
    case 0:
        return false;
    // info
    case OV_HOLE:
        Msg("Vorbisfile encoutered missing or corrupt data in the bitstream. Recovery is normally automatic and this "
            "return code is for informational purposes only.");
        return true;
    case OV_EBADLINK:
        Msg("The given link exists in the Vorbis data stream, but is not decipherable due to garbage or corruption.");
        return true;
    // error
    case OV_FALSE: Msg("Not true, or no data available"); return false;
    case OV_EREAD: Msg("Read error while fetching compressed data for decode"); return false;
    case OV_EFAULT: Msg("Internal inconsistency in decode state. Continuing is likely not possible."); return false;
    case OV_EIMPL: Msg("Feature not implemented"); return false;
    case OV_EINVAL:
        Msg("Either an invalid argument, or incompletely initialized argument passed to libvorbisfile call");
        return false;
    case OV_ENOTVORBIS: Msg("The given file/data was not recognized as Ogg Vorbis data."); return false;
    case OV_EBADHEADER:
        Msg("The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header.");
        return false;
    case OV_EVERSION: Msg("The bitstream format revision of the given stream is not supported."); return false;
    case OV_ENOSEEK: Msg("The given stream is not seekable"); return false;
    }
    return false;
}
}

void CSoundRender_Source::decompress(void* dest, u32 byte_offset, u32 size)
{
    std::lock_guard guard{ read_lock };

    if (!wave)
        attach();

    // seek
    const auto sample_offset = ogg_int64_t(byte_offset / m_wformat.nBlockAlign);
    const u32 cur_pos = u32(ov_pcm_tell(&ovf));
    if (cur_pos != sample_offset)
        ov_pcm_seek(&ovf, sample_offset);

    // decompress
    if (m_wformat.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        i_decompress(static_cast<float*>(dest), size);
    else
        i_decompress(static_cast<char*>(dest), size);
}

void CSoundRender_Source::i_decompress(char* _dest, u32 size)
{
    long TotalRet = 0;

    // Read loop
    while (TotalRet < static_cast<long>(size))
    {
        const auto ret = ov_read(&ovf, _dest + TotalRet, size - TotalRet, 0, 2, 1, nullptr);
        if (ret <= 0 && !ov_can_continue_read(ret))
            break;
        TotalRet += ret;
    }
}

void CSoundRender_Source::i_decompress(float* _dest, u32 size)
{
    s32 left = s32(size / m_wformat.nBlockAlign);
    while (left)
    {
        float** pcm;
        long samples = ov_read_float(&ovf, &pcm, left, nullptr);

        if (samples <= 0 && !ov_can_continue_read(samples))
            break;

        if (samples > left)
            samples = left;

        for (long j = 0; j < samples; j++)
            for (long i = 0; i < m_wformat.nChannels; i++)
                *_dest++ = pcm[i][j];

        left -= samples;
    }
}

constexpr ov_callbacks g_ov_callbacks =
{
    // read
    [](void* ptr, size_t size, size_t nmemb, void* datasource) -> size_t
    {
        auto* file = static_cast<IReader*>(datasource);
        const size_t exist_block = _max(0ul, iFloor(file->elapsed() / (float)size));
        const size_t read_block = std::min(exist_block, nmemb);
        file->r(ptr, read_block * size);
        return read_block;
    },
    // seek
    [](void* datasource, ogg_int64_t offset, int whence) -> int
    {
        //SEEK_SET 0 File beginning
        //SEEK_CUR 1 Current file pointer position
        //SEEK_END 2 End-of-file
        switch (whence)
        {
        case SEEK_SET: ((IReader*)datasource)->seek((int)offset); break;
        case SEEK_CUR: ((IReader*)datasource)->advance((int)offset); break;
        case SEEK_END: ((IReader*)datasource)->seek((int)offset + ((IReader*)datasource)->length()); break;
        }
        return 0;
    },
    // close
    [](void* /*datasource*/) -> int
    {
        return 0;
    },
    // tell
    [](void* datasource) -> long
    {
        const auto file = static_cast<IReader*>(datasource);
        return static_cast<long>(file->tell());
    },
};

void CSoundRender_Source::attach()
{
    VERIFY(0 == wave);
    if (wave)
        return;
    wave = FS.r_open(pname.c_str());
    R_ASSERT3(wave && wave->length(), "Can't open wave file:", pname.c_str());
    ov_open_callbacks(wave, &ovf, nullptr, 0, g_ov_callbacks);
}

void CSoundRender_Source::detach()
{
    if (wave)
    {
        ov_clear(&ovf);
        FS.r_close(wave);
    }
}

bool CSoundRender_Source::LoadWave(pcstr pName, bool crashOnError)
{
    pname = pName;

    attach();

    const vorbis_info* ovi = ov_info(&ovf, -1);

    // verify
    R_ASSERT3_CURE(ovi, "Invalid source info:", pName, !crashOnError,
    {
        detach();
        return false;
    });

    ZeroMemory(&m_wformat, sizeof(WAVEFORMATEX));

    m_wformat.nSamplesPerSec = ovi->rate;
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

    const s64 pcm_total = ov_pcm_total(&ovf, -1);
    dwBytesTotal = u32(pcm_total * m_wformat.nBlockAlign);
    fTimeTotal = dwBytesTotal / float(m_wformat.nAvgBytesPerSec);

    const vorbis_comment* ovm = ov_comment(&ovf, -1);
    if (ovm->comments)
    {
        IReader F(ovm->user_comments[0], ovm->comment_lengths[0]);
        const u32 vers = F.r_u32();
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
        detach();
        return false;
    });

    detach();
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
    }

    return soundExist;
}

void CSoundRender_Source::unload()
{
    fTimeTotal = 0.0f;
    dwBytesTotal = 0;
    detach();
}
