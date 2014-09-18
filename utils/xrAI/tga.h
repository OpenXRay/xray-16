// file: targasaver.h

#ifndef _INCDEF_TARGASAVER_H_
#define _INCDEF_TARGASAVER_H_

#pragma pack(push,1)
struct tgaImgSpecHeader{
	u16		tgaXOrigin;
	u16		tgaYOrigin;
	u16		tgaXSize;
	u16		tgaYSize;
	BYTE	tgaDepth;
	BYTE	tgaImgDesc;
};
struct tgaHeader{
	BYTE	tgaIDL;
	BYTE	tgaMapType;
	BYTE	tgaImgType;
	BYTE	tgaClrMapSpec[5];
	tgaImgSpecHeader tgaImgSpec;
};
#pragma pack(pop)


#define IMG_24B 0
#define IMG_32B 1

class ENGINE_API TGAdesc
{
public:
	int format;
	int scanlenght;
	int width,height;
	void *data;
public:
	TGAdesc()	{ data = 0; };
	~TGAdesc()	{};

	void maketga( IWriter &fs );
//	void maketga( int hf );
};

void	ENGINE_API	tga_save	(LPCSTR name, u32 w, u32 h, void* data, BOOL alpha );

#endif /*_INCDEF_TARGASAVER_H_*/

