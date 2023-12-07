#pragma once

#include "SoundRender_Cache.h"

// refs
struct OggVorbis_File;

class XRSOUND_API CSoundRender_Source final : public CSound_source
{
public:
    shared_str pname;
    shared_str fname;
    cache_cat CAT;

    float fTimeTotal;
    u32 dwBytesTotal;

    WAVEFORMATEX m_wformat; //= SoundRender->wfm;

    float m_fBaseVolume;
    float m_fMinDist;
    float m_fMaxDist;
    float m_fMaxAIDist;
    u32 m_uGameType;

private:
    void i_decompress(OggVorbis_File* ovf, char* dest, u32 size) const;
    void i_decompress(OggVorbis_File* ovf, float* dest, u32 size) const;

    bool LoadWave(pcstr name, bool crashOnError);

public:
    CSoundRender_Source();
    ~CSoundRender_Source() override;

    bool load(pcstr name, bool replaceWithNoSound = true, bool crashOnError = true);
    void unload();
    void decompress(u32 line, OggVorbis_File* ovf);

    [[nodiscard]] float length_sec() const override { return fTimeTotal; }
    [[nodiscard]] u32 game_type() const override { return m_uGameType; }
    [[nodiscard]] pcstr file_name() const override { return *fname; }
    [[nodiscard]] float base_volume() const { return m_fBaseVolume; }
    [[nodiscard]] u16 channels_num() const override { return m_wformat.nChannels; }
    [[nodiscard]] u32 bytes_total() const override { return dwBytesTotal; }
};
