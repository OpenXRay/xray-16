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
    fname = nullptr;
    CAT.table = nullptr;
    CAT.size = 0;
}

CSoundRender_Source::~CSoundRender_Source() { unload(); }
bool ov_error(int res)
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

void CSoundRender_Source::i_decompress_fr(OggVorbis_File* ovf, char* _dest, u32 left)
{
    int current_section;
    long TotalRet = 0, ret;

    // Read loop
    while (TotalRet < (long)left)
    {
        ret = ov_read(ovf, _dest + TotalRet, left - TotalRet, 0, 2, 1, &current_section);
        // BUG: ov_read can return negative value indicating an error, making this loop infinite
        // if end of file or read limit exceeded
        if (ret == 0)
            break;
        else if (ret < 0) // Error in bitstream
        {
            //
        }
        else
        {
            TotalRet += ret;
        }
    }
}
