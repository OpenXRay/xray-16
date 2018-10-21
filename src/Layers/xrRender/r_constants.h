#ifndef r_constantsH
#define r_constantsH
#pragma once
#include "xrCore/xr_resource.h"
#if defined(USE_DX10) || defined(USE_DX11)
#include "Layers/xrRenderDX10/dx10ConstantBuffer.h"
#endif // USE_DX10

class ECORE_API R_constant_setup;

enum
{
    RC_float = 0,
    RC_int = 1,
    RC_bool = 2,
    RC_sampler = 99, // DX9 shares index for sampler and texture
    RC_dx10texture = 100, // For DX10 sampler and texture are different resources
    RC_dx11UAV = 101
};
enum
{
    RC_1x1 = 0, // vector1, or scalar
    RC_1x4, // vector4
    RC_1x3, // vector3
    RC_1x2, // vector2
    RC_2x4, // 4x2 matrix, transpose
    RC_3x4, // 4x3 matrix, transpose
    RC_4x4, // 4x4 matrix, transpose
    RC_1x4a, // array: vector4
    RC_3x4a, // array: 4x3 matrix, transpose
    RC_4x4a // array: 4x4 matrix, transpose
};
enum
{
    //  Don't change this since some code relies on magic numbers
    RC_dest_pixel = (1 << 0),
    RC_dest_vertex = (1 << 1),
    RC_dest_sampler = (1 << 2), //  For DX10 it's either sampler or texture
    RC_dest_geometry = (1 << 3), // DX10 only
    RC_dest_hull = (1 << 4), // DX11 only
    RC_dest_domain = (1 << 5), //   DX11 only
    RC_dest_compute = (1 << 6), //  DX11 only
    RC_dest_compute_cb_index_mask = 0xF0000000, // Buffer index == 0..14
    RC_dest_compute_cb_index_shift = 28,
    RC_dest_domain_cb_index_mask = 0x0F000000, // Buffer index == 0..14
    RC_dest_domain_cb_index_shift = 24,
    RC_dest_hull_cb_index_mask = 0x00F00000, // Buffer index == 0..14
    RC_dest_hull_cb_index_shift = 20,
    RC_dest_pixel_cb_index_mask = 0x000F0000, // Buffer index == 0..14
    RC_dest_pixel_cb_index_shift = 16,
    RC_dest_vertex_cb_index_mask = 0x0000F000, // Buffer index == 0..14
    RC_dest_vertex_cb_index_shift = 12,
    RC_dest_geometry_cb_index_mask = 0x00000F00, // Buffer index == 0..14
    RC_dest_geometry_cb_index_shift = 8,
};

enum // Constant buffer index masks
{
    CB_BufferIndexMask = 0xF, // Buffer index == 0..14

    CB_BufferTypeMask = 0x70,
    CB_BufferPixelShader = 0x10,
    CB_BufferVertexShader = 0x20,
    CB_BufferGeometryShader = 0x30,
    CB_BufferHullShader = 0x40,
    CB_BufferDomainShader = 0x50,
    CB_BufferComputeShader = 0x60,
};

struct ECORE_API R_constant_load
{
    u16 index; // linear index (pixel)
    u16 cls; // element class

#ifdef USE_OGL
    GLuint location;
    GLuint program;

    R_constant_load() : index(u16(-1)), cls(u16(-1)), location(0), program(0) {};
#else
    R_constant_load() : index(u16(-1)), cls(u16(-1)) {};
#endif // USE_OGL

    BOOL equal(R_constant_load& C)
    {
#ifdef USE_OGL
        return (index == C.index) && (cls == C.cls) && (location == C.location) && (program == C.program);
#else
        return (index == C.index) && (cls == C.cls);
#endif // USE_OGL
    }
};

struct ECORE_API R_constant : public xr_resource
{
    shared_str name; // HLSL-name
    u16 type; // float=0/integer=1/boolean=2
    u32 destination; // pixel/vertex/(or both)/sampler

    R_constant_load ps;
    R_constant_load vs;
#ifndef USE_DX9
    R_constant_load gs;
#ifdef USE_DX11
    R_constant_load hs;
    R_constant_load ds;
    R_constant_load cs;
#endif
#endif // USE_DX10
    R_constant_load samp;
    R_constant_setup* handler;

    R_constant() : type(u16(-1)), destination(0), handler(nullptr){};

    R_constant_load& get_load(u32 destination)
    {
        static R_constant_load fake;
        switch (destination & 0xFF)
        {
        case RC_dest_vertex: return vs;
        case RC_dest_pixel: return ps;
#ifndef USE_DX9
        case RC_dest_geometry: return gs;
#ifdef USE_DX11
        case RC_dest_hull: return hs;
        case RC_dest_domain: return ds;
        case RC_dest_compute: return cs;
#endif
#endif
        default: FATAL("invalid enumeration for shader");
        }
        return fake;
    }

    BOOL equal(R_constant& C)
    {
        return (!xr_strcmp(name, C.name)) && (type == C.type) && (destination == C.destination) && ps.equal(C.ps) &&
            vs.equal(C.vs) && samp.equal(C.samp) && handler == C.handler;
    }

    BOOL equal(R_constant* C) { return equal(*C); }
};
typedef resptr_core<R_constant, resptr_base<R_constant>> ref_constant;

// Automatic constant setup
class ECORE_API R_constant_setup
{
public:
    virtual void setup(R_constant* C) = 0;
    virtual ~R_constant_setup() {}
};

class ECORE_API R_constant_table : public xr_resource_flagged
{
public:
    typedef xr_vector<ref_constant> c_table;
    c_table table;

#if defined(USE_DX10) || defined(USE_DX11)
    typedef std::pair<u32, ref_cbuffer> cb_table_record;
    typedef xr_vector<cb_table_record> cb_table;
    cb_table m_CBTable;
#endif // USE_DX10
private:
    void fatal(LPCSTR s);

#if defined(USE_DX10) || defined(USE_DX11)
    BOOL parseConstants(ID3DShaderReflectionConstantBuffer* pTable, u32 destination);
    BOOL parseResources(ID3DShaderReflection* pReflection, int ResNum, u32 destination);
#endif // USE_DX10

public:
    ~R_constant_table();

    void clear();
    BOOL parse(void* desc, u32 destination);
    void merge(R_constant_table* C);
    ref_constant get(LPCSTR name); // slow search
    ref_constant get(shared_str& name); // fast search

    BOOL equal(R_constant_table& C);
    BOOL equal(R_constant_table* C) { return equal(*C); }
    BOOL empty() { return 0 == table.size(); }
private:
};
typedef resptr_core<R_constant_table, resptr_base<R_constant_table>> ref_ctable;

#if defined(USE_DX10) || defined(USE_DX11)
#include "../xrRenderDX10/dx10ConstantBuffer_impl.h"
#endif // USE_DX10

#endif
