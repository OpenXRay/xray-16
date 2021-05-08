#pragma once

#include "xrCDB/xrCDB.h"
#include "utils/Shader_xrLC.h"
#include "xrAICore/Navigation/level_graph.h"
#include "utils/communicate.h"
#include "ESceneAIMapTools_Export.h"
#include "Layers/xrRender/ETextureParams.h"

// base patch used all the time up to merging
const u32 InvalidNode = (1 << 24) - 1;
const u32 UnkonnectedNode = 0xfffffff0;
const u16 InvalidSector = 0xff;

struct vertex // definition of "patch" or "node"
{
    union
    {
        struct
        {
            u32 n1, n2, n3, n4; // neighbourh patches (Left,Forward,Right,Backward)
        };
        u32 n[4];
    };
    Fplane Plane; // plane of patch
    Fvector Pos; // position of patch center
    u16 Sector; //

    u32 Group;

    float LightLevel;

    float high_cover[4]; // cover in four directions
    float low_cover[4]; // cover in four directions

    vertex()
    {
        n1 = n2 = n3 = n4 = UnkonnectedNode;
        Sector = InvalidSector;
        Group = 0;
    }
    u32 nLeft() { return n1; }
    u32 nForward() { return n2; }
    u32 nRight() { return n3; }
    u32 nBack() { return n4; }
};

//using DWORDs = xr_vector<u32>;

//#define LT_DIRECT 0
//#define LT_POINT 1
//#define LT_SECONDARY 2

/*struct R_Light
{
    u32 type; // Type of light source
    float amount; // Diffuse color of light
    Fvector position; // Position in world space
    Fvector direction; // Direction in world space
    float range; // Cutoff range
    float range2; // ^2
    float attenuation0; // Constant attenuation
    float attenuation1; // Linear attenuation
    float attenuation2; // Quadratic attenuation

    Fvector tri[3]; // Cached triangle for ray-testing
};*/

struct SCover
{
    u8 cover[4];
};

using Nodes = xr_vector<vertex>;
using Vectors = xr_vector<Fvector>;
using Marks = xr_vector<u8>;
//using Lights = xr_vector<R_Light>;

// data
extern CDB::MODEL Level;
extern Nodes g_nodes;
extern SAIParams g_params;

struct b_BuildTexture : public b_texture
{
    STextureParams THM;

    b_BuildTexture() : b_texture() {}
    b_BuildTexture(IReader*& file) : b_texture(file) {}
    b_BuildTexture(const b_texture& p) : b_texture(p) {}

    u32& Texel(u32 x, u32 y) { return pSurface[y * dwWidth + x]; }
    void Vflip()
    {
        R_ASSERT(pSurface);
        for (u32 y = 0; y < dwHeight / 2; y++)
        {
            u32 y2 = dwHeight - y - 1;
            for (u32 x = 0; x < dwWidth; x++)
            {
                u32 t = Texel(x, y);
                Texel(x, y) = Texel(x, y2);
                Texel(x, y2) = t;
            }
        }
    }
};

extern Shader_xrLC_LIB* g_shaders_xrlc;
extern xr_vector<b_material> g_materials;
//extern xr_vector<b_shader> g_shader_render;
//extern xr_vector<b_shader> g_shader_compile;
extern xr_vector<b_BuildTexture> g_textures;
extern xr_vector<b_rc_face> g_rc_faces;

// phases
void xrLoad(LPCSTR name, bool draft_mode);
void xrCover(bool pure_covers);
void xrSaveNodes(LPCSTR name, LPCSTR out_name);

// constants
const int RCAST_MaxTris = (2 * 1024);
const int RCAST_Count = 6;
const int RCAST_Total = (2 * RCAST_Count + 1) * (2 * RCAST_Count + 1);
const float RCAST_Depth = 1.f;

const float cover_distance = 30.f;
const float high_cover_height = 1.5f;
const float low_cover_height = 0.6f;
const float cover_sqr_dist = cover_distance * cover_distance;

const float sim_angle = 20.f;
const float sim_dist = 0.15f;
const int sim_light = 32;
const float sim_cover = 48;

struct CNodePositionCompressor
{
    IC CNodePositionCompressor(NodePosition& Pdest, Fvector& Psrc, hdrNODES& H);
};

IC CNodePositionCompressor::CNodePositionCompressor(NodePosition& Pdest, Fvector& Psrc, hdrNODES& H)
{
    float sp = 1 / g_params.fPatchSize;
    int row_length = iFloor((H.aabb.vMax.z - H.aabb.vMin.z) / H.size + EPS_L + 1.5f);
    int pxz = iFloor((Psrc.x - H.aabb.vMin.x) * sp + EPS_L + .5f) * row_length +
        iFloor((Psrc.z - H.aabb.vMin.z) * sp + EPS_L + .5f);
    int py = iFloor(65535.f * (Psrc.y - H.aabb.vMin.y) / (H.size_y) + EPS_L);
    VERIFY(pxz < (1 << MAX_NODE_BIT_COUNT) - 1);
    Pdest.xz(pxz);
    clamp(py, 0, 65535);
    Pdest.y(u16(py));
}
