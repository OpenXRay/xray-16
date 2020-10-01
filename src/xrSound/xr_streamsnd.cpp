#include "stdafx.h"
#include "SoundRender.h"
#include "SoundRender_Core.h"
#include "xr_streamsnd.h"

const u32 dwDestBufSize = 44 * 1024;
const u32 dsBufferSize = 88 * 1024;

CSoundStream::CSoundStream()
{
    fName = 0;
    fVolume = 1.0f;
    fRealVolume = 1.0f;
    fBaseVolume = 1.0f;

    bNeedUpdate = true;
    bMustPlay = false;
#if defined(WINDOWS)
    pBuffer = NULL;
#endif
    bMustLoop = false;
    iLoopCountRested = 0;

    dwStatus = 0;
#if defined(WINDOWS)
    hAcmStream = 0;
    ZeroMemory(&stream, sizeof(stream));
#endif
    pwfx = 0;
    psrc = 0;
    dwDecPos = 0;
    hf = 0;
    DataPos = 0;
    WaveSource = 0;
    WaveDest = 0;

    isPause = false;

    writepos = 0;

    isPresentData = false;
}

CSoundStream::~CSoundStream()
{
    Stop();
    FS.r_close(hf);

    xr_free(WaveSource);
    xr_free(WaveDest);
    xr_free(pwfx);
    xr_free(psrc);
#if defined(WINDOWS)
    _RELEASE(pBuffer);
#endif
}

void CSoundStream::Update()
{
#if defined(WINDOWS)
    if (dwStatus & DSBSTATUS_BUFFERLOST)
        pBuffer->Restore();
#endif
    if (bNeedUpdate)
    {
        fRealVolume = .5f * fRealVolume + .5f * fVolume;
#if defined(WINDOWS)
        pBuffer->SetVolume(LONG((1 - fRealVolume * psSoundVMusic * fBaseVolume) * float(DSBVOLUME_MIN)));
#endif
        bNeedUpdate = false;
    }
}

void CSoundStream::Play(BOOL loop, int cnt)
{
    VERIFY(Sound);

    if (isPause)
    {
        Pause();
        return;
    }
    if (dwStatus & DSBSTATUS_PLAYING)
        return;
    dwDecPos = 0;
    isPresentData = true;
    //----------------
#if defined(WINDOWS)
    if (hAcmStream)
    {
        CHK_DX(acmStreamClose(hAcmStream, 0));
    }
    CHK_DX(acmStreamOpen(&hAcmStream, 0, psrc, pwfx, 0, NULL, 0, 0));
    CHK_DX(acmStreamSize(hAcmStream, dwDestBufSize, u32*(&dwSrcBufSize), ACM_STREAMSIZEF_DESTINATION));
#endif
    // alloc source data buffer
    VERIFY(dwSrcBufSize);
    xr_free(WaveSource);
    WaveSource = (unsigned char*)xr_malloc(dwSrcBufSize);

    // seek to data start
    hf->seek(DataPos);
    writepos = 0;
    Decompress(WaveDest);
#if defined(WINDOWS)
    writepos = stream.cbDstLengthUsed;
#endif
    //-------
    iLoopCountRested = cnt;
    bMustLoop = loop;
    bMustPlay = true;
}

void CSoundStream::Stop()
{
    int code;
    xr_free(WaveSource);
#if defined(WINDOWS)
    if (hAcmStream)
    {
        code = acmStreamClose(hAcmStream, 0);
        VERIFY2(code == 0, "Can't close stream");
    }
    hAcmStream = 0;
    pBuffer->Stop();
    pBuffer->SetCurrentPosition(0);
#endif
    dwStatus = 0;
    isPause = false;
}

void CSoundStream::Pause()
{
    if (isPause)
    {
        bMustPlay = true;
        isPause = false;
    }
    else
    {
#if defined(WINDOWS)
        if (!(dwStatus & DSBSTATUS_PLAYING))
            return;
        pBuffer->Stop();
#endif
        isPause = true;
    }
}

#if defined(WINDOWS)
void CSoundStream::Restore() { pBuffer->Restore(); }
#else
void CSoundStream::Restore() { ; }
#endif
void CSoundStream::OnMove()
{

#if defined(WINDOWS)
	VERIFY(pBuffer);

    pBuffer->GetStatus(u32*(&dwStatus));
#endif
    if (isPause)
        return;

    u32 currpos;
    u32 delta;

    if (dwStatus & DSBSTATUS_PLAYING)
    {
        Update();
#if defined(WINDOWS)
        pBuffer->GetCurrentPosition(u32*(&currpos), 0);
#endif
        if (writepos < currpos)
            delta = currpos - writepos;
        else
            delta = dsBufferSize - (writepos - currpos);
        if (isPresentData
#if defined(WINDOWS)
        		&& (delta > stream.cbDstLengthUsed)
#endif
        )
        {
            isPresentData = Decompress(WaveDest);
#if defined(WINDOWS)
            writepos += stream.cbDstLengthUsed;
#endif
        }
        else
        {
            if (!isPresentData && (currpos < writepos))
            {
                Stop();
                if (bMustLoop && !iLoopCountRested)
                {
                    Play(bMustLoop, iLoopCountRested);
                }
                else
                {
                    if (bMustLoop)
                    {
                        if (iLoopCountRested)
                            iLoopCountRested--;
                        if (!iLoopCountRested)
                        {
                            bMustLoop = false;
                        }
                        else
                        {
                            Play(bMustLoop, iLoopCountRested);
                        }
                    }
                }
            }
        }
        if (writepos > dsBufferSize)
            writepos -= dsBufferSize;
    }
    else
    {
        if (bMustPlay)
        {
            bMustPlay = false;
            Update();
#if defined(WINDOWS)
            pBuffer->Play(0, 0, DSBPLAY_LOOPING);
#endif
            dwStatus |= DSBSTATUS_PLAYING;
        }
    }
}

void CSoundStream::Load(LPCSTR name)
{
    if (name)
    {
        xr_free(fName);
        fName = xr_strdup(name);
    }
    LoadADPCM();
    bNeedUpdate = true;
}

//--------------------------------------------------------------------------------------------------
BOOL CSoundStream::Decompress(unsigned char* dest)
{
    u32 dwSrcSize = dwSrcBufSize;
    BOOL r = true;

    VERIFY(hAcmStream);

    // check for EOF
    if (dwDecPos + dwSrcSize > dwTotalSize)
    {
        dwSrcSize = dwTotalSize - dwDecPos;
        r = false;
    }
    hf->r(WaveSource, dwSrcSize);

#if defined(WINDOWS)
    stream.cbStruct = sizeof(stream);
    stream.fdwStatus = 0;
    stream.pbSrc = WaveSource;
    stream.cbSrcLength = dwSrcSize;
    stream.pbDst = dest;
    stream.cbDstLength = dwDestBufSize;

    CHK_DX(acmStreamPrepareHeader(hAcmStream, &stream, 0));
    CHK_DX(acmStreamConvert(hAcmStream, &stream, 0));
    CHK_DX(acmStreamUnprepareHeader(hAcmStream, &stream, 0));
    dwDecPos += stream.cbSrcLengthUsed;

    AppWriteDataToBuffer(writepos, WaveDest, stream.cbDstLengthUsed);
#endif

    return r;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CSoundStream::AppWriteDataToBuffer(u32 dwOffset, // our own write cursor
    u8* lpbSoundData, // start of our data
    u32 dwSoundBytes) // size of block to copy
{
    LPVOID lpvPtr1, lpvPtr2;
    u32 dwBytes1;
    u32 dwBytes2;

    // Obtain memory address of write block. This will be in two parts
    // if the block wraps around.
#if defined(WINDOWS)
    if (DS_OK == pBuffer->Lock(dwOffset, dwSoundBytes, &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2, 0))
    {
        // Write to pointers.
        CopyMemory(lpvPtr1, lpbSoundData, dwBytes1);
        if (NULL != lpvPtr2)
            CopyMemory(lpvPtr2, lpbSoundData + dwBytes1, dwBytes2);
        // Release the data back to DSound.
        CHK_DX(pBuffer->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2));
    }
#endif
}

#define XRead(a) hf->r(&a, sizeof(a))

#if defined(WINDOWS)
//--------------------------------------------------------------------------------------------------
BOOL ADPCMCreateSoundBuffer(IDirectSound8* lpDS, IDirectSoundBuffer** pDSB, WAVEFORMATEX* fmt)
{
    DSBUFFERDESC dsBD;

    // Set up DSBUFFERDESC structure.
    ZeroMemory(&dsBD, sizeof(DSBUFFERDESC)); // Zero it out.
    dsBD.dwSize = sizeof(DSBUFFERDESC);
    dsBD.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;

    // One-second buffer.
    dsBD.dwBufferBytes = dsBufferSize;
    dsBD.lpwfxFormat = fmt;

    // Create buffer.
    if (FAILED(lpDS->CreateSoundBuffer(&dsBD, pDSB, NULL)))
    {
        pDSB = NULL;
        return FALSE;
    }
    return TRUE;
}
#endif

//--------------------------------------------------------------------------------------------------
void CSoundStream::LoadADPCM()
{
    char buf[255];
    int pos;
    sxr_riff riff;
    sxr_hdr hdr;

    string256 fn;
#if defined(WINDOWS)
    strconcat(fn, fName, ".ogg");
#elif defined(LINUX)
    strconcat(256, fn, fName, ".ogg");
#endif

    DataPos = NULL;

    hf = FS.r_open("$game_sounds$", fn);
    VERIFY(hf);
    ZeroMemory(&riff, sizeof(riff));
    XRead(riff);
    CopyMemory(buf, riff.id, 4);
    buf[4] = 0;
    CopyMemory(buf, riff.wave_id, 4);
    buf[4] = 0;

    while (!hf->eof())
    {
        XRead(hdr);
        CopyMemory(buf, hdr.id, 4);
        buf[4] = 0;
        pos = hf->tell();
        if (stricmp(buf, "fmt ") == 0)
        {
            dwFMT_Size = hdr.len;
            psrc = (LPWAVEFORMATEX)xr_malloc(dwFMT_Size);
            pwfx = (LPWAVEFORMATEX)xr_malloc(dwFMT_Size);
            hf->r(psrc, dwFMT_Size);
            CopyMemory(pwfx, psrc, dwFMT_Size);
            pwfx->wFormatTag = WAVE_FORMAT_PCM;
        }
        else
        {
            if (stricmp(buf, "data") == 0)
            {
                DataPos = hf->tell();
                dwTotalSize = hdr.len;
            }
        }
        hf->seek(hdr.len + pos);
    }

    VERIFY(DataPos);
    // dest format
#if defined(WINDOWS)
    CHK_DX(acmFormatSuggest(NULL, psrc, pwfx, dwFMT_Size, ACM_FORMATSUGGESTF_WFORMATTAG));
#endif
    // dest buffer (const size)
    WaveDest = (unsigned char*)xr_malloc(dwDestBufSize);
    // wave source -- alloc on Play
#if defined(WINDOWS)
    // DSound----------------------------------------------------------------
    ADPCMCreateSoundBuffer(SoundRender.pDevice, &pBuffer, pwfx);
#endif
}
