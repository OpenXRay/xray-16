#ifndef _TextureDescrManager_included_
#define _TextureDescrManager_included_

#pragma once
#include "ETextureParams.h"

class cl_dt_scaler;

class CTextureDescrMngr
{
	struct texture_assoc
	{
		shared_str			detail_name;
		//R_constant_setup*	cs;
		u8					usage;
        texture_assoc       () : /*cs(NULL),*/ usage(0) {}
		~texture_assoc		() { /*xr_delete(cs);*/ }

	};
	struct texture_spec
	{
		shared_str			m_bump_name;
		float				m_material;
		bool				m_use_steep_parallax;
	};
	struct texture_desc{
		texture_assoc*		m_assoc;
		texture_spec*		m_spec;
        texture_desc            ():m_assoc(NULL),m_spec(NULL){}
	};
	DEFINE_MAP(shared_str, texture_desc,	map_TD,	map_TDIt);
	DEFINE_MAP(shared_str, cl_dt_scaler*,	map_CS,	map_CSIt);

	map_TD									m_texture_details;
	map_CS									m_detail_scalers;

	void		LoadTHM		(LPCSTR initial);

public:
				~CTextureDescrMngr();
	void		Load		();
	void		UnLoad		();
public:
	shared_str	GetBumpName		(const shared_str& tex_name) const;
	float		GetMaterial		(const shared_str& tex_name) const;
	void		GetTextureUsage	(const shared_str& tex_name, BOOL& bDiffuse, BOOL& bBump) const;
	BOOL		GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup* &CS) const;
	BOOL		UseSteepParallax(const shared_str& tex_name) const;
};
#endif