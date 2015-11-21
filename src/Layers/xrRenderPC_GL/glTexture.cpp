// Texture.cpp: implementation of the CTexture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <gli/gli.hpp>

void fix_texture_name(LPSTR fn)
{
	LPSTR _ext = strext(fn);
	if (_ext &&
		(0 == stricmp(_ext, ".tga") ||
		0 == stricmp(_ext, ".dds") ||
		0 == stricmp(_ext, ".bmp") ||
		0 == stricmp(_ext, ".ogm")))
		*_ext = 0;
}

int get_texture_load_lod(LPCSTR fn)
{
	CInifile::Sect& sect	= pSettings->r_section("reduce_lod_texture_list");
	CInifile::SectCIt it_	= sect.Data.begin();
	CInifile::SectCIt it_e_	= sect.Data.end();

	CInifile::SectCIt it	= it_;
	CInifile::SectCIt it_e	= it_e_;

	for(;it!=it_e;++it)
	{
		if( strstr(fn, it->first.c_str()) )
		{
			if(psTextureLOD<1)
				return 0;
			else
			if(psTextureLOD<3)
				return 1;
			else
				return 2;
		}
	}

	if(psTextureLOD<2)
		return 0;
	else
	if(psTextureLOD<4)
		return 1;
	else
		return 2;
}

u32 calc_texture_size(int lod, u32 mip_cnt, u32 orig_size)
{
	if (1 == mip_cnt)
		return orig_size;

	int _lod = lod;
	float res = float(orig_size);

	while (_lod>0){
		--_lod;
		res -= res / 1.333f;
	}
	return iFloor(res);
}

GLuint	CRender::texture_load(LPCSTR fRName, u32& ret_msize, GLenum& ret_desc)
{
	GLuint					pTexture = 0;
	string_path				fn;
	u32						dwWidth, dwHeight;
	u32						img_size = 0;
	int						img_loaded_lod = 0;
	gli::gl::format			fmt;
	u32						mip_cnt = u32(-1);
	// validation
	R_ASSERT(fRName);
	R_ASSERT(fRName[0]);

	// make file name
	string_path				fname;
	strcpy_s(fname,fRName); //. andy if (strext(fname)) *strext(fname)=0;
	fix_texture_name		(fname);
	IReader* S				= NULL;
	// TODO: OGL: Implement bump mapping.
	//if (!FS.exist(fn,"$game_textures$",	fname,	".dds")	&& strstr(fname,"_bump"))	goto _BUMP_from_base;
	if (FS.exist(fn,"$level$",			fname,	".dds"))							goto _DDS;
	if (FS.exist(fn,"$game_saves$",		fname,	".dds"))							goto _DDS;
	if (FS.exist(fn,"$game_textures$",	fname,	".dds"))							goto _DDS;


#ifdef _EDITOR
	ELog.Msg(mtError, "Can't find texture '%s'", fname);
	return 0;
#else

	Msg("! Can't find texture '%s'", fname);
	R_ASSERT(FS.exist(fn, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"));
	goto _DDS;

	//	xrDebug::Fatal(DEBUG_INFO,"Can't find texture '%s'",fname);

#endif

_DDS:
	{
		// Load and get header
		S = FS.r_open(fn);
#ifdef DEBUG
		Msg("* Loaded: %s[%d]b", fn, S->length());
#endif // DEBUG
		img_size = S->length();
		R_ASSERT(S);
		gli::texture Texture = gli::load((char*)S->pointer(), img_size);
		R_ASSERT(!Texture.empty());
		if (gli::is_target_cube(Texture.target()))					goto _DDS_CUBE;
		else														goto _DDS_2D;

	_DDS_CUBE:
		{
			gli::gl GL;
			mip_cnt = Texture.levels();
			dwWidth = Texture.dimensions().x;
			dwHeight = Texture.dimensions().y;
			fmt = GL.translate(Texture.format());

			glGenTextures(1, &pTexture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, pTexture);
			CHK_GL(glTexStorage2D(GL_TEXTURE_CUBE_MAP, mip_cnt, fmt.Internal, dwWidth, dwHeight));

			for (size_t face = 0; face < Texture.faces(); face++)
			{
				for (size_t i = 0; i < mip_cnt; i++)
				{
					if (gli::is_compressed(Texture.format()))
					{
						CHK_GL(glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, i, 0, 0, Texture.dimensions(i).x, Texture.dimensions(i).y,
							fmt.Internal, Texture.size(i), Texture.data(0, face, i)));
					}
					else {
						CHK_GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, i, 0, 0, Texture.dimensions(i).x, Texture.dimensions(i).y,
							fmt.External, fmt.Type, Texture.data(0, face, i)));
					}
				}
			}
			FS.r_close(S);

			// OK
			ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
			ret_desc = GL_TEXTURE_CUBE_MAP;
			return					pTexture;
		}
	_DDS_2D:
		{
			// Check for LMAP and compress if needed
			strlwr(fn);


			// Load   SYS-MEM-surface, bound to device restrictions
			gli::gl GL;
			mip_cnt = Texture.levels();
			dwWidth = Texture.dimensions().x;
			dwHeight = Texture.dimensions().y;
			fmt = GL.translate(Texture.format());

			glGenTextures(1, &pTexture);
			glBindTexture(GL_TEXTURE_2D, pTexture);
			CHK_GL(glTexStorage2D(GL_TEXTURE_2D, mip_cnt, fmt.Internal, dwWidth, dwHeight));
			for (size_t i = 0; i < mip_cnt; i++)
			{
				if (gli::is_compressed(Texture.format()))
				{
					CHK_GL(glCompressedTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, Texture.dimensions(i).x, Texture.dimensions(i).y,
						fmt.Internal, Texture.size(i), Texture.data(0, 0, i)));
				}
				else {
					CHK_GL(glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, Texture.dimensions(i).x, Texture.dimensions(i).y,
						fmt.External, fmt.Type, Texture.data(0, 0, i)));
				}
			}
			FS.r_close(S);

			// OK
			img_loaded_lod = get_texture_load_lod(fn);
			ret_msize = calc_texture_size(img_loaded_lod, mip_cnt, img_size);
			ret_desc = GL_TEXTURE_2D;
			return					pTexture;
		}
	}
}
