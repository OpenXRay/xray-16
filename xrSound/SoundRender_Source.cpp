#include "stdafx.h"
#pragma hdrstop

#include "soundrender_core.h"
#include "soundrender_source.h"

CSoundRender_Source::CSoundRender_Source	()
{
	m_fMinDist		= 1.f;
	m_fMaxDist		= 300.f;
	m_fMaxAIDist	= 300.f;
	m_fBaseVolume	= 1.f;
	m_uGameType		= 0;
	fname			= 0;
    CAT.table		= 0;
	CAT.size		= 0;
}

CSoundRender_Source::~CSoundRender_Source	()
{
	unload			();
}

bool ov_error(int res)
{
    switch (res){
    case 0:				return false;
// info
    case OV_HOLE:		Msg("Vorbisfile encoutered missing or corrupt data in the bitstream. Recovery is normally automatic and this return code is for informational purposes only."); return true;
    case OV_EBADLINK:	Msg("The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption."); return true;
// error
    case OV_FALSE: 		Msg("Not true, or no data available"); return false;
    case OV_EREAD:		Msg("Read error while fetching compressed data for decode"); return false;
    case OV_EFAULT:		Msg("Internal inconsistency in decode state. Continuing is likely not possible."); return false;
    case OV_EIMPL:		Msg("Feature not implemented"); return false; 
    case OV_EINVAL:		Msg("Either an invalid argument, or incompletely initialized argument passed to libvorbisfile call"); return false;
    case OV_ENOTVORBIS:	Msg("The given file/data was not recognized as Ogg Vorbis data."); return false;
    case OV_EBADHEADER:	Msg("The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header."); return false;
    case OV_EVERSION:	Msg("The bitstream format revision of the given stream is not supported."); return false;
    case OV_ENOSEEK:	Msg("The given stream is not seekable"); return false;
    }
    return false;

}

void CSoundRender_Source::i_decompress_fr(OggVorbis_File* ovf, char* _dest, u32 left)
{
	// vars
//	char		eof = 0;
	int			current_section;
	long		TotalRet = 0, ret;

//.	char		*PCM;
//.	PCM = new char[left];

	// Read loop
	while (TotalRet < (long)left) 
	{
		ret = ov_read(ovf, /*PCM*/ _dest+ TotalRet, left - TotalRet, 0, 2, 1, &current_section);

		// if end of file or read limit exceeded
		if (ret == 0) break;
		else if (ret < 0) 		// Error in bitstream
		{
		//
		}
		else
		{
			TotalRet += ret;
		}
	}
//.	memcpy(_dest, PCM,TotalRet);
//.	delete [] PCM;
}


/*
void CSoundRender_Source::i_decompress_fr(OggVorbis_File* ovf, char* _dest, u32 left)
{

	float **pcm; 
    int val;
    long channels		= ov_info(ovf,-1)->channels;
    long bytespersample	= 2 / channels ;
	int					dummy;
    left				/= bytespersample;
    short* buffer  		= (short*)_dest;
	while (left){
		int samples		= ov_read_float	(ovf,&pcm,left,&dummy);
        if (samples>0){
			for(int i=0;i<channels;i++) 
			{
            	float *src			= pcm[i];
                short *dest			= ((short *)buffer)+i;

                for(int j=0;j<samples;j++) 
				{
                  	val				= iFloor(src[j]*32768.f);
                    if(val>32767)
						val			= 32767;
                    else 
					if(val<-32768)
						val			= -32768;

                    *dest			= short(val);
                    dest			+=channels;
                }
            }
            left 	-= samples;
            buffer	+= samples;
        }else{
        	if (ov_error(samples)) continue; else break;
        };
	}
}
*/