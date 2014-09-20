#include "stdafx.h"
#pragma hdrstop

#include <freeimage/freeimage.h>

struct SExts{
	xr_vector<LPSTR>	exts;
    void format_register(LPCSTR ext)
    {
    	if (ext&&ext[0]){
			for (u32 i=0; i<exts.size(); i++)
    			if (0==stricmp(exts[i],ext)) return;
    		exts.push_back(xr_strdup(ext));
        }
    }
    u32 size()	{	return (u32)exts.size();	}
    LPSTR operator [](int k){return exts[k];}
	~SExts()
    {
		for (u32 i=0; i<exts.size(); i++)
    		xr_free(exts[i]);
        exts.clear();
    }
};
SExts formats;

void	Surface_FormatExt(FREE_IMAGE_FORMAT f)
{
	LPCSTR n=FreeImage_GetFIFExtensionList(f);
    if (n){
        LPSTR base = xr_strdup(n);
        LPSTR ext = base;
        LPSTR cur = ext;
        for	(; ext[0]; ext++){
        if (ext[0]==','){
                ext[0] = 0;
                formats.format_register(cur);
                cur = ++ext;
            }
        }
        if (cur&&cur[0]) formats.format_register(cur);
        xr_free(base);
    }
}
void	Surface_Init()
{
	Msg("* ImageLibrary version: %s",FreeImage_GetVersion());

    formats.format_register("tga");
	Surface_FormatExt(FIF_BMP);
	Surface_FormatExt(FIF_ICO);
	Surface_FormatExt(FIF_JPEG);
	Surface_FormatExt(FIF_JNG);
	Surface_FormatExt(FIF_KOALA);
	Surface_FormatExt(FIF_LBM);
	Surface_FormatExt(FIF_MNG);
	Surface_FormatExt(FIF_PBM);
	Surface_FormatExt(FIF_PBMRAW);
	Surface_FormatExt(FIF_PCD);
	Surface_FormatExt(FIF_PCX);
	Surface_FormatExt(FIF_PGM);
	Surface_FormatExt(FIF_PGMRAW);
	Surface_FormatExt(FIF_PNG);
	Surface_FormatExt(FIF_PPM);
	Surface_FormatExt(FIF_PPMRAW);
	Surface_FormatExt(FIF_RAS);
	Surface_FormatExt(FIF_TARGA);
	Surface_FormatExt(FIF_TIFF);
	Surface_FormatExt(FIF_WBMP);
	Surface_FormatExt(FIF_PSD);
	Surface_FormatExt(FIF_IFF);

	Msg("* %d supported formats",formats.size());
}

BOOL	Surface_Detect(string_path& F, LPSTR N)
{
	FS.update_path		(F,"$game_textures$",strconcat(sizeof(F),F,N,".dds"));
	int					h = _open(F,O_RDONLY|O_BINARY);
	if (h > 0) {
		_close			(h);
		return			(TRUE);
	}

	return				(FALSE);
}

FIBITMAP*	Surface_Load(char* full_name)
{
	// load
	FREE_IMAGE_FORMAT	fif		= FreeImage_GetFIFFromFilename(full_name);
	FIBITMAP*			map		= FreeImage_Load(fif,full_name);
	if (0==map)			return NULL;

	// check if already 32bpp
	if (32==FreeImage_GetBPP(map))	return map;

	// convert
	FIBITMAP*			map32	= FreeImage_ConvertTo32Bits(map);
	if (0==map32)		map32	= map;
	else				FreeImage_Unload(map);

 	return				map32;
}

u32*	Surface_Load(char* name, u32& w, u32& h)
{
	if (strchr(name,'.')) *(strchr(name,'.')) = 0;

	// detect format
	string_path		full;
	if (!Surface_Detect(full,name)) return NULL;

	FIBITMAP* map32		= Surface_Load(full);

	h					= FreeImage_GetHeight	(map32);
	w					= FreeImage_GetWidth	(map32);

	u32			memSize	= w*h*4;
	u32*		memPTR	= (u32*)(xr_malloc(memSize));
	u32*		memDATA	= (u32*)(FreeImage_GetScanLine(map32,0));
	CopyMemory	(memPTR,memDATA,memSize);
	FreeImage_Unload		(map32);
	return		memPTR;
}
