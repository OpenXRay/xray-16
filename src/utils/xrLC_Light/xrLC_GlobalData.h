#pragma once

#include "utils/Shader_xrLC.h"
#include "xrCore/xrPool.h"
#include "xrfacedefs.h"
#include "xrdeflectordefs.h"
#include "b_build_texture.h"
#include "base_lighting.h"
#include "utils/communicate.h"
// struct _face;
// struct _vertex;
namespace CDB
{
class MODEL;
class CollectorPacked;
};
class CLightmap;
class xrMU_Model;
class xrMU_Reference;
class base_Vertex;
class base_Face;

struct compilers_global_data
{
    xr_vector<b_BuildTexture> _textures;
    xr_vector<b_material> _materials;
    Shader_xrLC_LIB _shaders;
    b_params _g_params;
    base_lighting _L_static;
    CDB::MODEL* _RCAST_Model;
};

class XRLC_LIGHT_API xrLC_GlobalData
{
    compilers_global_data _cl_globs;

    CMemoryWriter _err_invalid;
    CMemoryWriter _err_multiedge;
    CMemoryWriter _err_tjunction;
    xr_vector<CLightmap*> _g_lightmaps;
    xr_vector<xrMU_Model*> _mu_models;
    xr_vector<xrMU_Reference*> _mu_refs;
    vecVertex _g_vertices;
    vecFace _g_faces;
    vecDefl _g_deflectors;

    bool _b_nosun;
    bool _gl_linear;

private:
    bool b_vert_not_register;

public:
public:
    xrLC_GlobalData(); //:_RCAST_Model (0), _b_nosun(false),_gl_linear(false){}
    ~xrLC_GlobalData();
    IC xr_vector<b_BuildTexture>& textures() { return _cl_globs._textures; }
    IC xr_vector<CLightmap*>& lightmaps() { return _g_lightmaps; }
    IC xr_vector<b_material>& materials() { return _cl_globs._materials; }
    IC Shader_xrLC_LIB& shaders() { return _cl_globs._shaders; }
    IC CMemoryWriter& err_invalid() { return _err_invalid; }
    IC CMemoryWriter& err_multiedge() { return _err_multiedge; };
    IC CMemoryWriter& err_tjunction() { return _err_tjunction; };
    IC b_params& g_params() { return _cl_globs._g_params; }
    Face* create_face();
    void destroy_face(Face*& f);

    Vertex* create_vertex();
    void destroy_vertex(Vertex*& f);

    void vertices_isolate_and_pool_reload();

    vecVertex& g_vertices() { return _g_vertices; }
    vecFace& g_faces() { return _g_faces; }
    vecDefl& g_deflectors() { return _g_deflectors; }
    bool b_r_vertices();
    bool vert_construct_register() { return !b_r_vertices() && !b_vert_not_register; }
    //		bool						b_r_faces		()		;
    base_lighting& L_static() { return _cl_globs._L_static; }
    CDB::MODEL* RCAST_Model() { return _cl_globs._RCAST_Model; }
    xr_vector<xrMU_Model*>& mu_models() { return _mu_models; }
    xr_vector<xrMU_Reference*>& mu_refs() { return _mu_refs; }
    void read_mu_models(INetReader& r);
    void write_mu_models(IWriter& w) const;

    void read_mu_model_refs(INetReader& r);
    void write_mu_model_refs(IWriter& w) const;
    void close_models_read();
    void close_models_write() const;

    bool b_nosun() { return _b_nosun; }
    bool gl_linear() { return _gl_linear; }
    IC void b_nosun_set(bool v) { _b_nosun = v; }
    void initialize();
    void destroy_rcmodel();

    void create_rcmodel(CDB::CollectorPacked& CL);

    void clear_build_textures_surface();

    void clear_build_textures_surface(const xr_vector<u32>& exept);

    void set_faces_indexses();
    void set_vertices_indexses();
    // void						create_write_faces	() const;
    // void						destroy_write_faces	() const;
    // void						create_read_faces	() ;
    // void						destroy_read_faces	() ;

    //		tread_faces					*get_read_faces		()	;
    //		twrite_faces				*get_write_faces	()	;

    void gl_mesh_clear();

private:
    // std::pair<u32,u32>					get_id				( const _face * v ) const;
    // std::pair<u32,u32>					get_id				( const _vertex * v ) const;
public:
    void read_base(INetReader& r);
    void write_base(IWriter& w) const;
    void read(INetReader& r);
    void write(IWriter& w) const;
    void read_vertices(INetReader& r);
    void write_vertices(IWriter& w) const;
    void read_lm_data(INetReader& r);
    void write_lm_data(IWriter& w) const;

    void read_modes_color(INetReader& r);
    void write_modes_color(IWriter& w) const;

    void read(INetReader& r, base_Face*& f);
    void write(IWriter& r, const base_Face* f) const;
    void clear();
    void clear_mesh();
    void clear_mu_models();
    void mu_models_calc_materials();
    //	void						cdb_read_create	() ;
private:
};

extern "C" XRLC_LIGHT_API xrLC_GlobalData* lc_global_data();
extern "C" XRLC_LIGHT_API void create_global_data();
extern "C" XRLC_LIGHT_API void destroy_global_data();
extern "C" XRLC_LIGHT_API u32 InvalideFaces();
XRLC_LIGHT_API void ImplicitLighting(BOOL net);

extern xrLC_GlobalData* data;
IC xrLC_GlobalData* inlc_global_data() { return data; }
static LPCSTR gl_data_net_file_name = "tmp_global_data";

#ifdef _DEBUG
static LPCSTR libraries =
    "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,xrAPI.dll,DXT.dll,BugTrap.dll,BugTrapD.dll,FreeImage.dll,"
    "msvcr80.dll,Microsoft.VC80.CRT.manifest";
#else
static LPCSTR libraries =
    "XRLC_LightStab.dll,XRLC_Light.dll,xrCore.dll,xrCDB.dll,xrAPI.dll,DXT.dll,BugTrap.dll,FreeImage.dll,msvcr80.dll,"
    "Microsoft.VC80.CRT.manifest";
#endif
//#define NET_CMP
//#define LOAD_GL_DATA
