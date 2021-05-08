#ifndef tntQAVIH
#define tntQAVIH

#include <math.h>

#if defined(XR_PLATFORM_WINDOWS)
#include "vfw.h"
#include "mmsystem.h"
#elif defined(XR_PLATFORM_LINUX)
#include <FreeImage.h>
#endif

// replaced with standard AVIIF_KEYFRAME
// rr #define AVIINDEX_ISKEYFRAME 0x10 // ключевой кадр

// reverse enginered AVI index v.1 format
/*struct AviIndex {

 u32 dwChunkType; // chunk type, i.e. '##dc' - DIB compressed
 u32 dwFlags; // key-frame etc.
 u32 dwOffset; // sub-chunk offset from the begining of the LIST chunk
 u32 dwLenght; // chunk lenght

 };

 typedef struct {
 FOURCC fccType;
 FOURCC fccHandler;
 u32 dwFlags;
 u32 dwPriority;
 u32 dwInitialFrames;
 u32 dwScale;
 u32 dwRate;
 u32 dwStart;
 u32 dwLength;
 u32 dwSuggestedBufferSize;
 u32 dwQuality;
 u32 dwSampleSize;
 RECT rcFrame;
 } AVIStreamHeader;
 */
typedef struct
{
    FOURCC fccType;
    FOURCC fccHandler;
    u32 dwFlags;
    u32 dwPriority;
    u32 dwInitialFrames;
    u32 dwScale;
    u32 dwRate;
    u32 dwStart;
    u32 dwLength;
    u32 dwSuggestedBufferSize;
    u32 dwQuality;
    u32 dwSampleSize;
    struct
    {
        u16 left;
        u16 top;
        u16 right;
        u16 bottom;
    };
    // RECT rcFrame; - лажа в MSDN
} AVIStreamHeaderCustom;

class ENGINE_API CAviPlayerCustom
{
protected:
    CAviPlayerCustom* alpha;

protected:
    AVIINDEXENTRY* m_pMovieIndex;
    u8* m_pMovieData;
    HIC m_aviIC;
    u8* m_pDecompressedBuf;

    BITMAPINFOHEADER m_biOutFormat;
    BITMAPINFOHEADER m_biInFormat;

    float m_fRate; // стандартная скорость, fps
    float m_fCurrentRate; // текущая скорость, fps

    u32 m_dwFrameTotal;
    u32 m_dwFrameCurrent;
    u32 m_dwFirstFrameOffset;

    u32 CalcFrame();

    bool DecompressFrame(u32 dwFrameNum);
    void PreRoll(u32 dwFrameNum);

public:
    CAviPlayerCustom();
    ~CAviPlayerCustom();

    u32 m_dwWidth, m_dwHeight;

    void GetSize(u32* dwWidth, u32* dwHeight);

    bool Load(char* fname);
    bool GetFrame(u8** pDest);

    bool NeedUpdate() { return CalcFrame() != m_dwFrameCurrent; }
    int SetSpeed(int nPercent);
};
#endif
