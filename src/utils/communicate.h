#ifndef _XR_COMM_
#define _XR_COMM_

#include "Common/LevelStructure.hpp"

#pragma pack(push, 4)

const u32 XR_MAX_PORTAL_VERTS = 6;

// internal use
struct b_rc_face
{
    u16 dwMaterial;
    u32 dwMaterialGame;
    Fvector2 t[3]; // TC
    u16 reserved;
};

// All types to interact with xrLC
typedef Fvector b_vertex;

struct b_face
{
    u32 v[3]; // vertices
    Fvector2 t[3]; // TC
    u16 dwMaterial; // index of material
    u32 dwMaterialGame; // unique-ID of game material
};

struct b_material
{
    u16 surfidx; // indices of texture surface
    u16 shader; // index of shader that combine them
    u16 shader_xrlc; // compiler options
    u16 sector; // ***
    u16 reserved; //
    u32 internal_max_area; //
};

struct b_shader
{
    string128 name;
};

struct b_texture
{
    string128 name;
    u32 dwWidth;
    u32 dwHeight;
    BOOL bHasAlpha;
    u32* pSurface;
};

struct b_light_control // controller or "layer", 30fps
{
    string64 name; // empty for base layer
    u32 count; // 0 for base layer
    // u32				data[];
};

struct b_light
{
    u32 controller_ID; // 0 = base layer
    Flight data;
};

struct b_light_static : public b_light // For static lighting
{
};

struct b_light_dynamic : public b_light // For dynamic models
{
    svector<u16, 16> sectors;
};

struct b_glow
{
    Fvector P;
    float size;
    u32 flags; // 0x01 = non scalable
    u32 dwMaterial; // index of material
};

struct b_portal
{
    u16 sector_front;
    u16 sector_back;
    svector<Fvector, XR_MAX_PORTAL_VERTS> vertices;
};

struct b_lod_face
{
    Fvector v[4];
    Fvector2 t[4];
};

struct b_lod
{
    b_lod_face faces[8];
    u32 dwMaterial;
};

/*
    u32 NUMBER-OF-OBJECTS

    stringZ		name
    u32			vert_count
    b_vertex	vertices[]
    u32			face_count
    b_faces		faces[]
    u16			lod_id;			// u16(-1) = no lod, just static geometry
*/
struct b_mu_model
{
    string128 name;
    int m_iVertexCount;
    b_vertex* m_pVertices;
    int m_iFaceCount;
    b_face* m_pFaces;
    u32* m_smgroups;
    u16 lod_id; // u16(-1) = no lod, just static geometry
};

/*
    self-describing
*/
struct b_mu_reference
{
    u32 model_index;
    Fmatrix transform;
    Flags32 flags;
    u16 sector;
    u32 reserved[8];
};

struct b_params
{
    // Normals & optimization
    float m_sm_angle; // normal smooth angle		- 89.0
    float m_weld_distance; // by default 0.005f		- 5mm

    // Light maps
    float m_lm_pixels_per_meter; // LM - by default: 4 ppm
    u32 m_lm_jitter_samples; // 1/4/9 - by default		- 4
    u32 m_lm_rms_zero; // RMS - after what the lightmap will be shrinked to ZERO pixels
    u32 m_lm_rms; // RMS - shrink and recalc

    // build quality
    u16 m_quality;
    u16 u_reserved;

    // Progressive
    float f_reserved[6];

    void SaveLTX(CInifile& ini)
    {
        LPCSTR section = "build_params";
        ini.w_float(section, "smooth_angle", m_sm_angle);
        ini.w_float(section, "weld_distance", m_weld_distance);
        ini.w_float(section, "light_pixel_per_meter", m_lm_pixels_per_meter);
        ini.w_u32(section, "light_jitter_samples", m_lm_jitter_samples);
        ini.w_u32(section, "light_rms_zero", m_lm_rms_zero);
        ini.w_u32(section, "light_rms", m_lm_rms);
        ini.w_u16(section, "light_quality", m_quality);
        ini.w_u16(section, "light_quality_reserved", u_reserved);
        for (u32 i = 0; i < 6; ++i)
        {
            string128 buff;
            xr_sprintf(buff, sizeof(buff), "reserved_%d", i);
            ini.w_float(section, buff, f_reserved[i]);
        }
    }

    void LoadLTX(CInifile& ini)
    {
        LPCSTR section = "build_params";
        m_sm_angle = ini.r_float(section, "smooth_angle");
        m_weld_distance = ini.r_float(section, "weld_distance");
        m_lm_pixels_per_meter = ini.r_float(section, "light_pixel_per_meter");
        m_lm_jitter_samples = ini.r_u32(section, "light_jitter_samples");
        m_lm_rms_zero = ini.r_u32(section, "light_rms_zero");
        m_lm_rms = ini.r_u32(section, "light_rms");
        m_quality = ini.r_u16(section, "light_quality");
        u_reserved = ini.r_u16(section, "light_quality_reserved");
        for (u32 i = 0; i < 6; ++i)
        {
            string128 buff;
            xr_sprintf(buff, sizeof(buff), "reserved_%d", i);
            f_reserved[i] = ini.r_float(section, buff);
        }
    }

    void Init()
    {
        // Normals & optimization
        m_sm_angle = 75.f;
        m_weld_distance = 0.005f;

        // Light maps
        m_lm_rms_zero = 4;
        m_lm_rms = 4;

        setHighQuality();
    }

    void setDraftQuality()
    {
        m_quality = ebqDraft;
        m_lm_pixels_per_meter = 0.1f;
        m_lm_jitter_samples = 1;
    }

    void setHighQuality()
    {
        m_quality = ebqHigh;
        m_lm_pixels_per_meter = 10;
        m_lm_jitter_samples = 9;
    }
};
#pragma pack(pop)

enum EBUILD_CHUNKS
{
    EB_Version = 0, // XRCLC_CURRENT_VERSION
    EB_Parameters,
    EB_Vertices,
    EB_Faces,
    EB_Materials,
    EB_Shaders_Render,
    EB_Shaders_Compile,
    EB_Textures,
    EB_Glows,
    EB_Portals,
    EB_Light_control,
    EB_Light_static,
    EB_Light_dynamic,
    EB_LOD_models,
    EB_MU_models,
    EB_MU_refs,
    EB_SmoothGroups,

    EB_FORCE_DWORD = u32(-1)
};

#endif
