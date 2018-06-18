#ifndef __XR_STREAM_SOUND_H__
#define __XR_STREAM_SOUND_H__

#ifdef WINDOWS
#include <msacm.h>
#elif defined(LINUX)
#define DSBSTATUS_PLAYING           0x00000001
#endif

// refs
class ENGINE_API IReader;

class ENGINE_API CSoundStream : public CSound_stream_interface
{
protected:
    struct sxr_riff
    {
        u8 id[4]; // identifier string = "RIFF"
        u32 len; // remaining length after this header
        char wave_id[4]; // "WAVE"
    };

    struct sxr_hdr
    {
        u8 id[4]; // identifier, e.g. "fmt " or "data"
        u32 len; // remaining chunk length after header
    };

private:
    friend class CMusicStream;
    LPSTR fName;

    float fVolume;
    float fRealVolume;
    float fBaseVolume;

    BOOL bMustLoop;
    int iLoopCountRested;

    BOOL bNeedUpdate;
    BOOL bMustPlay;

    u32 dwStatus;
    BOOL isPause;

#if defined(WINDOWS)
    IDirectSoundBuffer* pBuffer;

    // ADPCM
    HACMSTREAM hAcmStream;
    ACMSTREAMHEADER stream;
#endif
    WAVEFORMATEX* pwfx;
    WAVEFORMATEX* psrc;
    u32 dwFMT_Size;
    u32 dwSrcBufSize;
    u32 dwTotalSize;
    unsigned char* WaveSource;
    unsigned char* WaveDest;

    u32 writepos;
    BOOL isPresentData; // признак окончания буфера
    u32 dwDecPos;
    IReader* hf;
    int DataPos;

private:
    //-----------------------------------------------------
    BOOL Decompress(unsigned char* dest);
    void AppWriteDataToBuffer(u32 dwOffset, // our own write cursor
        LPBYTE lpbSoundData, // start of our data
        u32 dwSoundBytes); // size of block to copy

    void LoadADPCM();

    void Load(LPCSTR _fName);

public:
    CSoundStream();
    ~CSoundStream();

    void Play(BOOL loop = false, int loop_cnt = 0);
    void Stop();
    void Pause();

    BOOL isPlaying(void) { return (dwStatus & DSBSTATUS_PLAYING) || bMustPlay; }
    void Commit();
    void SetVolume(float vol)
    {
        fVolume = vol;
        bNeedUpdate = true;
    }
    float GetVolume() { return fVolume; }
    void Restore();
    void Update();
    void OnMove();
};

#endif //__XR_STREAM_SOUND_H__
