// Image.cpp: implementation of the CImage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Image.h"
#include "../../../Layers/xrRender/tga.h"

void CImage::Create(u32 w, u32 h, u32* data)
{
	Create(w,h);
    CopyMemory(pData,data,w*h*sizeof(u32));
}

void CImage::Create(u32 w, u32 h)
{
	xr_free		(pData);
	dwWidth		= w;
	dwHeight	= h;
	pData		= (u32*)(xr_malloc(w*h*sizeof(u32)));
}

void CImage::SaveTGA(LPCSTR name, BOOL b24)
{
	// Save
	TGAdesc		tga;
	tga.data	= pData;
	tga.format	= b24?IMG_24B:IMG_32B;
	tga.height	= dwHeight;
	tga.width	= dwWidth;
	tga.scanlenght=dwWidth*4;

	IWriter* F	= FS.w_open(name);
    if (F){
		tga.maketga	(*F);
    	FS.w_close	(F);
    }else{
        Log			("!Can't save tga:",name);
    }
}

void CImage::Vflip()
{
	R_ASSERT(pData);
	for (u32 y=0; y<dwHeight/2; y++)
	{
		u32 y2 = dwHeight-y-1;
		for (u32 x=0; x<dwWidth; x++) {
			u32 t = GetPixel(x,y);
			PutPixel(x,y, GetPixel(x,y2));
			PutPixel(x,y2,t);
		}
	}
}
void CImage::Hflip()
{
	R_ASSERT(pData);
	for (u32 y=0; y<dwHeight; y++)
	{
		for (u32 x=0; x<dwWidth/2; x++) {
			u32 x2 = dwWidth-x-1;
			u32 t = GetPixel(x,y);
			PutPixel(x,y, GetPixel(x2,y));
			PutPixel(x2,y,t);
		}
	}
}

IC BYTE ClampColor(float a)
{
	int c = iFloor(a);
	if (c<0) c=0; else if (c>255) c=255;
    return BYTE(c);
}

void CImage::Contrast(float _fc)
{
	R_ASSERT(pData);

    // Apply contrast correction
    char *ptr       = (char *)pData;
    float fc        = _fc*2.f;
    for (u32 i=0; i<dwHeight; i++) {
		for (u32 j=0; j<dwWidth; j++) {
			BYTE *p = (BYTE *) &(ptr[i*dwWidth*4 + j*4]);
			p[0] = ClampColor(128.f + fc*(float(p[0]) - 128.f)); // red
			p[1] = ClampColor(128.f + fc*(float(p[1]) - 128.f)); // green
			p[2] = ClampColor(128.f + fc*(float(p[2]) - 128.f)); // blue
		}
    }
}
void CImage::Grayscale()
{
	R_ASSERT(pData);

	char *ptr = (char *)pData;
	for (u32 i=0; i<dwHeight; i++) {
		for (u32 j=0; j<dwWidth; j++) {
            BYTE *p = (BYTE *) &(ptr[i*dwWidth*4 + j*4]);
			// Approximate values for each component's contribution to luminance.
			// Based upon the NTSC standard described in ITU-R Recommendation BT.709.
			float grey = float(p[0]) * 0.2125f + float(p[1]) * 0.7154f + float(p[2]) * 0.0721f;
			p[0] = p[1] = p[2] = ClampColor(grey);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/*
#pragma pack(push,8)
struct _fileT {
	D3DFORMAT      fmt;
	u32                   dwFlags;
	u32                   dwWidth;
	u32                   dwHeight;
	u32                   dwMipCount;

	u32                   dwPitch;

	u32                   reserved[32-6];
};
#pragma pack(pop)

void CImage::LoadT(char *name)
{
	void*	data	= DownloadFile(name);
	_fileT*	hdr     = (_fileT*)data;
	u32*	pixels	= (u32 *) ((char *)data + sizeof(_fileT));
	dwWidth = hdr->dwWidth;
	dwHeight= hdr->dwHeight;
	bAlpha	= TUisAlphaPresents(hdr->fmt);

	pData	= (u32*)xr_malloc(dwWidth*dwHeight*4);
	// CopyMemory(pData,pixels,dwWidth*dwHeight*4);

	xr_free(data);
}
*/

//-----------------------------------------------------------------------------
//    Load an TGA file
//-----------------------------------------------------------------------------

#define UPPER_LEFT		0x20
#define LOWER_LEFT		0x00

#define TWO_INTERLEAVE	0x40
#define FOUR_INTERLEAVE 0x80

#define BASE            0
#define RUN             1
#define LITERAL         2

#ifndef RGBA_GETALPHA
#define RGBA_GETALPHA(rgb)      u32((rgb) >> 24)
#define RGBA_GETRED(rgb)        u32(((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      u32(((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       u32((rgb) & 0xff)
#endif

#pragma pack(push,1)     // Gotta pack these structures!
struct TGAHeader
{
	BYTE	idlen;
	BYTE	cmtype;
	BYTE	imgtype;

	u16		cmorg;
	u16		cmlen;
	BYTE	cmes;

	short	xorg;
	short	yorg;
	short	width;
	short	height;
	BYTE	pixsize;
	BYTE	desc;
};
#pragma pack(pop)

extern u32 *Surface_Load(char*,u32&,u32&);

void CImage::Load	(LPCSTR name)
{
	VERIFY		(!pData);
	pData		= Surface_Load((LPSTR)name,dwWidth,dwHeight);
}

bool CImage::LoadTGA(LPCSTR name)
{
	destructor<IReader>	TGA(FS.r_open(name));

	TGAHeader	hdr;
	BOOL		hflip, vflip;

	TGA().r(&hdr,sizeof(TGAHeader));

	if (!((hdr.imgtype==2)||(hdr.imgtype==10))){
    	Msg("Unsupported texture format (%s)",name);
        return false;
    }
	if (!((hdr.pixsize==24)||(hdr.pixsize==32))){
    	Msg("Texture (%s) - invalid pixsize: %d",name,hdr.pixsize);
        return false;
    }
#ifndef _EDITOR
	if (!btwIsPow2(hdr.width)){
    	Msg("Texture (%s) - invalid width: %d",name,hdr.width);
        return false;
    }
	if (!btwIsPow2(hdr.height)){
    	Msg("Texture (%s) - invalid height: %d",name,hdr.height);
        return false;
    }
#endif

	// Skip funky stuff
	if (hdr.idlen)	TGA().advance(hdr.idlen);
	if (hdr.cmlen)	TGA().advance(hdr.cmlen*((hdr.cmes+7)/8));

	hflip		= (hdr.desc & 0x10) ? TRUE : FALSE;		// Need hflip
	vflip		= (hdr.desc & 0x20) ? TRUE : FALSE;		// Need vflip

	dwWidth		= hdr.width;
	dwHeight	= hdr.height;
	bAlpha 		= (hdr.pixsize==32);

	// Alloc memory
	pData		= (u32*)xr_malloc(dwWidth*dwHeight*4);

    u32 pixel;
	u32*	ptr	= pData;
    for( int y=0; y<hdr.height; y++ ){
        u32 dwOffset = y*hdr.width;

        if( 0 == ( hdr.desc & 0x0010 ) ) dwOffset = (hdr.height-y-1)*hdr.width;
        for( int x=0; x<hdr.width; ){
            if( hdr.imgtype == 10 ){
                BYTE PacketInfo; TGA().r(&PacketInfo,1);
                u16 PacketType = u16(0x80 & PacketInfo);
                u16 PixelCount = u16(( 0x007f & PacketInfo ) + 1);
                if( PacketType ){
                    pixel = 0xffffffff;
                    if(hdr.pixsize==32) TGA().r(&pixel,4);
                    else                TGA().r(&pixel,3);
                    while( PixelCount-- ){
                    	*(ptr+dwOffset+x)=pixel;
                        x++;
                    }
                }else{
                    while( PixelCount-- ){
                        pixel = 0xffffffff;
                        if(hdr.pixsize==32) TGA().r(&pixel,4);
                        else                TGA().r(&pixel,3);
                    	*(ptr+dwOffset+x)=pixel;
                        x++;
                    }
                }
            }else{
                pixel = 0xffffffff;
                if(hdr.pixsize==32) TGA().r(&pixel,4);
                else                TGA().r(&pixel,3);
				*(ptr+dwOffset+x)	=pixel;
                x++;
            }
        }
    }
/*
	if (hdr.pixsize==24)
	{	// 24bpp
		bAlpha = FALSE;
		u32	pixel	= 0;
		u32*	ptr		= pData;
		for(int iy = 0; iy<hdr.height; ++iy) {
			for(int ix=0; ix<hdr.width; ++ix) {
				TGA.r(&pixel,3); *ptr++=pixel;
//				u32 R = RGBA_GETRED	(pixel)/2;
//				u32 G = RGBA_GETGREEN	(pixel)/2;
//				u32 B = RGBA_GETBLUE	(pixel)/2;
//				*ptr++ = D3DCOLOR_XRGB(R,G,B);
			}
		}
	}
	else
	{	// 32bpp
		bAlpha = TRUE;
		TGA.r(pData,hdr.width*hdr.height*4);
	}
*/
	if (vflip) Vflip();
	if (hflip) Hflip();

    return true;
}
