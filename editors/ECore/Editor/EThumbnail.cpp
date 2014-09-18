#include "stdafx.h"
#pragma hdrstop

#include "EThumbnail.h"
#include "folderlib.h"
#include "xrImage_Resampler.h"
#pragma package(smart_init)
//------------------------------------------------------------------------------
// Custom Thumbnail
//------------------------------------------------------------------------------
ECustomThumbnail::ECustomThumbnail(LPCSTR src_name, THMType type)
{
	m_Type		= type;
    m_SrcName   = src_name;
	m_Name 		= ChangeFileExt(AnsiString(src_name),".thm");
    m_Age		= 0;
}
//------------------------------------------------------------------------------

ECustomThumbnail::~ECustomThumbnail()
{
}
/*
void DrawThumbnail(TCanvas* pCanvas, TRect& r, U32Vec& data, bool bDrawWithAlpha, int _w = THUMB_WIDTH, int _h = THUMB_HEIGHT)
{
	pCanvas->CopyMode		= cmSrcCopy;
    Graphics::TBitmap *pBitmap = xr_new<Graphics::TBitmap>();

    pBitmap->PixelFormat 	= pf32bit;
    pBitmap->Height		 	= _h;
    pBitmap->Width		 	= _w;
        
    if (bDrawWithAlpha){
    	Fcolor back;
        back.set		(bgr2rgb(pCanvas->Brush->Color));  back.mul_rgb(255.f);
        for (int y = 0; y < pBitmap->Height; y++)
        {
            u32* ptr 		= (u32*)pBitmap->ScanLine[y];
            for (int x = 0; x < pBitmap->Width; x++){
                u32 src 	= data[(_h-1-y)*_w+x];
                float a		= float(color_get_A(src))/255.f;
                float inv_a	= 1.f-a;;
                u32 r		= iFloor(float(color_get_R(src))*a+back.r*inv_a);
                u32 g		= iFloor(float(color_get_G(src))*a+back.g*inv_a);
                u32 b		= iFloor(float(color_get_B(src))*a+back.b*inv_a);
                ptr[x] 		= color_rgba(r,g,b,0);
            }
        }
    }else{
        for (int y = 0; y < pBitmap->Height; y++)
        {
            u32* ptr 		= (u32*)pBitmap->ScanLine[y];
            Memory.mem_copy	(ptr,&data[(_h-1-y)*_w],_w*4);
        }
    }
    pCanvas->StretchDraw(r,pBitmap);
    xr_delete(pBitmap);
}
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Image Thumbnail
//------------------------------------------------------------------------------
EImageThumbnail::~EImageThumbnail()
{
	m_Pixels.clear();
}

void EImageThumbnail::VFlip()
{
	R_ASSERT(!m_Pixels.empty());
	u32 line[THUMB_WIDTH];
    u32 sz_ln=sizeof(u32)*THUMB_WIDTH;
    u32 y2 = THUMB_WIDTH-1;
    for (int y=0; y<THUMB_HEIGHT/2; y++,y2--){
    	CopyMemory(line,m_Pixels.begin()+y2*THUMB_WIDTH,sz_ln);
    	CopyMemory(m_Pixels.begin()+y2*THUMB_WIDTH,m_Pixels.begin()+y*THUMB_WIDTH,sz_ln);
    	CopyMemory(m_Pixels.begin()+y*THUMB_WIDTH,line,sz_ln);
    }
}

void EImageThumbnail::CreatePixels(u32* p, u32 w, u32 h)
{
//	imf_filter	imf_box  imf_triangle  imf_bell  imf_b_spline  imf_lanczos3  imf_mitchell
	R_ASSERT(p&&(w>0)&&(h>0));
	m_Pixels.resize(THUMB_SIZE);
	imf_Process(m_Pixels.begin(),THUMB_WIDTH,THUMB_HEIGHT,p,w,h,imf_box);
}

void EImageThumbnail::Draw(HDC hdc, const Irect& r)
{
	if (Valid())
    	FHelper.DrawThumbnail(hdc,r,Pixels(),THUMB_WIDTH,THUMB_HEIGHT);
}

EImageThumbnail* CreateThumbnail(LPCSTR src_name, ECustomThumbnail::THMType type, bool bLoad)
{
    switch (type){
    case ECustomThumbnail::ETObject: 	return xr_new<EObjectThumbnail>	(src_name,bLoad);
    case ECustomThumbnail::ETTexture:	return xr_new<ETextureThumbnail>(src_name,bLoad);
    case ECustomThumbnail::ETGroup:		return xr_new<EGroupThumbnail>	(src_name,bLoad);
    default: NODEFAULT;
    }
    return 0;              
}


