#pragma once
#include "utils/communicate.h"
#include "base_lighting.h"
#include "global_slots_data.h"
#include "b_build_texture.h"
#include "global_slots_data.h"
#include "xrCDB/xrCDB.h"
class Shader_xrLC_LIB;
//-----------------------------------------------------------------
struct global_claculation_data
{
    base_lighting g_lights; /////////////////////lc
    Shader_xrLC_LIB* g_shaders_xrlc; ////////////////lc
    b_params g_params; //////////////////////lc
    xr_vector<b_material> g_materials; ///////////////////lc
    xr_vector<b_BuildTexture> g_textures; ////////////////////lc
    CDB::MODEL RCAST_Model; ///////////////////lc

    Fbox LevelBB; //-----------============
    global_slots_data slots_data; //-------=============
    xr_vector<b_shader> g_shader_compile; //-----==========
    xr_vector<b_rc_face> g_rc_faces; //---------===============
    ///////////////////////////////////////////////////////////////////////
    void read(INetReader& r);
    void write(IWriter& w) const;
    //////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    global_claculation_data() : g_shaders_xrlc(0) {}
    void xrLoad();
};
extern global_claculation_data gl_data;
