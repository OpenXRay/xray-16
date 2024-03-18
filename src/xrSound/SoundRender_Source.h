#pragma once

#include <mutex>

#include <vorbis/vorbisfile.h>

enum class SoundFormat
{
    Unknown,
    PCM,
    Float32,
};

struct SoundSourceInfo
{
    SoundFormat format{};
    u16         channels{};       // number of channels (i.e. mono, stereo...)
    u32         samplesPerSec{};  // sample rate
    u32         avgBytesPerSec{}; // for buffer estimation
    u16         blockAlign{};     // block size of data
    u16         bitsPerSample{};  // number of bits per sample of mono data
    u32         bytesPerBuffer{}; // target buffer size
};

class XRSOUND_API CSoundRender_Source final : public CSound_source
{
public:
    shared_str pname;
    shared_str fname;

    OggVorbis_File ovf{};
    IReader* wave{};
    int refs{};
    std::mutex read_lock;

    float fTimeTotal;
    u32 dwBytesTotal;

    SoundSourceInfo m_info{};

    float m_fBaseVolume;
    float m_fMinDist;
    float m_fMaxDist;
    float m_fMaxAIDist;
    u32 m_uGameType;

private:
    void i_decompress(char* dest, u32 size);
    void i_decompress(float* dest, u32 size);

    bool LoadWave(pcstr name, bool crashOnError);

public:
    CSoundRender_Source();
    ~CSoundRender_Source() override;

    bool load(pcstr name, bool replaceWithNoSound = true, bool crashOnError = true);
    void unload();

    void attach();
    void detach();

    void decompress(void* dest, u32 byte_offset, u32 size);

    [[nodiscard]] float length_sec() const override { return fTimeTotal; }
    [[nodiscard]] u32 game_type() const override { return m_uGameType; }
    [[nodiscard]] pcstr file_name() const override { return *fname; }
    [[nodiscard]] float base_volume() const { return m_fBaseVolume; }
    [[nodiscard]] u16 channels_num() const override { return m_info.channels; }
    [[nodiscard]] u32 bytes_total() const override { return dwBytesTotal; }
};
