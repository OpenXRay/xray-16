#include "stdafx.h"
#pragma hdrstop

#include "tntQAVI.h"

CAviPlayerCustom::CAviPlayerCustom()
{
    alpha = NULL;

    m_pMovieIndex = NULL;
    m_pMovieData = NULL;
    m_aviIC = NULL;
    m_pDecompressedBuf = NULL;

    ZeroMemory(&m_biOutFormat, sizeof(BITMAPINFOHEADER));
    ZeroMemory(&m_biInFormat, sizeof(BITMAPINFOHEADER));

    m_fRate = 0.0f;
    m_fCurrentRate = 0.0f;

    m_dwFrameTotal = 0;
    m_dwFrameCurrent = 0xfffffffd; // страхуемся от 0xffffffff + 1 == 0
    m_dwFirstFrameOffset = 0;

    m_dwWidth = 0;
    m_dwHeight = 0;
}

CAviPlayerCustom::~CAviPlayerCustom()
{
    if (m_aviIC)
    {
        ICDecompressEnd(m_aviIC);
        ICClose(m_aviIC);
    }

    if (m_pDecompressedBuf)
        xr_free(m_pDecompressedBuf);

    if (m_pMovieData)
        xr_free(m_pMovieData);
    if (m_pMovieIndex)
        xr_free(m_pMovieIndex);

    xr_delete(alpha);
}

//---------------------------------
bool CAviPlayerCustom::Load(char* fname)
{
    // Check for alpha
    string_path aname;
    strconcat(sizeof(aname), aname, fname, "_alpha");
    if (FS.exist(aname))
    {
        alpha = xr_new<CAviPlayerCustom>();
        alpha->Load(aname);
    }

    // Открыть через mmioOpen( ) AVI файл
    HMMIO hmmioFile = mmioOpen(fname, NULL, MMIO_READ /*MMIO_EXCLUSIVE*/);
    if (hmmioFile == NULL)
    {
        return false;
    }

    // Найти чанк FOURCC('movi')

    MMCKINFO mmckinfoParent;
    ZeroMemory(&mmckinfoParent, sizeof(mmckinfoParent));
    mmckinfoParent.fccType = mmioFOURCC('A', 'V', 'I', ' ');
    MMRESULT res;
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoParent, NULL, MMIO_FINDRIFF)))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    ZeroMemory(&mmckinfoParent, sizeof(mmckinfoParent));
    mmckinfoParent.fccType = mmioFOURCC('h', 'd', 'r', 'l');
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoParent, NULL, MMIO_FINDLIST)))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }
    //-------------------------------------------------------------------
    //++strl
    ZeroMemory(&mmckinfoParent, sizeof(mmckinfoParent));
    mmckinfoParent.fccType = mmioFOURCC('s', 't', 'r', 'l');
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoParent, NULL, MMIO_FINDLIST)))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    //++strh
    ZeroMemory(&mmckinfoParent, sizeof(mmckinfoParent));
    mmckinfoParent.fccType = mmioFOURCC('s', 't', 'r', 'h');
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoParent, NULL, MMIO_FINDCHUNK)))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    AVIStreamHeaderCustom strh;
    ZeroMemory(&strh, sizeof(strh));
    if (mmckinfoParent.cksize != (DWORD)mmioRead(hmmioFile, (HPSTR)&strh, mmckinfoParent.cksize))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    AVIFileInit();
    PAVIFILE aviFile = 0;
    if (AVIERR_OK != AVIFileOpen(&aviFile, fname, OF_READ, 0))
        return false;

    AVIFILEINFO aviInfo;
    ZeroMemory(&aviInfo, sizeof(aviInfo));
    if (AVIERR_OK != AVIFileInfo(aviFile, &aviInfo, sizeof(aviInfo)))
    {
        AVIFileRelease(aviFile);
        return false;
    }

    m_dwFrameTotal = aviInfo.dwLength;
    m_fCurrentRate = (float)aviInfo.dwRate / (float)aviInfo.dwScale;

    m_dwWidth = aviInfo.dwWidth;
    m_dwHeight = aviInfo.dwHeight;

    AVIFileRelease(aviFile);

    R_ASSERT(m_dwWidth && m_dwHeight);

    m_pDecompressedBuf = (u8*)xr_malloc(m_dwWidth * m_dwHeight * 4 + 4);

    //++strf
    ZeroMemory(&mmckinfoParent, sizeof(mmckinfoParent));
    mmckinfoParent.fccType = mmioFOURCC('s', 't', 'r', 'f');
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoParent, NULL, MMIO_FINDCHUNK)))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    // получаем входной формат декомпрессора в BITMAPINFOHEADER
    if (mmckinfoParent.cksize != (DWORD)mmioRead(hmmioFile, (HPSTR)&m_biInFormat, mmckinfoParent.cksize))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    // создаем выходной формат декомпрессора (xRGB)
    m_biOutFormat.biSize = sizeof(m_biOutFormat);
    m_biOutFormat.biBitCount = 32;
    m_biOutFormat.biCompression = BI_RGB;
    m_biOutFormat.biPlanes = 1;
    m_biOutFormat.biWidth = m_dwWidth;
    m_biOutFormat.biHeight = m_dwHeight;
    m_biOutFormat.biSizeImage = m_dwWidth * m_dwHeight * 4;

    // Найти подходящий декомпрессор
    m_aviIC = ICLocate(ICTYPE_VIDEO, NULL, &m_biInFormat, &m_biOutFormat, // ICMODE_DECOMPRESS
        ICMODE_FASTDECOMPRESS);
    if (m_aviIC == 0)
    {
        return false;
    }

    // Проинитить декомпрессор
    if (ICERR_OK != ICDecompressBegin(m_aviIC, &m_biInFormat, &m_biOutFormat))
    {
        return false;
    }

    //--strf
    if (MMSYSERR_NOERROR != mmioAscend(hmmioFile, &mmckinfoParent, 0))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    //--strh
    if (MMSYSERR_NOERROR != mmioAscend(hmmioFile, &mmckinfoParent, 0))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    //--strl
    if (MMSYSERR_NOERROR != mmioAscend(hmmioFile, &mmckinfoParent, 0))
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    //-------------------------------------------------------------------
    MMCKINFO mmckinfoSubchunk;
    ZeroMemory(&mmckinfoSubchunk, sizeof(mmckinfoSubchunk));
    mmckinfoSubchunk.fccType = mmioFOURCC('m', 'o', 'v', 'i');
    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoSubchunk, NULL, MMIO_FINDLIST)) ||
        mmckinfoSubchunk.cksize <= 4)
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    mmioSeek(hmmioFile, mmckinfoSubchunk.dwDataOffset, SEEK_SET);

    // Выделить память под сжатые данные всего клипа
    m_pMovieData = (u8*)xr_malloc(mmckinfoSubchunk.cksize);
    if (m_pMovieData == NULL)
    {
        mmioClose(hmmioFile, 0);
        return false;
    }

    if (mmckinfoSubchunk.cksize != (DWORD)mmioRead(hmmioFile, (HPSTR)m_pMovieData, mmckinfoSubchunk.cksize))
    {
        xr_free(m_pMovieData);
        m_pMovieData = NULL;
        mmioClose(hmmioFile, 0);
        return false;
    }

    if (MMSYSERR_NOERROR != mmioAscend(hmmioFile, &mmckinfoSubchunk, 0))
    {
        xr_free(m_pMovieData);
        m_pMovieData = NULL;
        mmioClose(hmmioFile, 0);
        return false;
    }

    // Найти чанк FOURCC('idx1')
    ZeroMemory(&mmckinfoSubchunk, sizeof(mmckinfoSubchunk));
    mmckinfoSubchunk.fccType = mmioFOURCC('i', 'd', 'x', '1');

    if (MMSYSERR_NOERROR != (res = mmioDescend(hmmioFile, &mmckinfoSubchunk, NULL, MMIO_FINDCHUNK)) ||
        mmckinfoSubchunk.cksize <= 4)
    {
        xr_free(m_pMovieData);
        m_pMovieData = NULL;
        mmioClose(hmmioFile, 0);
        return false;
    }

    // Выделить память под индекс
    m_pMovieIndex = (AVIINDEXENTRY*)xr_malloc(mmckinfoSubchunk.cksize);
    if (m_pMovieIndex == NULL)
    {
        xr_free(m_pMovieData);
        m_pMovieData = NULL;
        mmioClose(hmmioFile, 0);
        return false;
    }

    if (mmckinfoSubchunk.cksize != (DWORD)mmioRead(hmmioFile, (HPSTR)m_pMovieIndex, mmckinfoSubchunk.cksize))
    {
        xr_free(m_pMovieIndex);
        m_pMovieIndex = NULL;
        xr_free(m_pMovieData);
        m_pMovieData = NULL;
        mmioClose(hmmioFile, 0);
        return false;
    }

    // Закрыть AVI файл через mmioClose( )
    mmioClose(hmmioFile, 0);

    if (alpha)
    {
        R_ASSERT(m_dwWidth == alpha->m_dwWidth);
        R_ASSERT(m_dwHeight == alpha->m_dwHeight);
    }

    //-----------------------------------------------------------------
    return true;
}

bool CAviPlayerCustom::DecompressFrame(u32 dwFrameNum)
{
    // получаем элемент индекса
    AVIINDEXENTRY* pCurrFrameIndex = &m_pMovieIndex[dwFrameNum];

    m_biInFormat.biSizeImage = pCurrFrameIndex->dwChunkLength;
    R_ASSERT(m_biInFormat.biSizeImage != 0);

    u32 dwFlags;
    dwFlags = (pCurrFrameIndex->dwFlags & AVIIF_KEYFRAME) ? 0 : ICDECOMPRESS_NOTKEYFRAME;
    m_biInFormat.biSizeImage = pCurrFrameIndex->dwChunkLength;
    dwFlags |= (m_biInFormat.biSizeImage) ? 0 : ICDECOMPRESS_NULLFRAME;

    if (ICERR_OK != ICDecompress(m_aviIC, dwFlags, &m_biInFormat, (m_pMovieData + pCurrFrameIndex->dwChunkOffset + 8),
                        &m_biOutFormat, m_pDecompressedBuf))
    {
        return false;
    }

    if (alpha)
    {
        // update
        u8* alpha_buf;
        alpha->GetFrame(&alpha_buf);
        u32* dst = (u32*)m_pDecompressedBuf;
        u32* src = (u32*)alpha_buf;
        u32* end = dst + u32(m_dwWidth * m_dwHeight);
        for (; dst != end; src++, dst++)
        {
            u32& d = *dst;
            u32 s = *src;
            u32 a = (color_get_R(s) + color_get_G(s) + color_get_B(s)) / 3;
            d = subst_alpha(d, a);
        }
    }

    return true;
}

/*
GetFrame

возвращает true если кадр изменился, иначе false
*/
bool CAviPlayerCustom::GetFrame(u8** pDest)
{
    R_ASSERT(pDest);

    u32 dwCurrFrame;
    dwCurrFrame = CalcFrame();

    //** debug dwCurrFrame = 112;

    // Если заданный кадр равен предидущему
    if (dwCurrFrame == m_dwFrameCurrent)
    {
        *pDest = m_pDecompressedBuf;

        return false;
    }
    else
        // Если заданный кадр это Предидущий кадр + 1
        if (dwCurrFrame == m_dwFrameCurrent + 1)
    {
        ++m_dwFrameCurrent; // dwCurrFrame == m_dwFrameCurrent + 1

        *pDest = m_pDecompressedBuf;

        DecompressFrame(m_dwFrameCurrent);
        return true;
    }
    else
    {
        // Это произвольный кадр

        if (!(m_pMovieIndex[dwCurrFrame].dwFlags & AVIIF_KEYFRAME))
        {
            // Это НЕ ключевой кадр -
            // делаем PreRoll от ближайшего предидущего ключевого кадра до Заданного-1
            PreRoll(dwCurrFrame);
        }

        m_dwFrameCurrent = dwCurrFrame;

        *pDest = m_pDecompressedBuf;

        // Декомпрессим заданный кадр
        DecompressFrame(m_dwFrameCurrent);
        return true;
    }
}

// минимум проверок на валидность переданного для преролла кадра - нужна скорость
void CAviPlayerCustom::PreRoll(u32 dwFrameNum)
{
    int i;

    AVIINDEXENTRY* pCurrFrameIndex;
    u32 res;

    // находим в массиве индексов первый предшествующий ему ключевой кадр
    // или берем кадр, корректно расжатый до этого
    for (i = (int)dwFrameNum - 1; i > 0; i--)
    {
        if (m_pMovieIndex[i].dwFlags & AVIIF_KEYFRAME)
            break;

        if ((int)m_dwFrameCurrent == i)
        {
            // нам раньше встретился расжатый перед этим кадр:
            // декомпрессим все последующие НЕключевые кадры с флагами PREROLL & NOTKEYFRAME
            for (i++; i < (int)dwFrameNum; i++)
            {
                pCurrFrameIndex = &m_pMovieIndex[i];

                u32 dwFlags;
                dwFlags = ICDECOMPRESS_PREROLL | ICDECOMPRESS_NOTKEYFRAME | ICDECOMPRESS_HURRYUP;
                m_biInFormat.biSizeImage = pCurrFrameIndex->dwChunkLength;
                dwFlags |= (m_biInFormat.biSizeImage) ? 0 : ICDECOMPRESS_NULLFRAME;

                res = ICDecompress(m_aviIC, dwFlags, &m_biInFormat,
                    (m_pMovieData + pCurrFrameIndex->dwChunkOffset + 8) /*m_pCompressedBuf*/, &m_biOutFormat,
                    m_pDecompressedBuf);
                if (ICERR_OK != res && ICERR_DONTDRAW != res)
                {
                    // проверка на ICERR_DONTDRAW введена из-за indeo 5.11

                    R_ASSERT(0);
                }

            } // for(...

            return;
        } // if( (int)m_dwFrameCurrent == i )...
    } // for( i=(int)dwFrameNum-1 ; i>0 ; i-- )...

    // получаем элемент индекса
    pCurrFrameIndex = &m_pMovieIndex[i];
    m_biInFormat.biSizeImage = pCurrFrameIndex->dwChunkLength;
    R_ASSERT(m_biInFormat.biSizeImage);

    // декомпрессим ключевой кадр с флагом ICDECOMPRESS_PREROLL
    res = ICDecompress(m_aviIC, ICDECOMPRESS_PREROLL | ICDECOMPRESS_HURRYUP, &m_biInFormat,
        m_pMovieData + pCurrFrameIndex->dwChunkOffset + 8 /*m_pCompressedBuf*/, &m_biOutFormat, m_pDecompressedBuf);
    if (ICERR_OK != res && ICERR_DONTDRAW != res)
    {
        R_ASSERT(0);
    }

    // декомпрессим все последующие НЕключевые кадры с флагами PREROLL & NOTKEYFRAME
    for (i++; i < (int)dwFrameNum; i++)
    {
        pCurrFrameIndex = &m_pMovieIndex[i];

        u32 dwFlags;
        dwFlags = ICDECOMPRESS_PREROLL | ICDECOMPRESS_NOTKEYFRAME | ICDECOMPRESS_HURRYUP;
        m_biInFormat.biSizeImage = pCurrFrameIndex->dwChunkLength;
        dwFlags |= (m_biInFormat.biSizeImage) ? 0 : ICDECOMPRESS_NULLFRAME;

        res = ICDecompress(m_aviIC, dwFlags, &m_biInFormat,
            (m_pMovieData + pCurrFrameIndex->dwChunkOffset + 8) /*m_pCompressedBuf*/, &m_biOutFormat,
            m_pDecompressedBuf);
        if (ICERR_OK != res && ICERR_DONTDRAW != res)
        {
            // проверка на ICERR_DONTDRAW введена из-за indeo 5.11

            R_ASSERT(0);
        }
    } // for(...
}

void CAviPlayerCustom::GetSize(u32* dwWidth, u32* dwHeight)
{
    if (dwWidth)
        *dwWidth = m_dwWidth;
    if (dwHeight)
        *dwHeight = m_dwHeight;
}

int CAviPlayerCustom::SetSpeed(int nPercent)
{
    int res = int(m_fCurrentRate / m_fRate * 100);

    m_fCurrentRate = m_fRate * FLOAT(nPercent / 100.0f);

    return res;
}
u32 CAviPlayerCustom::CalcFrame()
{
    if (0 == m_dwFirstFrameOffset)
        m_dwFirstFrameOffset = RDEVICE.dwTimeContinual - 1;

    return u32(floor((RDEVICE.dwTimeContinual - m_dwFirstFrameOffset) * m_fCurrentRate / 1000.0f)) % m_dwFrameTotal;
}
