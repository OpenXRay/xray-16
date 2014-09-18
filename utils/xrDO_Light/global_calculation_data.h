#ifndef __GLOBAL_CALCULATION_DATA_H__
#define __GLOBAL_CALCULATION_DATA_H__

#include "../../editors/LevelEditor/Engine/communicate.h"
#include "base_lighting.h"
#include "global_slots_data.h"
#include "build_texture.h"
#include "global_slots_data.h"

class Shader_xrLC_LIB;
//-----------------------------------------------------------------
struct global_claculation_data
{
	base_lighting					g_lights;
	CDB::MODEL						RCAST_Model;
	Fbox							LevelBB;


	Shader_xrLC_LIB*				g_shaders_xrlc;

	b_params						g_params;

	global_slots_data				slots_data;

	xr_vector<b_material>			g_materials;
	xr_vector<b_shader>				g_shader_render;
	xr_vector<b_shader>				g_shader_compile;
	xr_vector<b_BuildTexture>		g_textures;
	xr_vector<b_rc_face>			g_rc_faces;
///////////////////////////////////////////////////////////////////
			global_claculation_data		(): g_shaders_xrlc( 0 ) {}
	void	xrLoad						( );
};
extern global_claculation_data	gl_data;
//-----------------------------------------------------------------
#endif //__GLOBAL_CALCULATION_DATA_H__