/*********************************************************************NVMH2****
Path:  C:\Dev\devrel\Nv_sdk_4\Direct3D\Decompress_DXTC
File:  Image_DXTC.h

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments:
A class to load and decompress DXT textures to 32-bit raw image data format.
.RAW output files can be loaded into photoshop by specifying the resolution 
and 4 color channels of 8-bit, interleaved.

A few approaches to block decompression are in place and a simple code timing
function is called.  Output of timing test is saved to a local .txt file.

******************************************************************************/



#if !defined(AFX_IMAGE_DXTC_H__4B89D8D0_7857_11D4_9630_00A0C996DE3D__INCLUDED_)
#define AFX_IMAGE_DXTC_H__4B89D8D0_7857_11D4_9630_00A0C996DE3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <ddraw.h>



//struct TimingInfo;		// defined in Image_DXTC.cpp


enum PixFormat
{
	PF_ARGB,
	PF_DXT1,
	PF_DXT2,
	PF_DXT3,
	PF_DXT4,
	PF_DXT5,
	PF_UNKNOWN
};


class Image_DXTC  
{
	BYTE*			m_pCompBytes;		// compressed image bytes
	BYTE*			m_pDecompBytes;

	int				m_nCompSize;
	int				m_nCompLineSz;

	string256		m_strFormat;
	PixFormat		m_CompFormat;

	DDSURFACEDESC2  m_DDSD;				// read from dds file
	bool			m_bMipTexture;		// texture has mipmaps?

	int				m_nWidth;			// in pixels of uncompressed image 
	int				m_nHeight;
private:
	void			DecompressDXT1		();
	void			DecompressDXT2		();
	void			DecompressDXT3		();
	void			DecompressDXT4		();
	void			DecompressDXT5		();

	void			DecodePixelFormat	(LPSTR strPixelFormat, DDPIXELFORMAT* pddpf );
	void			AllocateDecompBytes	();
public:
					Image_DXTC			();
	virtual			~Image_DXTC			();

	bool			LoadFromFile		(LPCSTR filename);		// true if success
	void			Decompress			();

	void			SaveAsRaw			();							// save decompressed bits

	BYTE*			GetCompDataPointer	() { return( m_pCompBytes );	};
	BYTE*			GetDecompDataPointer() { return( m_pDecompBytes );	};

	int				Width				() { return( m_nWidth );		}
	int				Height				() { return( m_nHeight );		}

	bool			MipTexture			() { return( m_bMipTexture );	}
};

#endif // !defined(AFX_IMAGE_DXTC_H__4B89D8D0_7857_11D4_9630_00A0C996DE3D__INCLUDED_)
