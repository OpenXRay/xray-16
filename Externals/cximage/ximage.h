/*
 * File:	ximage.h
 * Purpose:	General Purpose Image Class 
 */
/*
  --------------------------------------------------------------------------------

	COPYRIGHT NOTICE, DISCLAIMER, and LICENSE:

	CxImage version 6.0.0 02/Feb/2008

	CxImage : Copyright (C) 2001 - 2008, Davide Pizzolato

	Original CImage and CImageIterator implementation are:
	Copyright (C) 1995, Alejandro Aguilar Sierra (asierra(at)servidor(dot)unam(dot)mx)

	Covered code is provided under this license on an "as is" basis, without warranty
	of any kind, either expressed or implied, including, without limitation, warranties
	that the covered code is free of defects, merchantable, fit for a particular purpose
	or non-infringing. The entire risk as to the quality and performance of the covered
	code is with you. Should any covered code prove defective in any respect, you (not
	the initial developer or any other contributor) assume the cost of any necessary
	servicing, repair or correction. This disclaimer of warranty constitutes an essential
	part of this license. No use of any covered code is authorized hereunder except under
	this disclaimer.

	Permission is hereby granted to use, copy, modify, and distribute this
	source code, or portions hereof, for any purpose, including commercial applications,
	freely and without fee, subject to the following restrictions: 

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source distribution.

  --------------------------------------------------------------------------------

	Other information about CxImage, and the latest version, can be found at the
	CxImage home page: http://www.xdp.it/cximage/

  --------------------------------------------------------------------------------
 */
#if !defined(__CXIMAGE_H)
#define __CXIMAGE_H

#if _MSC_VER > 1000
#pragma once
#endif 

/////////////////////////////////////////////////////////////////////////////
#include "xfile.h"
#include "xiofile.h"
#include "xmemfile.h"
#include "ximadef.h"	//<vho> adjust some #define

/* see "ximacfg.h" for CxImage configuration options */

/////////////////////////////////////////////////////////////////////////////
// CxImage formats enumerator
enum ENUM_CXIMAGE_FORMATS{
CXIMAGE_FORMAT_UNKNOWN = 0,
#if CXIMAGE_SUPPORT_BMP
CXIMAGE_FORMAT_BMP = 1,
#endif
#if CXIMAGE_SUPPORT_GIF
CXIMAGE_FORMAT_GIF = 2,
#endif
#if CXIMAGE_SUPPORT_JPG
CXIMAGE_FORMAT_JPG = 3,
#endif
#if CXIMAGE_SUPPORT_PNG
CXIMAGE_FORMAT_PNG = 4,
#endif
#if CXIMAGE_SUPPORT_ICO
CXIMAGE_FORMAT_ICO = 5,
#endif
#if CXIMAGE_SUPPORT_TIF
CXIMAGE_FORMAT_TIF = 6,
#endif
#if CXIMAGE_SUPPORT_TGA
CXIMAGE_FORMAT_TGA = 7,
#endif
#if CXIMAGE_SUPPORT_PCX
CXIMAGE_FORMAT_PCX = 8,
#endif
#if CXIMAGE_SUPPORT_WBMP
CXIMAGE_FORMAT_WBMP = 9,
#endif
#if CXIMAGE_SUPPORT_WMF
CXIMAGE_FORMAT_WMF = 10,
#endif
#if CXIMAGE_SUPPORT_JP2
CXIMAGE_FORMAT_JP2 = 11,
#endif
#if CXIMAGE_SUPPORT_JPC
CXIMAGE_FORMAT_JPC = 12,
#endif
#if CXIMAGE_SUPPORT_PGX
CXIMAGE_FORMAT_PGX = 13,
#endif
#if CXIMAGE_SUPPORT_PNM
CXIMAGE_FORMAT_PNM = 14,
#endif
#if CXIMAGE_SUPPORT_RAS
CXIMAGE_FORMAT_RAS = 15,
#endif
#if CXIMAGE_SUPPORT_JBG
CXIMAGE_FORMAT_JBG = 16,
#endif
#if CXIMAGE_SUPPORT_MNG
CXIMAGE_FORMAT_MNG = 17,
#endif
#if CXIMAGE_SUPPORT_SKA
CXIMAGE_FORMAT_SKA = 18,
#endif
#if CXIMAGE_SUPPORT_RAW
CXIMAGE_FORMAT_RAW = 19,
#endif
CMAX_IMAGE_FORMATS = CXIMAGE_SUPPORT_BMP + CXIMAGE_SUPPORT_GIF + CXIMAGE_SUPPORT_JPG +
					 CXIMAGE_SUPPORT_PNG + CXIMAGE_SUPPORT_MNG + CXIMAGE_SUPPORT_ICO +
					 CXIMAGE_SUPPORT_TIF + CXIMAGE_SUPPORT_TGA + CXIMAGE_SUPPORT_PCX +
					 CXIMAGE_SUPPORT_WBMP+ CXIMAGE_SUPPORT_WMF +
					 CXIMAGE_SUPPORT_JBG + CXIMAGE_SUPPORT_JP2 + CXIMAGE_SUPPORT_JPC +
					 CXIMAGE_SUPPORT_PGX + CXIMAGE_SUPPORT_PNM + CXIMAGE_SUPPORT_RAS +
					 CXIMAGE_SUPPORT_SKA + CXIMAGE_SUPPORT_RAW + 1
};

/////////////////////////////////////////////////////////////////////////////
// CxImage class
/////////////////////////////////////////////////////////////////////////////
class DLL_EXP CxImage
{
//extensible information collector
typedef struct tagCxImageInfo {
	unsigned int	dwEffWidth;			///< unsigned int aligned scan line width
	unsigned char*	pImage;				///< THE IMAGE BITS
	CxImage* pGhost;			///< if this is a ghost, pGhost points to the body
	CxImage* pParent;			///< if this is a layer, pParent points to the body
	unsigned int	dwType;				///< original image format
	char	szLastError[256];	///< debugging
	long	nProgress;			///< monitor
	long	nEscape;			///< escape
	long	nBkgndIndex;		///< used for GIF, PNG, MNG
	RGBQUAD nBkgndColor;		///< used for RGB transparency
	float	fQuality;			///< used for JPEG, JPEG2000 (0.0f ... 100.0f)
	unsigned char	nJpegScale;			///< used for JPEG [ignacio]
	long	nFrame;				///< used for TIF, GIF, MNG : actual frame
	long	nNumFrames;			///< used for TIF, GIF, MNG : total number of frames
	unsigned int	dwFrameDelay;		///< used for GIF, MNG
	long	xDPI;				///< horizontal resolution
	long	yDPI;				///< vertical resolution
	RECT	rSelectionBox;		///< bounding rectangle
	unsigned char	nAlphaMax;			///< max opacity (fade)
	bool	bAlphaPaletteEnabled; ///< true if alpha values in the palette are enabled.
	bool	bEnabled;			///< enables the painting functions
	long	xOffset;
	long	yOffset;
	unsigned int	dwCodecOpt[CMAX_IMAGE_FORMATS];	///< for GIF, TIF : 0=def.1=unc,2=fax3,3=fax4,4=pack,5=jpg
	RGBQUAD last_c;				///< for GetNearestIndex optimization
	unsigned char	last_c_index;
	bool	last_c_isvalid;
	long	nNumLayers;
	unsigned int	dwFlags;			///< 0x??00000 = reserved, 0x00??0000 = blend mode, 0x0000???? = layer id - user flags
	unsigned char	dispmeth;
	bool	bGetAllFrames;
	bool	bLittleEndianHost;

} CXIMAGEINFO;

public:
	//public structures
struct rgb_color { unsigned char r,g,b; };

#if CXIMAGE_SUPPORT_WINDOWS
// <VATI> text placement data
// members must be initialized with the InitTextInfo(&this) function.
typedef struct tagCxTextInfo
{
#if defined (_WIN32_WCE)
	TCHAR    text[256];  ///< text for windows CE
#else
	TCHAR    text[4096]; ///< text (char -> TCHAR for UNICODE [Cesar M])
#endif
	LOGFONT  lfont;      ///< font and codepage data
    COLORREF fcolor;     ///< foreground color
    long     align;      ///< DT_CENTER, DT_RIGHT, DT_LEFT aligment for multiline text
    unsigned char     smooth;     ///< text smoothing option. Default is false.
    unsigned char     opaque;     ///< text has background or hasn't. Default is true.
						 ///< data for background (ignored if .opaque==FALSE) 
    COLORREF bcolor;     ///< background color
    float    b_opacity;  ///< opacity value for background between 0.0-1.0 Default is 0. (opaque)
    unsigned char     b_outline;  ///< outline width for background (zero: no outline)
    unsigned char     b_round;    ///< rounding radius for background rectangle. % of the height, between 0-50. Default is 10.
                         ///< (backgr. always has a frame: width = 3 pixel + 10% of height by default.)
} CXTEXTINFO;
#endif

public:
/** \addtogroup Constructors */ //@{
	CxImage(unsigned int imagetype = 0);
	CxImage(unsigned int dwWidth, unsigned int dwHeight, unsigned int wBpp, unsigned int imagetype = 0);
	CxImage(const CxImage &src, bool copypixels = true, bool copyselection = true, bool copyalpha = true);
	CxImage(const TCHAR * filename, unsigned int imagetype);	// For UNICODE support: char -> TCHAR
	CxImage(FILE * stream, unsigned int imagetype);
	CxImage(CxFile * stream, unsigned int imagetype);
	CxImage(unsigned char * buffer, unsigned int size, unsigned int imagetype);
	virtual ~CxImage() { DestroyFrames(); Destroy(); };
	CxImage& operator = (const CxImage&);
//@}

/** \addtogroup Initialization */ //@{
	void*	Create(unsigned int dwWidth, unsigned int dwHeight, unsigned int wBpp, unsigned int imagetype = 0);
	bool	Destroy();
	bool	DestroyFrames();
	void	Clear(unsigned char bval=0);
	void	Copy(const CxImage &src, bool copypixels = true, bool copyselection = true, bool copyalpha = true);
	bool	Transfer(CxImage &from, bool bTransferFrames = true);
	bool	CreateFromArray(unsigned char* pArray,unsigned int dwWidth,unsigned int dwHeight,unsigned int dwBitsperpixel, unsigned int dwBytesperline, bool bFlipImage);
	bool	CreateFromMatrix(unsigned char** ppMatrix,unsigned int dwWidth,unsigned int dwHeight,unsigned int dwBitsperpixel, unsigned int dwBytesperline, bool bFlipImage);
	void	FreeMemory(void* memblock);

	unsigned int Dump(unsigned char * dst);
	unsigned int UnDump(const unsigned char * src);
	unsigned int DumpSize();

//@}

/** \addtogroup Attributes */ //@{
	long	GetSize();
	unsigned char*	GetBits(unsigned int row = 0);
	unsigned char	GetColorType();
	void*	GetDIB() const;
	unsigned int	GetHeight() const;
	unsigned int	GetWidth() const;
	unsigned int	GetEffWidth() const;
	unsigned int	GetNumColors() const;
	unsigned short	GetBpp() const;
	unsigned int	GetType() const;
	const char*	GetLastError();
	static const TCHAR* GetVersion();
	static const float GetVersionNumber();

	unsigned int	GetFrameDelay() const;
	void	SetFrameDelay(unsigned int d);

	void	GetOffset(long *x,long *y);
	void	SetOffset(long x,long y);

	unsigned char	GetJpegQuality() const;
	void	SetJpegQuality(unsigned char q);
	float	GetJpegQualityF() const;
	void	SetJpegQualityF(float q);

	unsigned char	GetJpegScale() const;
	void	SetJpegScale(unsigned char q);

	long	GetXDPI() const;
	long	GetYDPI() const;
	void	SetXDPI(long dpi);
	void	SetYDPI(long dpi);

	unsigned int	GetClrImportant() const;
	void	SetClrImportant(unsigned int ncolors = 0);

	long	GetProgress() const;
	long	GetEscape() const;
	void	SetProgress(long p);
	void	SetEscape(long i);

	long	GetTransIndex() const;
	RGBQUAD	GetTransColor();
	void	SetTransIndex(long idx);
	void	SetTransColor(RGBQUAD rgb);
	bool	IsTransparent() const;

	unsigned int	GetCodecOption(unsigned int imagetype = 0);
	bool	SetCodecOption(unsigned int opt, unsigned int imagetype = 0);

	unsigned int	GetFlags() const;
	void	SetFlags(unsigned int flags, bool bLockReservedFlags = true);

	unsigned char	GetDisposalMethod() const;
	void	SetDisposalMethod(unsigned char dm);

	bool	SetType(unsigned int type);

	static unsigned int GetNumTypes();
	static unsigned int GetTypeIdFromName(const TCHAR* ext);
	static unsigned int GetTypeIdFromIndex(const unsigned int index);
	static unsigned int GetTypeIndexFromId(const unsigned int id);

	bool	GetRetreiveAllFrames() const;
	void	SetRetreiveAllFrames(bool flag);
	CxImage * GetFrame(long nFrame) const;

	//void*	GetUserData() const {return info.pUserData;}
	//void	SetUserData(void* pUserData) {info.pUserData = pUserData;}
//@}

/** \addtogroup Palette
 * These functions have no effects on RGB images and in this case the returned value is always 0.
 * @{ */
	bool	IsGrayScale();
	bool	IsIndexed() const;
	bool	IsSamePalette(CxImage &img, bool bCheckAlpha = true);
	unsigned int	GetPaletteSize();
	RGBQUAD* GetPalette() const;
	RGBQUAD GetPaletteColor(unsigned char idx);
	bool	GetPaletteColor(unsigned char i, unsigned char* r, unsigned char* g, unsigned char* b);
	unsigned char	GetNearestIndex(RGBQUAD c);
	void	BlendPalette(COLORREF cr,long perc);
	void	SetGrayPalette();
	void	SetPalette(unsigned int n, unsigned char *r, unsigned char *g, unsigned char *b);
	void	SetPalette(RGBQUAD* pPal,unsigned int nColors=256);
	void	SetPalette(rgb_color *rgb,unsigned int nColors=256);
	void	SetPaletteColor(unsigned char idx, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha=0);
	void	SetPaletteColor(unsigned char idx, RGBQUAD c);
	void	SetPaletteColor(unsigned char idx, COLORREF cr);
	void	SwapIndex(unsigned char idx1, unsigned char idx2);
	void	SwapRGB2BGR();
	void	SetStdPalette();
//@}

/** \addtogroup Pixel */ //@{
	bool	IsInside(long x, long y);
	bool	IsTransparent(long x,long y);
	bool	GetTransparentMask(CxImage* iDst = 0);
	RGBQUAD GetPixelColor(long x,long y, bool bGetAlpha = true);
	unsigned char	GetPixelIndex(long x,long y);
	unsigned char	GetPixelGray(long x, long y);
	void	SetPixelColor(long x,long y,RGBQUAD c, bool bSetAlpha = false);
	void	SetPixelColor(long x,long y,COLORREF cr);
	void	SetPixelIndex(long x,long y,unsigned char i);
	void	DrawLine(int StartX, int EndX, int StartY, int EndY, RGBQUAD color, bool bSetAlpha=false);
	void	DrawLine(int StartX, int EndX, int StartY, int EndY, COLORREF cr);
	void	BlendPixelColor(long x,long y,RGBQUAD c, float blend, bool bSetAlpha = false);
//@}

protected:
/** \addtogroup Protected */ //@{
	unsigned char BlindGetPixelIndex(const long x,const long y);
	RGBQUAD BlindGetPixelColor(const long x,const long y, bool bGetAlpha = true);
	void *BlindGetPixelPointer(const long x,const  long y);
	void BlindSetPixelColor(long x,long y,RGBQUAD c, bool bSetAlpha = false);
	void BlindSetPixelIndex(long x,long y,unsigned char i);
//@}

public:

#if CXIMAGE_SUPPORT_INTERPOLATION
/** \addtogroup Interpolation */ //@{
	//overflow methods:
	enum OverflowMethod {
		OM_COLOR=1,
		OM_BACKGROUND=2,
		OM_TRANSPARENT=3,
		OM_WRAP=4,
		OM_REPEAT=5,
		OM_MIRROR=6
	};
	void OverflowCoordinates(float &x, float &y, OverflowMethod const ofMethod);
	void OverflowCoordinates(long  &x, long &y, OverflowMethod const ofMethod);
	RGBQUAD GetPixelColorWithOverflow(long x, long y, OverflowMethod const ofMethod=OM_BACKGROUND, RGBQUAD* const rplColor=0);
	//interpolation methods:
	enum InterpolationMethod {
		IM_NEAREST_NEIGHBOUR=1,
		IM_BILINEAR		=2,
		IM_BSPLINE		=3,
		IM_BICUBIC		=4,
		IM_BICUBIC2		=5,
		IM_LANCZOS		=6,
		IM_BOX			=7,
		IM_HERMITE		=8,
		IM_HAMMING		=9,
		IM_SINC			=10,
		IM_BLACKMAN		=11,
		IM_BESSEL		=12,
		IM_GAUSSIAN		=13,
		IM_QUADRATIC	=14,
		IM_MITCHELL		=15,
		IM_CATROM		=16,
		IM_HANNING		=17,
		IM_POWER		=18
	};
	RGBQUAD GetPixelColorInterpolated(float x,float y, InterpolationMethod const inMethod=IM_BILINEAR, OverflowMethod const ofMethod=OM_BACKGROUND, RGBQUAD* const rplColor=0);
	RGBQUAD GetAreaColorInterpolated(float const xc, float const yc, float const w, float const h, InterpolationMethod const inMethod, OverflowMethod const ofMethod=OM_BACKGROUND, RGBQUAD* const rplColor=0);
//@}

protected:
/** \addtogroup Protected */ //@{
	void  AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa);
//@}

/** \addtogroup Kernels */ //@{
public:
	static float KernelBSpline(const float x);
	static float KernelLinear(const float t);
	static float KernelCubic(const float t);
	static float KernelGeneralizedCubic(const float t, const float a=-1);
	static float KernelLanczosSinc(const float t, const float r = 3);
	static float KernelBox(const float x);
	static float KernelHermite(const float x);
	static float KernelHamming(const float x);
	static float KernelSinc(const float x);
	static float KernelBlackman(const float x);
	static float KernelBessel_J1(const float x);
	static float KernelBessel_P1(const float x);
	static float KernelBessel_Q1(const float x);
	static float KernelBessel_Order1(float x);
	static float KernelBessel(const float x);
	static float KernelGaussian(const float x);
	static float KernelQuadratic(const float x);
	static float KernelMitchell(const float x);
	static float KernelCatrom(const float x);
	static float KernelHanning(const float x);
	static float KernelPower(const float x, const float a = 2);
//@}
#endif //CXIMAGE_SUPPORT_INTERPOLATION
	
/** \addtogroup Painting */ //@{
#if CXIMAGE_SUPPORT_WINDOWS
	long	Blt(HDC pDC, long x=0, long y=0);
	HBITMAP MakeBitmap(HDC hdc = NULL);
	HANDLE	CopyToHandle();
	bool	CreateFromHANDLE(HANDLE hMem);		//Windows objects (clipboard)
	bool	CreateFromHBITMAP(HBITMAP hbmp, HPALETTE hpal=0);	//Windows resource
	bool	CreateFromHICON(HICON hico);
	long	Draw(HDC hdc, long x=0, long y=0, long cx = -1, long cy = -1, RECT* pClipRect = 0, bool bSmooth = false);
	long	Draw(HDC hdc, const RECT& rect, RECT* pClipRect=NULL, bool bSmooth = false);
	long	Stretch(HDC hdc, long xoffset, long yoffset, long xsize, long ysize, unsigned int dwRop = SRCCOPY);
	long	Stretch(HDC hdc, const RECT& rect, unsigned int dwRop = SRCCOPY);
	long	Tile(HDC hdc, RECT *rc);
	long	Draw2(HDC hdc, long x=0, long y=0, long cx = -1, long cy = -1);
	long	Draw2(HDC hdc, const RECT& rect);
	//long	DrawString(HDC hdc, long x, long y, const char* text, RGBQUAD color, const char* font, long lSize=0, long lWeight=400, unsigned char bItalic=0, unsigned char bUnderline=0, bool bSetAlpha=false);
	long	DrawString(HDC hdc, long x, long y, const TCHAR* text, RGBQUAD color, const TCHAR* font, long lSize=0, long lWeight=400, unsigned char bItalic=0, unsigned char bUnderline=0, bool bSetAlpha=false);
	// <VATI> extensions
	long    DrawStringEx(HDC hdc, long x, long y, CXTEXTINFO *pTextType, bool bSetAlpha=false );
	void    InitTextInfo( CXTEXTINFO *txt );
#endif //CXIMAGE_SUPPORT_WINDOWS
//@}

	// file operations
#if CXIMAGE_SUPPORT_DECODE
/** \addtogroup Decode */ //@{
#ifdef WIN32
	//bool Load(const wchar_t * filename, unsigned int imagetype=0);
	bool LoadResource(HRSRC hRes, unsigned int imagetype, HMODULE hModule=NULL);
#endif
	// For UNICODE support: char -> TCHAR
	bool Load(const TCHAR* filename, unsigned int imagetype=0);
	//bool Load(const char * filename, unsigned int imagetype=0);
	bool Decode(FILE * hFile, unsigned int imagetype);
	bool Decode(CxFile * hFile, unsigned int imagetype);
	bool Decode(unsigned char * buffer, unsigned int size, unsigned int imagetype);

	bool CheckFormat(CxFile * hFile, unsigned int imagetype = 0);
	bool CheckFormat(unsigned char * buffer, unsigned int size, unsigned int imagetype = 0);
//@}
#endif //CXIMAGE_SUPPORT_DECODE

#if CXIMAGE_SUPPORT_ENCODE
protected:
/** \addtogroup Protected */ //@{
	bool EncodeSafeCheck(CxFile *hFile);
//@}

public:
/** \addtogroup Encode */ //@{
#ifdef WIN32
	//bool Save(const wchar_t * filename, unsigned int imagetype=0);
#endif
	// For UNICODE support: char -> TCHAR
	bool Save(const TCHAR* filename, unsigned int imagetype);
	//bool Save(const char * filename, unsigned int imagetype=0);
	bool Encode(FILE * hFile, unsigned int imagetype);
	bool Encode(CxFile * hFile, unsigned int imagetype);
	bool Encode(CxFile * hFile, CxImage ** pImages, int pagecount, unsigned int imagetype);
	bool Encode(FILE *hFile, CxImage ** pImages, int pagecount, unsigned int imagetype);
	bool Encode(unsigned char * &buffer, long &size, unsigned int imagetype);

	bool Encode2RGBA(CxFile *hFile, bool bFlipY = false);
	bool Encode2RGBA(unsigned char * &buffer, long &size, bool bFlipY = false);
//@}
#endif //CXIMAGE_SUPPORT_ENCODE

/** \addtogroup Attributes */ //@{
	//misc.
	bool IsValid() const;
	bool IsEnabled() const;
	void Enable(bool enable=true);

	// frame operations
	long GetNumFrames() const;
	long GetFrame() const;
	void SetFrame(long nFrame);
//@}

#if CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
/** \addtogroup BasicTransformations */ //@{
	bool GrayScale();
	bool Flip(bool bFlipSelection = false, bool bFlipAlpha = true);
	bool Mirror(bool bMirrorSelection = false, bool bMirrorAlpha = true);
	bool Negative();
	bool RotateLeft(CxImage* iDst = NULL);
	bool RotateRight(CxImage* iDst = NULL);
//@}
#endif //CXIMAGE_SUPPORT_BASICTRANSFORMATIONS

#if CXIMAGE_SUPPORT_TRANSFORMATION
/** \addtogroup Transformations */ //@{
	// image operations
	bool Rotate(float angle, CxImage* iDst = NULL);
	bool Rotate2(float angle, CxImage *iDst = NULL, InterpolationMethod inMethod=IM_BILINEAR,
                OverflowMethod ofMethod=OM_BACKGROUND, RGBQUAD *replColor=0,
                bool const optimizeRightAngles=true, bool const bKeepOriginalSize=false);
	bool Rotate180(CxImage* iDst = NULL);
	bool Resample(long newx, long newy, int mode = 1, CxImage* iDst = NULL);
	bool Resample2(long newx, long newy, InterpolationMethod const inMethod=IM_BICUBIC2,
				OverflowMethod const ofMethod=OM_REPEAT, CxImage* const iDst = NULL,
				bool const disableAveraging=false);
	bool DecreaseBpp(unsigned int nbit, bool errordiffusion, RGBQUAD* ppal = 0, unsigned int clrimportant = 0);
	bool IncreaseBpp(unsigned int nbit);
	bool Dither(long method = 0);
	bool Crop(long left, long top, long right, long bottom, CxImage* iDst = NULL);
	bool Crop(const RECT& rect, CxImage* iDst = NULL);
	bool CropRotatedRectangle( long topx, long topy, long width, long height, float angle, CxImage* iDst = NULL);
	bool Skew(float xgain, float ygain, long xpivot=0, long ypivot=0, bool bEnableInterpolation = false);
	bool Expand(long left, long top, long right, long bottom, RGBQUAD canvascolor, CxImage* iDst = 0);
	bool Expand(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst = 0);
	bool Thumbnail(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst = 0);
	bool CircleTransform(int type,long rmax=0,float Koeff=1.0f);
	bool RedEyeRemove(float strength = 0.8f);
	bool QIShrink(long newx, long newy, CxImage* const iDst = NULL, bool bChangeBpp = false);

//@}
#endif //CXIMAGE_SUPPORT_TRANSFORMATION

#if CXIMAGE_SUPPORT_DSP
/** \addtogroup DSP */ //@{
	bool Contour();
	bool HistogramStretch(long method = 0, double threshold = 0);
	bool HistogramEqualize();
	bool HistogramNormalize();
	bool HistogramRoot();
	bool HistogramLog();
	long Histogram(long* red, long* green = 0, long* blue = 0, long* gray = 0, long colorspace = 0);
	bool Jitter(long radius=2);
	bool Repair(float radius = 0.25f, long niterations = 1, long colorspace = 0);
	bool Combine(CxImage* r,CxImage* g,CxImage* b,CxImage* a, long colorspace = 0);
	bool FFT2(CxImage* srcReal, CxImage* srcImag, CxImage* dstReal, CxImage* dstImag, long direction = 1, bool bForceFFT = true, bool bMagnitude = true);
	bool Noise(long level);
	bool Median(long Ksize=3);
	bool Gamma(float gamma);
	bool GammaRGB(float gammaR, float gammaG, float gammaB);
	bool ShiftRGB(long r, long g, long b);
	bool Threshold(unsigned char level);
	bool Threshold(CxImage* pThresholdMask);
	bool Threshold2(unsigned char level, bool bDirection, RGBQUAD nBkgndColor, bool bSetAlpha = false);
	bool Colorize(unsigned char hue, unsigned char sat, float blend = 1.0f);
	bool Light(long brightness, long contrast = 0);
	float Mean();
	bool Filter(long* kernel, long Ksize, long Kfactor, long Koffset);
	bool Erode(long Ksize=2);
	bool Dilate(long Ksize=2);
	bool Edge(long Ksize=2);
	void HuePalette(float correction=1);
	enum ImageOpType { OpAdd, OpAnd, OpXor, OpOr, OpMask, OpSrcCopy, OpDstCopy, OpSub, OpSrcBlend, OpScreen, OpAvg };
	void Mix(CxImage & imgsrc2, ImageOpType op, long lXOffset = 0, long lYOffset = 0, bool bMixAlpha = false);
	void MixFrom(CxImage & imagesrc2, long lXOffset, long lYOffset);
	bool UnsharpMask(float radius = 5.0f, float amount = 0.5f, int threshold = 0);
	bool Lut(unsigned char* pLut);
	bool Lut(unsigned char* pLutR, unsigned char* pLutG, unsigned char* pLutB, unsigned char* pLutA = 0);
	bool GaussianBlur(float radius = 1.0f, CxImage* iDst = 0);
	bool TextBlur(unsigned char threshold = 100, unsigned char decay = 2, unsigned char max_depth = 5, bool bBlurHorizontal = true, bool bBlurVertical = true, CxImage* iDst = 0);
	bool SelectiveBlur(float radius = 1.0f, unsigned char threshold = 25, CxImage* iDst = 0);
	bool Solarize(unsigned char level = 128, bool bLinkedChannels = true);
	bool FloodFill(const long xStart, const long yStart, const RGBQUAD cFillColor, const unsigned char tolerance = 0,
					unsigned char nOpacity = 255, const bool bSelectFilledArea = false, const unsigned char nSelectionLevel = 255);
	bool Saturate(const long saturation, const long colorspace = 1);
	bool ConvertColorSpace(const long dstColorSpace, const long srcColorSpace);
	int  OptimalThreshold(long method = 0, RECT * pBox = 0, CxImage* pContrastMask = 0);
	bool AdaptiveThreshold(long method = 0, long nBoxSize = 64, CxImage* pContrastMask = 0, long nBias = 0, float fGlobalLocalBalance = 0.5f);

//@}

protected:
/** \addtogroup Protected */ //@{
	bool IsPowerof2(long x);
	bool FFT(int dir,int m,double *x,double *y);
	bool DFT(int dir,long m,double *x1,double *y1,double *x2,double *y2);
	bool RepairChannel(CxImage *ch, float radius);
	// <nipper>
	int gen_convolve_matrix (float radius, float **cmatrix_p);
	float* gen_lookup_table (float *cmatrix, int cmatrix_length);
	void blur_line (float *ctable, float *cmatrix, int cmatrix_length, unsigned char* cur_col, unsigned char* dest_col, int y, long bytes);
	void blur_text (unsigned char threshold, unsigned char decay, unsigned char max_depth, CxImage* iSrc, CxImage* iDst, unsigned char bytes);
//@}

public:
/** \addtogroup ColorSpace */ //@{
	bool SplitRGB(CxImage* r,CxImage* g,CxImage* b);
	bool SplitYUV(CxImage* y,CxImage* u,CxImage* v);
	bool SplitHSL(CxImage* h,CxImage* s,CxImage* l);
	bool SplitYIQ(CxImage* y,CxImage* i,CxImage* q);
	bool SplitXYZ(CxImage* x,CxImage* y,CxImage* z);
	bool SplitCMYK(CxImage* c,CxImage* m,CxImage* y,CxImage* k);
	static RGBQUAD HSLtoRGB(COLORREF cHSLColor);
	static RGBQUAD RGBtoHSL(RGBQUAD lRGBColor);
	static RGBQUAD HSLtoRGB(RGBQUAD lHSLColor);
	static RGBQUAD YUVtoRGB(RGBQUAD lYUVColor);
	static RGBQUAD RGBtoYUV(RGBQUAD lRGBColor);
	static RGBQUAD YIQtoRGB(RGBQUAD lYIQColor);
	static RGBQUAD RGBtoYIQ(RGBQUAD lRGBColor);
	static RGBQUAD XYZtoRGB(RGBQUAD lXYZColor);
	static RGBQUAD RGBtoXYZ(RGBQUAD lRGBColor);
#endif //CXIMAGE_SUPPORT_DSP
	static RGBQUAD RGBtoRGBQUAD(COLORREF cr);
	static COLORREF RGBQUADtoRGB (RGBQUAD c);
//@}

#if CXIMAGE_SUPPORT_SELECTION
/** \addtogroup Selection */ //@{
	bool SelectionClear(unsigned char level = 0);
	bool SelectionCreate();
	bool SelectionDelete();
	bool SelectionInvert();
	bool SelectionMirror();
	bool SelectionFlip();
	bool SelectionAddRect(RECT r, unsigned char level = 255);
	bool SelectionAddEllipse(RECT r, unsigned char level = 255);
	bool SelectionAddPolygon(POINT *points, long npoints, unsigned char level = 255);
	bool SelectionAddColor(RGBQUAD c, unsigned char level = 255);
	bool SelectionAddPixel(long x, long y, unsigned char level = 255);
	bool SelectionCopy(CxImage &from);
	bool SelectionIsInside(long x, long y);
	bool SelectionIsValid();
	void SelectionGetBox(RECT& r);
	bool SelectionToHRGN(HRGN& region);
	bool SelectionSplit(CxImage *dest);
	unsigned char SelectionGet(const long x,const long y);
	bool SelectionSet(CxImage &from);
	void SelectionRebuildBox();
	unsigned char* SelectionGetPointer(const long x = 0,const long y = 0);
//@}

protected:
/** \addtogroup Protected */ //@{
	bool BlindSelectionIsInside(long x, long y);
	unsigned char BlindSelectionGet(const long x,const long y);
	void SelectionSet(const long x,const long y,const unsigned char level);
//@}

public:

#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
/** \addtogroup Alpha */ //@{
	void AlphaClear();
	bool AlphaCreate();
	void AlphaDelete();
	void AlphaInvert();
	bool AlphaMirror();
	bool AlphaFlip();
	bool AlphaCopy(CxImage &from);
	bool AlphaSplit(CxImage *dest);
	void AlphaStrip();
	void AlphaSet(unsigned char level);
	bool AlphaSet(CxImage &from);
	void AlphaSet(const long x,const long y,const unsigned char level);
	unsigned char AlphaGet(const long x,const long y);
	unsigned char AlphaGetMax() const;
	void AlphaSetMax(unsigned char nAlphaMax);
	bool AlphaIsValid();
	unsigned char* AlphaGetPointer(const long x = 0,const long y = 0);
	bool AlphaFromTransparency();

	void AlphaPaletteClear();
	void AlphaPaletteEnable(bool enable=true);
	bool AlphaPaletteIsEnabled();
	bool AlphaPaletteIsValid();
	bool AlphaPaletteSplit(CxImage *dest);
//@}

protected:
/** \addtogroup Protected */ //@{
	unsigned char BlindAlphaGet(const long x,const long y);
//@}
#endif //CXIMAGE_SUPPORT_ALPHA

public:
#if CXIMAGE_SUPPORT_LAYERS
/** \addtogroup Layers */ //@{
	bool LayerCreate(long position = -1);
	bool LayerDelete(long position = -1);
	void LayerDeleteAll();
	CxImage* GetLayer(long position);
	CxImage* GetParent() const;
	long GetNumLayers() const;
	long LayerDrawAll(HDC hdc, long x=0, long y=0, long cx = -1, long cy = -1, RECT* pClipRect = 0, bool bSmooth = false);
	long LayerDrawAll(HDC hdc, const RECT& rect, RECT* pClipRect=NULL, bool bSmooth = false);
//@}
#endif //CXIMAGE_SUPPORT_LAYERS

protected:
/** \addtogroup Protected */ //@{
	void Startup(unsigned int imagetype = 0);
	void CopyInfo(const CxImage &src);
	void Ghost(const CxImage *src);
	void RGBtoBGR(unsigned char *buffer, int length);
	static float HueToRGB(float n1,float n2, float hue);
	void Bitfield2RGB(unsigned char *src, unsigned int redmask, unsigned int greenmask, unsigned int bluemask, unsigned char bpp);
	static int CompareColors(const void *elem1, const void *elem2);
	short ntohs(const short word);
	long ntohl(const long dword);
	void bihtoh(BITMAPINFOHEADER* bih);

	void*				pDib; //contains the header, the palette, the pixels
    BITMAPINFOHEADER    head; //standard header
	CXIMAGEINFO			info; //extended information
	unsigned char*				pSelection;	//selected region
	unsigned char*				pAlpha; //alpha channel
	CxImage**			ppLayers; //generic layers
	CxImage**			ppFrames;
//@}
};

////////////////////////////////////////////////////////////////////////////
#endif // !defined(__CXIMAGE_H)
