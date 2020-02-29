#ifndef tntQAVIH
#define tntQAVIH

#include <math.h>

#if defined(WINDOWS)
#include "vfw.h"
#include "mmsystem.h"
#elif defined(LINUX)
#include <FreeImage.h>
#endif

// replaced with standard AVIIF_KEYFRAME
// rr #define AVIINDEX_ISKEYFRAME 0x10 // ключевой кадр

// reverse enginered AVI index v.1 format
/*struct AviIndex {

 unsigned int dwChunkType; // chunk type, i.e. '##dc' - DIB compressed
 unsigned int dwFlags; // key-frame etc.
 unsigned int dwOffset; // sub-chunk offset from the begining of the LIST chunk
 unsigned int dwLenght; // chunk lenght

 };

 typedef struct {
 FOURCC fccType;
 FOURCC fccHandler;
 unsigned int dwFlags;
 unsigned int dwPriority;
 unsigned int dwInitialFrames;
 unsigned int dwScale;
 unsigned int dwRate;
 unsigned int dwStart;
 unsigned int dwLength;
 unsigned int dwSuggestedBufferSize;
 unsigned int dwQuality;
 unsigned int dwSampleSize;
 RECT rcFrame;
 } AVIStreamHeader;
 */
typedef struct
{
    FOURCC fccType;
    FOURCC fccHandler;
    unsigned int dwFlags;
    unsigned int dwPriority;
    unsigned int dwInitialFrames;
    unsigned int dwScale;
    unsigned int dwRate;
    unsigned int dwStart;
    unsigned int dwLength;
    unsigned int dwSuggestedBufferSize;
    unsigned int dwQuality;
    unsigned int dwSampleSize;
    struct
    {
        unsigned short left;
        unsigned short top;
        unsigned short right;
        unsigned short bottom;
    };
    // RECT rcFrame; - лажа в MSDN
} AVIStreamHeaderCustom;

class ENGINE_API CAviPlayerCustom
{
protected:
    CAviPlayerCustom* alpha;

protected:
    AVIINDEXENTRY* m_pMovieIndex;
    unsigned char* m_pMovieData;
    HIC m_aviIC;
    unsigned char* m_pDecompressedBuf;

    BITMAPINFOHEADER m_biOutFormat;
    BITMAPINFOHEADER m_biInFormat;

    float m_fRate; // стандартная скорость, fps
    float m_fCurrentRate; // текущая скорость, fps

    unsigned int m_dwFrameTotal;
    unsigned int m_dwFrameCurrent;
    u32 m_dwFirstFrameOffset;

    unsigned int CalcFrame();

    bool DecompressFrame(unsigned int dwFrameNum);
    void PreRoll(unsigned int dwFrameNum);

public:
    CAviPlayerCustom();
    ~CAviPlayerCustom();

    unsigned int m_dwWidth, m_dwHeight;

    void GetSize(unsigned int* dwWidth, unsigned int* dwHeight);

    bool Load(char* fname);
    bool GetFrame(unsigned char** pDest);

    bool NeedUpdate() { return CalcFrame() != m_dwFrameCurrent; }
    signed int SetSpeed(signed int nPercent);
};
#endif
