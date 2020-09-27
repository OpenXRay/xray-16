/*

	Modified by Chris Losinger for Smaller Animals Software's ImgLib/ImgDLL.
	Esp. RGB->BGR and row order switches. 9/98
*/
#include <stdio.h>
#include <io.h>

/*
	TGADefs.h - Tye and Constant declaration file
	This File defines the Types, and the Constant Values used by the 
	TGAFile Class.

	Created By: Timothy A. Bish
	Created On:	08/18/98
*/

#ifndef TGADEFSH
#define TGADEFSH

#include <windows.h>

#pragma pack(1) // Align the structure on byte boundries...

// Possible Image Types
#define TGA_NOIMAGETYPE    0	// No Image Data Included in Image
#define TGA_MAPRGBTYPE     1	// Colormapped Image Data - No Compression
#define TGA_RAWRGBTYPE     2	// Truecolor Image Data - No Compression
#define TGA_RAWMONOTYPE    3	// Monochrome Image Data - No Compression
#define TGA_MAPENCODETYPE  9	// Colormapped Image Data - Compressed RLE
#define TGA_RAWENCODETYPE  10	// Truecolor Image Data - Compressed RLE
#define TGA_MONOENCODETYPE 11	// Monochrome Image Data - Compressed RLE
// Version Macro
#define TGA_VERSIONONE 1		// Version 1 File Format
#define TGA_VERSIONTWO 2		// Version 2 File Format
// File Read Write Modes
const int GREYSC = 0;			// Image is Greyscale
const int COLOUR = 1;			// Image is Color
const int MAPPED = 2;			// Image has a Color Map
const int RLENCO = 4;			// Image is RLE Encoded

// 18 Byte Sturcture representin the basic definitions of
// the image
typedef struct _aTGAHEADER
{
	BYTE IDLength;			// 00h Size of ID Field
	BYTE ColorMapType;		// 01h Color Map Type
	BYTE ImageType;			// 02h Image Type Code
	WORD CMapStart;			// 03h Color Map Origin
	WORD CMapLength;		// 05h Color Map Length
	BYTE CMapDepth;			// 07h Color Map Depth
	WORD XOffset;			// 08h X origin of Image
	WORD YOffset;			// 0Ah Y origin of Image
	WORD Width;				// 0Ch Width of Image
	WORD Height;			// 0Eh Height of Image
	BYTE PixelDepth;		// 10h Image Pixel Size
	BYTE ImageDescriptor;	// 11h Image Description Byte
} TGAHEADER;

// The footer is 26 Bytes in length and is always at the end of a
// TGA v2.0 file.
typedef struct _aTGAFOOTER
{
	DWORD ExtensionOffset;		// Extension Area Offset
	DWORD DeveloperOffset;		// Developer Directory Offset
	CHAR Signature[18];			// TGA Signature
} TGAFOOTER;

typedef struct _aTGATAG
{
	WORD  TagNumber;		// ID Number of the Tag
	DWORD DataOffset;		// Offset Location of the Tag
	DWORD DataSize;			// Size of the Tag Data in Bytes
} TGATAG;

// The extension area is basically the second header in the TGA v2.0
// file format.
typedef struct _aTGAEXTENSION
{
	WORD  Size;					// Extension Size
	CHAR  AuthorName[41];		// Author Name
	CHAR  AuthorComment[324];	// Author Comment
	WORD  StampMonth;			// Date/Time Stamp: Month
	WORD  StampDay;				// Date/Time Stamp: Day
	WORD  StampYear;			// Date/Time Stamp: Year
	WORD  StampHour;			// Date/Time Stamp: Hour
	WORD  StampMinute;			// Date/Time Stamp: Minute
	WORD  StampSecond;			// Date/Time Stamp: Second
	CHAR  JobName[41];			// Job Name/ID
	WORD  JobHour;				// Job Time: Hours
	WORD  JobMinute;			// Job Time: Minutes
	WORD  JobSecond;			// Job Time: Seconds
	CHAR  SoftwareId[41];		// Software ID
	WORD  VersionNumber;		// Version Number of Software
	BYTE  VersionLetter;		// Version Letter of Software
	DWORD KeyColor;				// Key Color
	WORD  PixelNumerator;		// Pixel Aspect Ratio
	WORD  PixelDenominator;		// Pixel Aspect Ratio
	WORD  GammaNumerator;		// Gamma Value
	WORD  GammaDenominator;		// Gamma Value
	DWORD ColorOffset;			// Color Correction Offset
	DWORD StampOffset;			// Postage Stamp Offset
	DWORD ScanOffset;			// Scanline Table Offset
	BYTE  AttributesType;		// Attributes Type
} TGAEXTENSION;

// The Color Correction Table is an array of 2048 Bytes in length, which
// contians 256 entries used to store the values used for color remapping.
typedef struct _aTGACOLORCORRECTIONTABLE
{
	SHORT Alpha;	// Alpha Channel Seldom Used
	SHORT Red;		// Red Value of Correction
	SHORT Green;	// Green Value of Correction
	SHORT Blue;		// Green Value of Correction
} TGACOLORCORRECTIONTABLE;

#define TRIALVERSION	-1		// LIB was not initialized with a registered key

#define IMGOK			0		// no err
#define MEMERR			1		// out of mem
#define FILEOPENERR		2		// error on file open
#define FILEREADERR		3		// error on file read
#define FILEWRITEERR	4		// error on file write
#define BADPARAMERR		5		// bad user param
#define INVALIDBMPERR	6		// bad BMP file
#define BMPRLEERR		7		// we don't do compressed (RLE) BMP files
#define INVALIDGIFERR	8		// bad GIF file
#define INVALIDJPGERR	9		// bad JPG file
#define IMGDCERR		10		// error with device context
#define IMGDIBERR		11		// problem with a GetDIBits call
#define NOGIFERR		12		// GIF support disabled
#define IMGNORESOURCE	13		// resource not found
#define CALLBACKCANCEL	14		// callback returned FALSE - operation aborted
#define INVALIDPNGERR	15		// bad PNG file
#define PNGCREATEERR	16		// internal PNG lib behavior - contact smaller animals s.w.
#define IMGDLLINTERNAL	17		// misc unexpected behavior error - contact smaller animals s.w.
#define IMGFONTERR		18		// trouble creating a font object
#define INTTIFFERR		19		// misc internal TIFF error
#define INVALIDTIFFERR	20		// invalid TIFF file
#define TIFFLZWNOTSUPPORTED	21	// this will not read TIFF-LZW iamges
#define INVALIDPCXERR	22		// invalid PCX image
#define CREATEBMPERR		23		// a call to the fn CreateCompatibleBitmap failed
#define IMGNOLINES		24		// end of an image while using single-line de/compression
#define GETDIBERR			25		// error during a call to GetDIBits
#define DEVOPNOSUPPORT	26		// device does not support an operation required by this function
#define INVALIDWMF		27		// invalid windows metafile
#define DEPTHMISMATCHERR	28	// the file was not of the requested bit-depth
#define INVALIDTGAERR 35		// Invalid TGA File
#define NOTGATHUMBNAIL 36		// No TGA Thumbnail in the file 

#pragma pack()

#endif

class TGAFile
{

public:

	// parameters
	__int32  m_error;	

public:

	// operations

	TGAFile();

	BOOL		IsFileTGA(const char * fileName);
	
	LPVOID	 LoadTGA(	const char *fileName,		// Name of file
										UINT32 *width,			// Width in Pixel
										UINT32 *height);		// Height

	HGLOBAL LoadTGA8Bit(const char    *fileName,	// Name of File
							  UINT32  *width,		// Width in pixels
							  UINT32  *height,		// Height
							  RGBQUAD *pal);		// Palette of RGBQUADS

	BOOL GetTGADims(const char *fileName, 
						  UINT32 *width, 
						  UINT32 *height);

	BOOL SaveTGA32(const char * fileName,	// output path
						 BYTE *inBuf,		// RGB buffer
						 UINT32 width,		// size
						 UINT32 height);

	BOOL Save8BitTGA(const char * fileName, 	// output path
					 BYTE *colormappedbuffer,	// one BYTE per pixel colomapped image
					 UINT32 width,				// Width of image
					 UINT32 height,				// Height of image
					 __int32 colors,				// number of colors (number of RGBQUADs)
					 RGBQUAD *colormap);		// array of RGBQUADs 

	HGLOBAL LoadTGAThumbnail(const char *fileName,	// Name of file
								   UINT32 *width,	// Width in Pixel
								   UINT32 *height);	// Height

private:

	// Parameters

	char					TGA_ImageIDFeild[256];// Text in file
	BYTE					TGA_Attribute;	// Number of attribute bytes per pixel
											// i.e. 1 for T16 and 8 for T32
	UINT32 					mode;			// Mode of current Read or Write
	
	// RLE Decompression Variables
	BYTE					Red,			// Stores pixel value for
							Grn,			// RLE series of oixels
							Blu, 
							Alpha;
	UINT32 l;								// Used when 8 bit files use RLE 
	__int32 RLECount, RLEFlag;				// Indicates whether the RLE series
											// is still going or is finished

private:

	// Operations

	__int32 TGA_GetFileVersion(FILE *fp);		// Determines whether this is a V1.0
											// or V2.0 TGA File
	BOOL TGA_GetMapEntry(BYTE   *Red,		// Get the Color Values out of the 
											 BYTE   *Green,		// Color map in the TGA File
											 BYTE   *Blue,		// Return TRUE on Success
											 BYTE   *Alpha,
											 FILE   *fp,
											 UINT32 Depth);

	// version that takes a file ptr
	BOOL TGA_GetPixelValue(BYTE    *Red,	// Get and parse a single pixel value
											   BYTE    *Grn,	// from the TGA file. Handles Unencoding
											   BYTE    *Blu,	// of RLE encoded files.
											   BYTE    *Alp,	// plus Alpha (08jan00/bgw)
											   FILE    *fp,
											   UINT32  PixelDepth,
											   RGBQUAD *CMap);

	// version that takes a buffer ptr							   
	BOOL TGA_GetPixelValue(BYTE    *Red,	// Get and parse a single pixel value
											   BYTE    *Grn,	// from the TGA file. Handles Unencoding
											   BYTE    *Blu,	// of RLE encoded files.
											   BYTE    *Alp,	// plus Alpha (08jan00/bgw)
											   BYTE ** ppTGAData,
											   UINT32  PixelDepth,
											   RGBQUAD *CMap);
};

//#define MAX_IMAGEREAD_BUFFER 65535
//BYTE *	gpImageReadBuffer = NULL;
//long		glImageReadBufferSize = 0;
//BYTE *	gpImageReadBufPos = NULL;
//
//
//typedef struct tagTGAColorComponents
//    {
//    BYTE red;
//    BYTE green;
//    BYTE blue;
//    BYTE alpha;
//    }	TGAColorComponents;
    
/* TGA File REader Classs Implementation File
   This Implementation Allows the reading of TGA (Targa) Files
   into an RGB buffer. Also the class allows an RGB Buffer to be 
   written to a TGA File. There is also a function to determine 
   the dimensions of a TGA file.

  Created By: Timothy A. Bish
  Created On: 08/17/98

*/
////////////////////////////////////////////////////////////////////////////
//		No Much going on here
TGAFile::TGAFile()
	{
	m_error = IMGOK;
	}


////////////////////////////////////////////////////////////////////////////
//		GetTGADimns
//		Find dims of the image in a TGA file
//		Returns - TRUE on success
BOOL
TGAFile::GetTGADims(const char * fileName, UINT32 * width, UINT32 * height)
	{
	// for safety
	*width = 0;
	*height = 0;
	FILE * fp;
	TGAHEADER tgahd;

	// Init the file Header to all zeros
	ZeroMemory(&tgahd, sizeof(tgahd));

	// init
	m_error = IMGOK;
	fp = fopen(fileName, "rb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}

	// Get the Header
	if(fread(&tgahd, 1, sizeof(TGAHEADER), fp) != sizeof(TGAHEADER))
		{
		m_error = FILEREADERR;
		fclose(fp);
		return FALSE;
		}

	// Check fo valid data in structure
	if(tgahd.PixelDepth > 32)
		{
		// I don't do Pixel Depths Bigger than 32
		m_error = INVALIDTGAERR;
		fclose(fp);
		return NULL;
		}

	// Anything other than the standard TGA types
	// and I quit
	switch(tgahd.ImageType)
		{
		case TGA_MAPRGBTYPE:
		case TGA_RAWRGBTYPE:
		case TGA_RAWMONOTYPE:
		case TGA_MAPENCODETYPE:
		case TGA_RAWENCODETYPE:
		case TGA_MONOENCODETYPE:
			break;

		default:
			m_error = INVALIDTGAERR;
			fclose(fp);
			return NULL;
		}

	// Grab the Image dimensions	
	*width = tgahd.Width;
	*height = tgahd.Height;
	fclose(fp);
	return TRUE;
	}

/*****************************************************************************
* NAME: 
*  TGAFile::IsFileTGA
* 
* DESCRIPTION: 
*  Description goes here...
* 
*******************************************************************************/
BOOL
TGAFile::IsFileTGA(const char * fileName)
	{
	TGAHEADER		tgahd;
	FILE *			fp;

	ZeroMemory(&tgahd, sizeof(tgahd));
	fp = fopen(fileName, "rb");
	long  rc = fread(&tgahd, 1, sizeof(TGAHEADER), fp);
	fclose(fp);

	if(	(rc == sizeof(TGAHEADER)) &&							// must be big enough for a header...
			(tgahd.PixelDepth == 32) &&								// 32-bit TGAs only
			(tgahd.ImageType == TGA_RAWRGBTYPE) )			// Raw RGBA format only
		return(TRUE);

	return(FALSE);
	}

////////////////////////////////////////////////////////////////////////////
//		LoadTGA
//		load a .TGA file - 1,4,8,24,32 bit
//		allocates and returns an RGB buffer containing the image.
//		modifies width and height accordingly - NULL, 0, 0 on error
LPVOID
TGAFile::LoadTGA(const char * fileName, UINT32 * width, UINT32 * height)
	{
	LPVOID		pNew = NULL;
	BYTE *		pRGB = NULL;
	BYTE			Alpha;

	TGAHEADER tgahd;

	BYTE  TGA_Origin = 0;

	RGBQUAD CColMap[256];

	// for safety
	*width = 0;
	*height = 0;

	// init
	m_error = IMGOK;

	// Init the file Header to all zeros
	ZeroMemory(&tgahd, sizeof(tgahd));
	FILE * fp;
	fp = fopen(fileName, "rb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}

	// Read the TGA Header
	long  rc = fread(&tgahd, 1, sizeof(TGAHEADER), fp);

	if(rc != sizeof(TGAHEADER))
		{
		m_error = FILEREADERR;
		fclose(fp);
		return NULL;
		}

	// Check fo valid data in structure
	if((tgahd.PixelDepth> 32) || (tgahd.PixelDepth<8))
		{
		// I don't do Pixel Depths Bigger than 32
		m_error = INVALIDTGAERR;
		fclose(fp);
		return NULL;
		}

	// Anything other than the standard TGA types
	// and I quit
	switch(tgahd.ImageType)
		{
		case TGA_MAPRGBTYPE:
		case TGA_RAWRGBTYPE:
		case TGA_RAWMONOTYPE:
		case TGA_MAPENCODETYPE:
		case TGA_RAWENCODETYPE:
		case TGA_MONOENCODETYPE:
			break;

		default:
			m_error = INVALIDTGAERR;
			fclose(fp);
			return NULL;
		}

	// Set the number of Color Planes
	if(tgahd.ImageType == TGA_RAWMONOTYPE)
		{
		mode = GREYSC;
		}
	else
		{
		mode = COLOUR;
		}

	// Read the ID Descriptor if present
	if(tgahd.IDLength != 0)
		{
		// Read the TGA Comments
		long  rc = fread(&TGA_ImageIDFeild, 1, tgahd.IDLength, fp);

		if(rc != tgahd.IDLength)
			{
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}
		}

	// Parse the Image Descriptor
	TGA_Attribute = (BYTE)(tgahd.ImageDescriptor & 0x0f);
	TGA_Origin    = (BYTE)((tgahd.ImageDescriptor & 0x20) / 32);

	// If present read the color map
	if(tgahd.ColorMapType != 0)
		{
		// Get the color map
		for(__int32 i = 0; i < (tgahd.CMapStart + tgahd.CMapLength); i++)
			{
			TGA_GetMapEntry(&CColMap[i].rgbRed, &CColMap[i].rgbGreen, &CColMap[i].rgbBlue, &Alpha, fp, tgahd.CMapDepth);
			}

		// If the TGA file actually needs the color map
		// Set the mode to show this
		if((tgahd.ImageType != TGA_RAWRGBTYPE) && (tgahd.ImageType != TGA_RAWMONOTYPE) && (tgahd.ImageType != TGA_RAWENCODETYPE))
			mode = mode | MAPPED;
		}

	// Check Run Length Encoding
	if((tgahd.ImageType == TGA_MAPENCODETYPE) || (tgahd.ImageType == TGA_RAWENCODETYPE) || (tgahd.ImageType == TGA_MONOENCODETYPE))
		mode = mode | RLENCO;

	long lImgDataSize = (tgahd.Height * tgahd.Width) * (tgahd.PixelDepth / 8);

	// Allocate the memory buffers
	pNew = malloc(lImgDataSize);
//	pNew = (LPVOID) theApp.m_TGABuffer.GetBuffer((size_t)(tgahd.Width * tgahd.Height * 4));

	if(pNew == NULL)
		{
		m_error = MEMERR;
		fclose(fp);
		return NULL;
		}
	else
		pRGB = (BYTE *)pNew;

	// RGB from image Data
	//DWORD  destOffset = 0;

	RLECount = 0;
	RLEFlag = 0;

//	//
//	// (re-)alocate a local image buffer to read the TGA formatted
//	// data into. this avoids the huge critical-section delays in
//	// every call to getc().
//	//
//	long lBufSize = lImgDataSize + 16; // slop
//	if(lBufSize < MAX_IMAGEREAD_BUFFER)
//		lBufSize = MAX_IMAGEREAD_BUFFER;
//	if(glImageReadBufferSize < lBufSize) 
//		{
//		if(gpImageReadBuffer)
//			{
//			free(gpImageReadBuffer);
//			gpImageReadBuffer = NULL; // tidy
//			gpImageReadBufPos = NULL;
//			}
//			
//		glImageReadBufferSize = lBufSize;
//		gpImageReadBuffer = (BYTE *) malloc(glImageReadBufferSize);
//		
//		if(!gpImageReadBuffer)
//			{
//			glImageReadBufferSize = 0;
//			return NULL; // v. bad news.
//			}
//		TRACE("* (Re-)Allocated local TGA data buffer: %d bytes\n", glImageReadBufferSize);
//		}

	//
	// Read the TGA format data into the local buffer
	//
	long lTGABytesRead = fread(pRGB, 1, lImgDataSize, fp);
	if(lTGABytesRead != lImgDataSize)
		{
//		ASSERT(0);
		//free(pNew);
		return NULL;
		}
		
	// Grab the image dimensions
	*width = tgahd.Width;
	*height = tgahd.Height;

	// Clean Up
	fclose(fp);
	m_error = IMGOK;
	return pNew;

///////////////
// all out...dorks didn't realize that TGA's memory format == DIBSections!
//////////////
#if 0 
	//
	// Read the TGA format data into the local buffer
	//
	long lTGABytesRead = fread(gpImageReadBuffer, 1, lImgDataSize, fp);
	if(lTGABytesRead != lImgDataSize)
		{
		ASSERT(0);
		return NULL;
		}

	gpImageReadBufPos = gpImageReadBuffer;

	// copy DWORDs instead of bytes...
	UINT32 * pPixel = (UINT32 *) pRGB; 
	UINT32 * pReadBufPixel = (UINT32 *) gpImageReadBufPos;
	
	for(UINT32 row = 0; row < tgahd.Height; row++)
		{
		for(UINT32 col = 0; col < tgahd.Width; col++)
			{

			// Reset RLE Counters
			BYTE  Red, Grn, Blu, Alp;
			TGA_GetPixelValue(&Red, &Grn, &Blu, &Alp, &gpImageReadBufPos, tgahd.PixelDepth, CColMap);
			
			// Invert if the image origin is in Bottom left
			if(TGA_Origin != 0)
				{  // Bottom Left Origin
				destOffset = ((tgahd.Height -1) -row) * tgahd.Width * 4 + col * 4;
				*(pRGB + destOffset + 0) = Red;
				*(pRGB + destOffset + 1) = Grn;
				*(pRGB + destOffset + 2) = Blu;
				*(pRGB + destOffset + 3) = Alp;
				}
			else
				{  // Top Left Origin
				*(pRGB + destOffset + 0) = Red;
				*(pRGB + destOffset + 1) = Grn;
				*(pRGB + destOffset + 2) = Blu;
				*(pRGB + destOffset + 3) = Alp;
				destOffset += 4;
				}
	#if 0 // even faster!
	//				Red = *(gpImageReadBufPos++);
	//				Grn = *(gpImageReadBufPos++);
	//				Blu = *(gpImageReadBufPos++);
	//				Alp = *(gpImageReadBufPos++);
	//				
	//				*(pRGB + destOffset + 0) = Red;
	//				*(pRGB + destOffset + 1) = Grn;
	//				*(pRGB + destOffset + 2) = Blu;
	//				*(pRGB + destOffset + 3) = Alp;
	//				destOffset += 4;

					//	TGAColorComponents	rgbaPixel;

					*(pPixel++) = *(pReadBufPixel++);
					
	//				BYTE * p = pRGB + destOffset;
	//				*(p++) = *(gpImageReadBufPos++);
	//				*(p++) = *(gpImageReadBufPos++);
	//				*(p++) = *(gpImageReadBufPos++);
	//				*(p++) = *(gpImageReadBufPos++); 
	//				destOffset += 4;
	#endif
			} // loop col
		} // loop row

	// Grab the image dimensions
	*width = tgahd.Width;
	*height = tgahd.Height;

	// Clean Up
	fclose(fp);
	m_error = IMGOK;
	return pNew;
#endif
	}


///////////////////////////////////////////////////////////////////////////////////
//		LoadTGA8Bit
//		Loads in an 8 Bit buffer and the color palette that is
//		associated with that buffer, if the image is 8 Bit, else
//		it sets global error to DEPTHMISMATCHERR
HGLOBAL
TGAFile::LoadTGA8Bit(const char * fileName, // Name of File
UINT32 * width, // Width in pixels
UINT32 * height, // Height
RGBQUAD * pal) // Palette of RGBQUADS
	{
	HGLOBAL  hNew = NULL;
	BYTE *    pRGB = NULL;
	BYTE     Alpha;

	TGAHEADER tgahd;

	BYTE  TGA_Origin = 0;

	// for safety
	*width = 0;
	*height = 0;

	// init
	m_error = IMGOK;

	// Init the file Header to all zeros
	ZeroMemory(&tgahd, sizeof(tgahd));

	// Init the Palette to all zeros
	ZeroMemory(pal, sizeof(pal));
	FILE * fp;
	fp = fopen(fileName, "rb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}
	else
		{
		// Read the TGA Header
		long  rc = fread(&tgahd, 1, sizeof(TGAHEADER), fp);

		if(rc != sizeof(TGAHEADER))
			{
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Check fo valid data in structure
		if(tgahd.PixelDepth> 8)
			{
			// Not an 8bit Image
			m_error = DEPTHMISMATCHERR;
			fclose(fp);
			return NULL;
			}

		// Anything other than the standard TGA types
		// and I quit
		switch(tgahd.ImageType)
			{
			case TGA_MAPRGBTYPE:
			case TGA_MAPENCODETYPE:
				break;

			default:
				m_error = DEPTHMISMATCHERR;
				fclose(fp);
				return NULL;
			}

		// Set the Color Mode
		if(tgahd.ImageType == TGA_RAWMONOTYPE)
			{
			mode = GREYSC;
			}
		else
			{
			mode = COLOUR;
			}

		// Read the ID Descriptor if present
		if(tgahd.IDLength != 0)
			{
			// Read the TGA Comments
			long  rc = fread(&TGA_ImageIDFeild, 1, tgahd.IDLength, fp);

			if(rc != tgahd.IDLength)
				{
				m_error = FILEREADERR;
				fclose(fp);
				return NULL;
				}
			}

		// Parse the Image Descriptor
		TGA_Attribute = (BYTE)(tgahd.ImageDescriptor & 0x0f);
		TGA_Origin    = (BYTE)((tgahd.ImageDescriptor & 0x20) / 32);

		// If present read the color map
		if(tgahd.ColorMapType != 0)
			{
			// Get the color map
			for(__int32 i = 0; i < (tgahd.CMapStart + tgahd.CMapLength); i++)
				{
				TGA_GetMapEntry(&pal[i].rgbRed, &pal[i].rgbGreen, &pal[i].rgbBlue, &Alpha, fp, tgahd.CMapDepth);
				}
			}
		else
			{
			m_error = INVALIDTGAERR;
			fclose(fp);
			return NULL;
			}

		// Check Run Length Encoding
		if((tgahd.ImageType == TGA_MAPENCODETYPE) || (tgahd.ImageType == TGA_RAWENCODETYPE) || (tgahd.ImageType == TGA_MONOENCODETYPE))
			mode = mode | RLENCO;

		// Allocate the memory buffers
		hNew = GlobalAlloc(GHND, tgahd.Width * tgahd.Height);

		if(hNew == NULL)
			{
			m_error = MEMERR;
			fclose(fp);
			return NULL;
			}
		else
			{
			pRGB = (BYTE *)GlobalLock(hNew);

			if(pRGB == NULL)
				{
				m_error = MEMERR;
				fclose(fp);
				return NULL;
				}
			}

		// RGB from image Data
		DWORD  destOffset = 0;

		RLECount = 0;
		RLEFlag = 0;

		for(UINT32 row = 0; row < tgahd.Height; row++)
			{
			for(UINT32 col = 0; col < tgahd.Width; col++)
				{
				BYTE  Red, Grn, Blu, Alp;

				// Reset RLE Counters
				TGA_GetPixelValue(&Red, &Grn, &Blu, &Alp, fp, tgahd.PixelDepth, pal);

				// Invert if the image origin is in Bottom left
				if(TGA_Origin == 0)
					{  // Bottom Left Origin
					*(pRGB + destOffset) = Red;
					destOffset = ((tgahd.Height -1) -row) * tgahd.Width + col;
					}
				else
					{  // Top Left Origin
					*(pRGB + destOffset) = Red;
					destOffset++;
					}
				}
			}
		}

	// Grab the image dimensions
	*width = tgahd.Width;
	*height = tgahd.Height;

	// Clean Up
	GlobalUnlock(hNew);
	fclose(fp);
	m_error = IMGOK;
	return hNew;
	}


///////////////////////////////////////////////////////////////////////////////////
//		LoadTGAThumbNail
//		Checks the TGA file for the existance of a thumbnail image
//		Reads and returns it in a 24 Bit RGB Buffer if a thumbnail
//		exists. Returns NULL if there isn't one sets TGANOTHUMBNAIL
HGLOBAL
TGAFile::LoadTGAThumbnail
	(
	const char * fileName,	// Name of file
	UINT32 * width,					// Width in Pixel
	UINT32 * height					// Height
	)
	{
	HGLOBAL  hNew = NULL;

	TGAHEADER tgahd;
	TGAFOOTER tgaft;
	TGAEXTENSION tgaext;

	BYTE  stampWidth = 0, stampHeight = 0;
	long  lResult;

	// for safety
	*width = 0;
	*height = 0;

	// init
	m_error = IMGOK;

	// Init the file structs
	ZeroMemory(&tgahd, sizeof(tgahd));
	ZeroMemory(&tgaft, sizeof(tgaft));
	ZeroMemory(&tgaext, sizeof(tgaext));
	FILE * fp;
	fp = fopen(fileName, "rb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}
	else
		{
		// Read the TGA Header
		long  rc = fread(&tgahd, 1, sizeof(TGAHEADER), fp);

		if(rc != sizeof(TGAHEADER))
			{
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Check fo valid data in structure
		if((tgahd.PixelDepth> 32) || (tgahd.PixelDepth<8))
			{
			// I don't do Pixel Depths Bigger than 32
			m_error = INVALIDTGAERR;
			fclose(fp);
			return NULL;
			}

		// Anything other than the standard TGA types
		// and I quit
		switch(tgahd.ImageType)
			{
			case TGA_MAPRGBTYPE:
			case TGA_RAWRGBTYPE:
			case TGA_RAWMONOTYPE:
			case TGA_MAPENCODETYPE:
			case TGA_RAWENCODETYPE:
			case TGA_MONOENCODETYPE:
				break;

			default:
				m_error = INVALIDTGAERR;
				fclose(fp);
				return NULL;
			}

		// Set the number of Color Planes
		if(tgahd.ImageType == TGA_RAWMONOTYPE)
			{
			mode = GREYSC;
			}
		else
			{
			mode = COLOUR;
			}

		// Check for file Version		
		// Seek the last 26 bytes of the file
		if(fseek(fp, -26, SEEK_END))
			{
			// Error Quit
			fclose(fp);
			return NULL;
			}

		// Read in the last 26 Bytes of the File
		lResult = fread(&tgaft, 1, sizeof(TGAFOOTER), fp);

		if(lResult != sizeof(TGAFOOTER))
			{
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Check for the Marker at the end of the file	
		lResult = strcmp(tgaft.Signature, "TRUEVISION-XFILE.");

		if(lResult != 0)
			{
			// Not V2.0 File no Thumbnail
			m_error = NOTGATHUMBNAIL;
			fclose(fp);
			return NULL;
			}

		// Check for the existance of an extension area
		if(tgaft.ExtensionOffset == 0)
			{
			// No Thumbnail in this file
			m_error = NOTGATHUMBNAIL;
			fclose(fp);
			return NULL;
			}

		// Seek the extension area
		if(fseek(fp, tgaft.ExtensionOffset, SEEK_SET))
			{
			// Error Quit
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Read in the last 26 Bytes of the File
		lResult = fread(&tgaext, 1, sizeof(TGAEXTENSION), fp);

		if(lResult != sizeof(TGAEXTENSION))
			{
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Seek the thumbnail image
		if(fseek(fp, tgaext.StampOffset, SEEK_SET))
			{
			// Error Quit
			m_error = FILEREADERR;
			fclose(fp);
			return NULL;
			}

		// Read the Width and Height from the first two bytes
		// of the postage stamp data.
		fread(&stampWidth, 1, 1, fp);
		fread(&stampHeight, 1, 1, fp);

		if((stampWidth <= 0) || (stampWidth> 64) || (stampHeight <= 0) || (stampHeight> 64))
			{
			m_error = INVALIDTGAERR;
			fclose(fp);
			return NULL;
			}
		}

	// Clean Up
	*width = stampWidth;
	*height = stampHeight;
	fclose(fp);

	// Return Okay Image
	return hNew;
	}


///////////////////////////////////////////////////////////////////////////////////
//		SaveTGA32
//		Saves the buffer as a 32 bit True Color Image in TGA format
BOOL
TGAFile::SaveTGA32
	(
	const char *	fileName, // output path
	BYTE *				inBuf,		// BGR buffer
	UINT32				width,		// size in pixels
	UINT32				height
	)
	{
	long  lResult = 0;

	TGAHEADER tgahd;
	m_error = IMGOK;

	// Init the file Header to all zeros
	ZeroMemory(&tgahd, sizeof(tgahd));

	if(inBuf == NULL)
		{
		m_error = BADPARAMERR;
		return FALSE;
		}

	if((width == 0) || (height == 0))
		{
		m_error = BADPARAMERR;
		return FALSE;
		}

	// Initialize the Header for the File
	tgahd.IDLength = 0;
	tgahd.ColorMapType = 0;
	tgahd.ImageType = TGA_RAWRGBTYPE;
	tgahd.CMapStart = 0;
	tgahd.CMapLength = 0;
	tgahd.CMapDepth = 0;
	tgahd.XOffset = 0;
	tgahd.YOffset = 0;
	tgahd.Width   = (WORD)width;
	tgahd.Height  = (WORD)height;
	tgahd.PixelDepth = 32;
	tgahd.ImageDescriptor = 0;

	// Open a file to write
	FILE * fp = fopen(fileName, "wb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}

	// Write the Header to File.
	if((lResult = fwrite(&tgahd, 1, sizeof(TGAHEADER), fp)) != 18)
		{
		fclose(fp);
		m_error = FILEWRITEERR;
		return FALSE;
		}

	// Wrte the Bytes to file
	DWORD  destOffset = 0;
	BYTE   temp = 0;
	DWORD  rowStride = tgahd.Width * 4;

	for(UINT32 row = 0; row < tgahd.Height; row++)
		{
		//DWORD rowOffset = rowStride * row;
		DWORD  rowOffset = rowStride *((tgahd.Height -1) -row);

		for(UINT32 col = 0; col < tgahd.Width; col++)
			{
			destOffset = rowOffset + 4 * col;
			temp = *(inBuf + destOffset + 2);

			if(fwrite(&temp, 1, 1, fp) != 1)
				{
				m_error = FILEWRITEERR;
				fclose(fp);
				return FALSE;
				}
			temp = *(inBuf + destOffset + 1);

			if(fwrite(&temp, 1, 1, fp) != 1)
				{
				m_error = FILEWRITEERR;
				fclose(fp);
				return FALSE;
				}
			temp = *(inBuf + destOffset + 0);

			if(fwrite(&temp, 1, 1, fp) != 1)
				{
				m_error = FILEWRITEERR;
				fclose(fp);
				return FALSE;
				}
				
			if(fwrite(&temp, 1, 1, fp) != 1)
				{
				m_error = FILEWRITEERR;
				fclose(fp);
				return FALSE;
				}
			}
		}

	// Cleanup
	fclose(fp);
	m_error = IMGOK;
	return TRUE;
	}


///////////////////////////////////////////////////////////////////////////////////
//		Save8BitTGA
//		Save's to an 8 Bit Color mapped file using the Palette 
//		passed in to the function.
BOOL
TGAFile::Save8BitTGA(const char * fileName, // output path
BYTE * inBuf, // one BYTE per pixel colomapped image
UINT32 width, // Width of Image
UINT32 height, // Height of Image
__int32 colors, // number of colors (number of RGBQUADs)
RGBQUAD * colormap) // array of RGBQUADs 
	{
	long  lResult = 0;

	TGAHEADER tgahd;

	// Init
	m_error = IMGOK;

	// Init the file Header to all zeros
	ZeroMemory(&tgahd, sizeof(tgahd));

	if(inBuf == NULL)
		{
		m_error = BADPARAMERR;
		return FALSE;
		}

	if((width == 0) || (height == 0))
		{
		m_error = BADPARAMERR;
		return FALSE;
		}

	if(colormap == NULL)
		{
		m_error = BADPARAMERR;
		return FALSE;
		}

	// Initialize the Header for the File
	tgahd.IDLength = 0;
	tgahd.ColorMapType = 1;
	tgahd.ImageType = TGA_MAPRGBTYPE;
	tgahd.CMapStart = 0;
	tgahd.CMapLength = (SHORT)colors;
	tgahd.CMapDepth = 24;
	tgahd.XOffset = 0;
	tgahd.YOffset = 0;
	tgahd.Width   = (WORD)width;
	tgahd.Height  = (WORD)height;
	tgahd.PixelDepth = 8;
	tgahd.ImageDescriptor = 0x28;

	// Open a file to write
	FILE * fp = fopen(fileName, "wb");

	if(fp == NULL)
		{
		m_error = FILEOPENERR;
		return FALSE;
		}

	// Write the Header to File.
	if((lResult = fwrite(&tgahd, 1, sizeof(TGAHEADER), fp)) != 18)
		{
		m_error = FILEWRITEERR;
		fclose(fp);
		return FALSE;
		}

	// Write out the Colormap
	for(__int32 i = 0; i < colors; i++)
		{
		putc(colormap[i].rgbBlue, fp);
		putc(colormap[i].rgbGreen, fp);
		putc(colormap[i].rgbRed, fp);
		}

	// Wrte the Bytes to file
	DWORD  destOffset = 0;
	BYTE   temp = 0;

	for(UINT32 row = 0; row < tgahd.Height; row++)
		{
		for(UINT32 col = 0; col < tgahd.Width; col++)
			{
			temp = *(inBuf + destOffset + 0);

			if(fwrite(&temp, 1, 1, fp) != 1)
				{
				m_error = FILEWRITEERR;
				fclose(fp);
				return FALSE;
				}
			destOffset += 1;
			}
		}

	// Cleanup
	fclose(fp);
	m_error = IMGOK;
	return TRUE;
	}


////////////////////////////////////////////////////////////////////////////////////
//		TGA_GetMapEntry
//		Get the Color Values out of the
//		Color map in the TGA File
//		Return 0 on Success
BOOL
TGAFile::TGA_GetMapEntry(BYTE * Red, BYTE * Green, BYTE * Blue, BYTE * Alpha, FILE * fp, UINT32 Depth)
	{
	UINT32  j, k, l;
	BYTE    i, r, g, b, a = 0;
	long    lResult;

	switch(Depth)
		{
		case 8:  // Greyscale Read and Triplicate
			lResult = fread(&i, 1, 1, fp);

			// Check for error
			if(lResult != 1)
				{
				m_error = FILEREADERR;
				return FALSE;
				}

			// Set RGB Values
			r = i;
			g = i;
			b = i;
			break;

		case 16:  // 5 bits each of Red, Green, and Blue

		case 15:  // Watch for the Byte order
			lResult = fread(&j, 1, 1, fp);
			lResult = lResult + fread(&k, 1, 1, fp);

			// Check for error
			if(lResult != 2)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			l = j + k * 256;
			b = (BYTE)(((l >> 10) & 31) << 3);
			g = (BYTE)(((l >> 5) & 31) << 3);
			r = (BYTE)((l & 31) << 3);
			break;

		case 32:  // Read the Alpha bit a Throw it away

		case 24:  // Eight bits for each Red, Green and Blue
			lResult = fread(&i, 1, 1, fp);
			r = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			g = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			b = i;

			// Check for error
			if(lResult != 3)
				{
				m_error = FILEREADERR;
				return FALSE;
				}

			if(Depth == 32)
				{
				lResult = fread(&i, 1, 1, fp);

				if(lResult != 1)
					{
					m_error = FILEREADERR;
					return FALSE;
					}

				// Stroe Alpha bit
				a = i;
				}
			break;

		default:
			// Some Other Pixel Depth Which I don't support
			return FALSE;
		}
	*Red = r;
	*Green = g;
	*Blue = b;
	*Alpha = a;

	// Reutrn No Error
	return TRUE;
	}


////////////////////////////////////////////////////////////////////////////////////
//		TGA_GetFileVersion
//		Retrieves the Version of the TGA File
//		BYTES 8-23 of a Version 2.0 Footer will be equal 
//		to "TRUEVISION-XFILE" as ASCII
//		Returns - Version number 1 or 2
__int32
TGAFile::TGA_GetFileVersion(FILE * fp)
	{
	long    result;
	fpos_t  pos;

	TGAFOOTER tgaft;

	// Save the Current position of the File Stream
	if(fgetpos(fp, &pos))
		{
		// Error Quit
		return FILEREADERR;
		}

	// Seek the last 26 bytes of the file
	if(fseek(fp, -26, SEEK_END))
		{
		// Error Quit
		return FILEREADERR;
		}

	// Read in the last 26 Bytes of the File
	result = fread(&tgaft, 1, sizeof(TGAFOOTER), fp);

	if(result != sizeof(TGAFOOTER))
		{
		m_error = FILEREADERR;
		return FILEREADERR;
		}

	// Return the File Stream to its initial position
	if(fsetpos(fp, &pos))
		{
		// Error Quit
		return FILEREADERR;
		}

	// Check for the Marker at the end of the file	
	if(!strcspn(tgaft.Signature, "TRUEVISION-XFILE"))
		{
		// Marker found its V2.0 TGA
		return TGA_VERSIONTWO;
		}

	// No Marker was found Assume V1.0 TGA
	return TGA_VERSIONONE;
	}


////////////////////////////////////////////////////////////////////////////////////
//		TGA_getPixelValue
//		Retreve a pixel value from the buffer and parse
//		the value if its RLE encoded. Returns the RGB
//		value of the pixel.
//		Retruns - TRUE on success
BOOL
TGAFile::TGA_GetPixelValue
	(
	BYTE * rRed, 
	BYTE * rGrn, 
	BYTE * rBlu, 
	BYTE * rAlp, 
	BYTE ** ppTGAData, 
	UINT32 PixelDepth, 
	RGBQUAD * CColMap
	)
	{
	//
	// Buffered TGAs are always 32-bit,
	// so go direct from file to RGBA
	//
	*rRed = *((*ppTGAData)++);
	*rGrn = *((*ppTGAData)++);
	*rBlu = *((*ppTGAData)++);
	*rAlp = *((*ppTGAData)++);
	
	return TRUE;
	}

////////////////////////////////////////////////////////////////////////////////////
//		TGA_getPixelValue
//		Retreve a pixel value from the file and parse
//		the value if its RLE encoded. Returns the RGB
//		value of the pixel.
//		Retruns - TRUE on success
BOOL
TGAFile::TGA_GetPixelValue(BYTE * rRed, BYTE * rGrn, BYTE * rBlu, BYTE * rAlp, FILE * fp, UINT32 PixelDepth, RGBQUAD * CColMap)
	{
	BYTE  i, j, k;
	long  lResult;

	// Check for Run Length Encoding
	if((mode & RLENCO) != 0)
		{
		if(RLECount == 0)
			{  // Restrat the rum
			lResult = fread(&i, 1, 1, fp);

			if(lResult != 1)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			RLEFlag = (i & 0x80) >> 7;

			if(RLEFlag == 0)
				{  // Stream of unencoded pixels
				RLECount = i + 1;
				}
			else
				{  // Single Pixel Replicated
				RLECount = i-127;
				}
			RLECount--;  // ecrement count and get pixel
			}
		else
			{
			// I have already read the count and at least the first pixel
			RLECount--;

			if(RLEFlag != 0)
				{
				// Replicated Pixels
				goto PixelEncode;
				}
			}
		}

	// Rea the appropiate number of BYTES and break into RGB
	switch(PixelDepth)
		{
		case 8:  // Greyscale Read 1 Byte and Triplicate
			lResult = fread(&i, 1, 1, fp);

			if(lResult != 1)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			Red = i;
			Grn = i;
			Blu = i;
			l = i;
			break;

		case 16:  // 1 Bit alpha not used

		case 15:  // Five bits each for RGB watch byte ordering
			lResult = fread(&j, 1, 1, fp);
			lResult = lResult + fread(&k, 1, 1, fp);

			// Check for error
			if(lResult != 2)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			l = j + k * 256;
			Blu = (BYTE)(((l >> 10) & 31) << 3);
			Grn = (BYTE)(((l >> 5) & 31) << 3);
			Red = (BYTE)((l & 31) << 3);
			break;

		case 24:  // Eight bits each for RGB
			lResult = fread(&i, 1, 1, fp);
			Red = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			Grn = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			Blu = i;

			// opaque alpha (08jan00/bgw)
			Alpha = 0xFF;

			// Check for error
			if(lResult != 3)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			break;
			
		case 32:  // With alpha (08jan00/bgw)
			lResult = fread(&i, 1, 1, fp);
			Red = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			Grn = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			Blu = i;
			lResult = lResult + fread(&i, 1, 1, fp);
			Alpha = i;

			// Check for error
			if(lResult != 4)
				{
				m_error = FILEREADERR;
				return FALSE;
				}
			break;

		default:  // Unknown number of bis per pixel
			m_error = INVALIDTGAERR;
			return NULL;
		}
		
PixelEncode:  // Set the actual pixel values

	if((mode & MAPPED) == MAPPED)
		{
		// Remap Color Mapped Pixels
		*rRed =	CColMap[l].rgbRed;
		*rGrn = CColMap[l].rgbGreen;
		*rBlu = CColMap[l].rgbBlue;
		*rAlp = 0xFF; // opaque always (08jan00/bgw)
		}
	else
		{
		// Set Unmapped Values
		*rRed = Red;
		*rGrn = Grn;
		*rBlu = Blu;
		*rAlp = Alpha;
		}
	return TRUE;
	}

extern "C" unsigned char * LoadTGAFile
(
	const char * filename,
	int * width,
	int * height
)
{
	TGAFile tgaFile;
	return (unsigned char *)tgaFile.LoadTGA(filename, (UINT32 *)width, (UINT32 *)height);
}