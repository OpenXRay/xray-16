#include "stdafx.h"
#pragma hdrstop

#include "BlenderListLoader.h"
#include "Shader_XRLC.h"
#include "GameMtlLib.h"

IC bool str_pred(LPCSTR x, LPCSTR y)
{
	return xr_strcmp(x,y)<0;
}

void ClearList(LPSTRVec& lst)
{
	for (LPSTRIt it=lst.begin(); it!=lst.end(); it++)
		xr_free			(*it);
}

int LoadBlenderList(LPSTRVec& lst)
{
	ClearList			(lst);

	// Load blenders
	string_path			sh;
	FS.update_path		(sh,"$game_data$","shaders.xr");
	IReader* R			= FS.r_open(sh); 
	R_ASSERT2			(R,sh);
	IReader* fs			= R->open_chunk(3); R_ASSERT(fs);

	xr_string			buf;
	lst.resize			(fs->r_u32());
	for (LPSTRIt it=lst.begin(); it!=lst.end(); it++){
		fs->r_stringZ	(buf); 
		*it=xr_strdup	(buf.c_str());
	}
	fs->close();
	std::sort			(lst.begin(), lst.end(), str_pred);
	return lst.size();
}

int LoadShaderLCList(LPSTRVec& lst)
{
	Shader_xrLC_LIB	LIB;

	ClearList			(lst);

	string_path fn;
	FS.update_path		(fn,"$game_data$","shaders_xrlc.xr");
	LIB.Load			(fn);

	lst.resize			(LIB.Library().size());
	LPSTRIt s_it		= lst.begin();
	for (Shader_xrLCIt l_it=LIB.Library().begin(); l_it!=LIB.Library().end(); l_it++,s_it++)
		*s_it			= xr_strdup(l_it->Name);
	LIB.Unload			();
	std::sort			(lst.begin(), lst.end(), str_pred);
	return lst.size		();
}

int LoadGameMtlList(LPSTRVec& lst)
{
	ClearList			(lst);

	GMLib.Load			();

	lst.resize			(GMLib.CountMaterial());
	LPSTRIt s_it		= lst.begin();
	for (GameMtlIt gm_it=GMLib.FirstMaterial(); gm_it!=GMLib.LastMaterial(); gm_it++,s_it++)
		*s_it			= xr_strdup(*(*gm_it)->m_Name);
	GMLib.Unload		();
	std::sort			(lst.begin(), lst.end(), str_pred);
	return lst.size		();
}
