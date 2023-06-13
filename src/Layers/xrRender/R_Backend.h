#ifndef r_backendH
#define r_backendH
#pragma once

//#define RBackend_PGO

#ifdef RBackend_PGO
#define PGO(a) a
#else
#define PGO(a)
#endif

#include "Layers/xrRender/BufferUtils.h"
#include "R_DStreams.h"
#include "r_constants_cache.h"
#include "R_Backend_xform.h"
#include "R_Backend_hemi.h"
#include "R_Backend_tree.h"

#ifdef USE_DX11
#include "Layers/xrRenderPC_R4/r_backend_lod.h"
#include "Layers/xrRenderDX11/StateManager/dx11StateManager.h"
#include "Layers/xrRenderDX11/StateManager/dx11ShaderResourceStateCache.h"
#include "Layers/xrRenderDX11/StateManager/dx11StateCache.h"
#endif

#include "FVF.h"

const u32 CULL_CCW = D3DCULL_CCW;
const u32 CULL_CW = D3DCULL_CW;
const u32 CULL_NONE = D3DCULL_NONE;

///		detailed statistic
struct R_statistics_element
{
    u32 verts, dips;
    ICF void add(u32 _verts)
    {
        verts += _verts;
        dips++;
    }
};

struct R_statistics
{
    R_statistics_element s_static;
    R_statistics_element s_flora;
    R_statistics_element s_flora_lods;
    R_statistics_element s_details;
    R_statistics_element s_ui;
    R_statistics_element s_dynamic;
    R_statistics_element s_dynamic_sw;
    R_statistics_element s_dynamic_inst;
    R_statistics_element s_dynamic_1B;
    R_statistics_element s_dynamic_2B;
    R_statistics_element s_dynamic_3B;
    R_statistics_element s_dynamic_4B;
};

#pragma warning(push)
#pragma warning(disable:4324)
class ECORE_API CBackend
{
public:
    enum
    {
        MaxCBuffers = 14
    };

public:
    R_xforms xforms;
    R_hemi hemi;
    R_tree tree;
#ifdef USE_DX11
    R_LOD LOD;
#endif

#if defined(USE_DX11)
    ref_cbuffer m_aVertexConstants[MaxCBuffers];
    ref_cbuffer m_aPixelConstants[MaxCBuffers];

    ref_cbuffer m_aGeometryConstants[MaxCBuffers];
    ref_cbuffer m_aComputeConstants[MaxCBuffers];

    ref_cbuffer m_aHullConstants[MaxCBuffers];
    ref_cbuffer m_aDomainConstants[MaxCBuffers];

    D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
    ID3DInputLayout* m_pInputLayout;
    u32 dummy0; // Padding to avoid warning
    u32 dummy1; // Padding to avoid warning
    u32 dummy2; // Padding to avoid warning
#endif
private:
    // Render-targets
#if defined(USE_DX9) || defined (USE_DX11)
    ID3DRenderTargetView* pRT[4];
    ID3DDepthStencilView* pZB;
#elif defined(USE_OGL)
    GLuint pFB;
    GLuint pRT[4];
    GLuint pZB;
#else
#   error No graphics API selected or enabled!
#endif

    // Vertices/Indices/etc
    SDeclaration* decl;
    VertexBufferHandle vb;
    IndexBufferHandle ib;
    u32 vb_stride;

    // Pixel/Vertex constants
    alignas(16) R_constants constants;
    R_constant_table* ctable;

    // Shaders/State
    ID3DState* state;
#if defined(USE_DX9)
    ID3DPixelShader* ps;
    ID3DVertexShader* vs;
#elif defined(USE_DX11)
    ID3DPixelShader* ps;
    ID3DVertexShader* vs;
    ID3DGeometryShader* gs;
    ID3D11HullShader* hs;
    ID3D11DomainShader* ds;
    ID3D11ComputeShader* cs;
#elif defined(USE_OGL)
    GLuint ps;
    GLuint vs;
    GLuint gs;
    GLuint pp;
#else
#   error No graphics API selected or enabled!
#endif

#ifdef DEBUG
    LPCSTR ps_name;
    LPCSTR vs_name;
#ifndef USE_DX9
    LPCSTR gs_name;
#if defined(USE_DX11)
    LPCSTR hs_name;
    LPCSTR ds_name;
    LPCSTR cs_name;
#endif // USE_DX11
#endif // !USE_DX9
#   ifdef USE_OGL
    pcstr pp_name;
#   endif
#endif // DEBUG

    u32 stencil_enable;
    u32 stencil_func;
    u32 stencil_ref;
    u32 stencil_mask;
    u32 stencil_writemask;
    u32 stencil_fail;
    u32 stencil_pass;
    u32 stencil_zfail;
    u32 colorwrite_mask;
    u32 fill_mode;
    u32 cull_mode;
    u32 z_enable;
    u32 z_func;
    u32 alpha_ref;

    // Lists
    STextureList* T;
    SMatrixList* M;
    SConstantList* C;

    // Lists-expanded
    CTexture* textures_ps[CTexture::mtMaxPixelShaderTextures]; // stages
    //CTexture* textures_vs[5]; // dmap + 4 vs
    CTexture* textures_vs[CTexture::mtMaxVertexShaderTextures]; // 4 vs
#ifndef USE_DX9
    CTexture* textures_gs[CTexture::mtMaxGeometryShaderTextures]; // 4 vs
#   if defined(USE_DX11)
    CTexture* textures_hs[CTexture::mtMaxHullShaderTextures]; // 4 vs
    CTexture* textures_ds[CTexture::mtMaxDomainShaderTextures]; // 4 vs
    CTexture* textures_cs[CTexture::mtMaxComputeShaderTextures]; // 4 vs
#   endif
#elif defined(USE_DX9)
    // Nothing, because DX9 only has VS and PS
#else
#   error No graphics API selected or enabled!
#endif

    CMatrix* matrices[8]{}; // matrices are supported only for FFP

public:
    struct _stats
    {
        struct
        {
            u32 calls;
            u32 verts;
            u32 polys;
        } render;
        struct
        {
            u32 calls;
            u32 groups_x;
            u32 groups_y;
            u32 groups_z;
        } compute;
        u32 vs;
        u32 ps;
        u32 gs;
        u32 hs;
        u32 ds;
        u32 cs;
        u32 pp;
        u32 decl;
        u32 vb;
        u32 ib;
        u32 states; // Number of times the shader-state changes
        u32 textures; // Number of times the shader-tex changes
        u32 matrices; // Number of times the shader-xform changes
        u32 constants; // Number of times the shader-consts changes
        u32 xforms;
        u32 target_rt;
        u32 target_zb;

        R_statistics r;
    } stat;

    u32 context_id{ CHW::IMM_CTX_ID };

public:
    CTexture* get_ActiveTexture(u32 stage)
    {
        if (stage < CTexture::rstVertex)
            return textures_ps[stage];

        if (stage < CTexture::rstGeometry)
            return textures_vs[stage - CTexture::rstVertex];
#if defined(USE_DX11)
        if (stage < CTexture::rstHull)
            return textures_gs[stage - CTexture::rstGeometry];

        if (stage < CTexture::rstDomain)
            return textures_hs[stage - CTexture::rstHull];

        if (stage < CTexture::rstCompute)
            return textures_ds[stage - CTexture::rstDomain];

        if (stage < CTexture::rstInvalid)
            return textures_cs[stage - CTexture::rstCompute];
#endif // USE_DX11
        VERIFY(!"Invalid texture stage");
        return nullptr;
    }

    float o_hemi;
    float o_hemi_cube[/*CROS_impl::NUM_FACES*/6];
    float o_sun;

    void apply_lmaterial();

#if defined(USE_DX9)
    R_constant_array& get_ConstantCache_Vertex() { return constants.a_vertex; }
    R_constant_array& get_ConstantCache_Pixel() { return constants.a_pixel; }
#elif defined(USE_DX11)
    IC void get_ConstantDirect(const shared_str& n, size_t DataSize, void** pVData, void** pGData, void** pPData);
#endif

    void Invalidate();

    // API
    IC void set_xform(u32 ID, const Fmatrix& M);
    IC void set_xform_world(const Fmatrix& M);
    IC void set_xform_view(const Fmatrix& M);
    IC void set_xform_project(const Fmatrix& M);
    IC const Fmatrix& get_xform_world();
    IC const Fmatrix& get_xform_view();
    IC const Fmatrix& get_xform_project();

    u32 curr_rt_width{};
    u32 curr_rt_height{};

    IC void set_pass_targets(const ref_rt& mrt0, const ref_rt& mrt1, const ref_rt& mrt2, const ref_rt& zb);

#if defined(USE_DX9) || defined(USE_DX11)
    IC void set_RT(ID3DRenderTargetView* RT, u32 ID = 0);
    IC void set_ZB(ID3DDepthStencilView* ZB);
    IC ID3DRenderTargetView* get_RT(u32 ID = 0);
    IC ID3DDepthStencilView* get_ZB();
#elif defined(USE_OGL)
    IC void set_FB(GLuint FB = 0);
    IC void set_RT(GLuint RT, u32 ID = 0);
    IC void set_ZB(GLuint ZB);
    IC GLuint get_FB();
    IC GLuint get_RT(u32 ID = 0);
    IC GLuint get_ZB();
#else
#   error No graphics API selected or enabled!
#endif

#if defined(USE_DX9) || defined(USE_DX11)
    IC void ClearRT(ID3DRenderTargetView* rt, const Fcolor& color);

    IC void ClearZB(ID3DDepthStencilView* zb, float depth);
    IC void ClearZB(ID3DDepthStencilView* zb, float depth, u8 stencil);

    IC bool ClearRTRect(ID3DRenderTargetView* rt, const Fcolor& color, size_t numRects, const Irect* rects);
    IC bool ClearZBRect(ID3DDepthStencilView* zb, float depth, size_t numRects, const Irect* rects);
#elif defined(USE_OGL)
    IC void ClearRT(GLuint rt, const Fcolor& color);

    IC void ClearZB(GLuint zb, float depth);
    IC void ClearZB(GLuint zb, float depth, u8 stencil);

    IC bool ClearRTRect(GLuint rt, const Fcolor& color, size_t numRects, const Irect* rects);
    IC bool ClearZBRect(GLuint zb, float depth, size_t numRects, const Irect* rects);
#else
#   error No graphics API selected or enabled!
#endif

    ICF void ClearRT(ref_rt& rt, const Fcolor& color) { ClearRT(rt->pRT, color); }
    ICF bool ClearRTRect(ref_rt& rt, const Fcolor& color, size_t numRects, const Irect* rects)
    {
        return ClearRTRect(rt->pRT, color, numRects, rects);
    }

#if defined(USE_DX9) || defined(USE_OGL)
    ICF void ClearZB(ref_rt& zb, float depth) { ClearZB(zb->pRT, depth);}
    ICF void ClearZB(ref_rt& zb, float depth, u8 stencil) { ClearZB(zb->pRT, depth, stencil);}
    ICF bool ClearZBRect(ref_rt& zb, float depth, size_t numRects, const Irect* rects)
    {
        return ClearZBRect(zb->pRT, depth, numRects, rects);
    }
#elif defined(USE_DX11)
    ICF void ClearZB(ref_rt& zb, float depth) { ClearZB(zb->pZRT[context_id], depth); }
    ICF void ClearZB(ref_rt& zb, float depth, u8 stencil) { ClearZB(zb->pZRT[context_id], depth, stencil); }
    ICF bool ClearZBRect(ref_rt& zb, float depth, size_t numRects, const Irect* rects)
    {
        return ClearZBRect(zb->pZRT[context_id], depth, numRects, rects);
    }
#else
#   error No graphics API selected or enabled!
#endif

    IC void set_Constants(R_constant_table* C);
    void set_Constants(ref_ctable& C) { set_Constants(&*C); }

    void set_Textures(STextureList* T);
    void set_Textures(ref_texture_list& T) { set_Textures(&*T); }

    IC void set_Matrices(SMatrixList* M);
    IC void set_Matrices(ref_matrix_list& M) { set_Matrices(&*M); }

    IC void set_Pass(SPass* P);
    void set_Pass(ref_pass& P) { set_Pass(&*P); }

    ICF void set_Element(ShaderElement* S, u32 pass = 0);
    void set_Element(ref_selement& S, u32 pass = 0) { set_Element(&*S, pass); }

    ICF void set_Shader(Shader* S, u32 pass = 0);
    void set_Shader(ref_shader& S, u32 pass = 0) { set_Shader(&*S, pass); }

    ICF void set_States(SState* _state);
    ICF void set_States(ref_state& _state) { set_States(&*_state); }

    ICF void set_Format(SDeclaration* _decl);

private:
#if defined(USE_DX9) || defined(USE_DX11)
    ICF void set_PS(ID3DPixelShader* _ps, LPCSTR _n = nullptr);
#elif defined(USE_OGL)
    ICF void set_PS(GLuint _ps, LPCSTR _n = 0);
#else
#   error No graphics API selected or enabled!
#endif

    ICF void set_PS(ref_ps& _ps) { set_PS(_ps->sh, _ps->cName.c_str()); }

#ifndef USE_DX9
    ICF void set_GS(ref_gs& _gs) { set_GS(_gs->sh, _gs->cName.c_str()); }

#   if defined(USE_DX11)
    ICF void set_GS(ID3DGeometryShader* _gs, LPCSTR _n = nullptr);

    ICF void set_HS(ID3D11HullShader* _hs, LPCSTR _n = nullptr);
    ICF void set_HS(ref_hs& _hs) { set_HS(_hs->sh, _hs->cName.c_str()); }

    ICF void set_DS(ID3D11DomainShader* _ds, LPCSTR _n = nullptr);
    ICF void set_DS(ref_ds& _ds) { set_DS(_ds->sh, _ds->cName.c_str()); }
#   elif defined(USE_OGL)
    ICF void set_GS(GLuint _gs, LPCSTR _n = 0);

    ICF void set_PP(GLuint _pp, pcstr _n = nullptr);
    ICF void set_PP(ref_pp& _pp) { set_PP(_pp->pp, _pp->cName.c_str()); }
#   endif
#endif // USE_DX9

    ICF void set_VS(ref_vs& _vs);

#if defined(USE_DX11)
    ICF void set_VS(SVS* _vs);
#endif

#if defined(USE_DX9) || defined(USE_DX11)
    ICF void set_VS(ID3DVertexShader* _vs, LPCSTR _n = nullptr);
#elif defined(USE_OGL)
    ICF void set_VS(GLuint _vs, LPCSTR _n = 0);
#else
#   error No graphics API selected or enabled!
#endif

public:
#if defined(USE_DX11)
    ICF void set_CS(ID3D11ComputeShader* _cs, LPCSTR _n = nullptr);
    ICF void set_CS(ref_cs& _cs) { set_CS(_cs->sh, _cs->cName.c_str()); }
    ICF void Compute(u32 ThreadGroupCountX, u32 ThreadGroupCountY, u32 ThreadGroupCountZ);
#endif

public:
#if defined(USE_DX9) || defined(USE_OGL)
    ICF bool is_TessEnabled() { return false; }
#elif defined(USE_DX11)
    ICF bool is_TessEnabled();
#else
#   error No graphics API selected or enabled!
#endif

    ICF void set_Vertices(VertexBufferHandle _vb, u32 _vb_stride);
    ICF void set_Indices(IndexBufferHandle _ib);
    ICF void set_Geometry(SGeometry* _geom);
    ICF void set_Geometry(ref_geom& _geom) { set_Geometry(&*_geom); }
    IC void set_Stencil(u32 _enable, u32 _func = D3DCMP_ALWAYS, u32 _ref = 0x00, u32 _mask = 0x00,
                        u32 _writemask = 0x00, u32 _fail = D3DSTENCILOP_KEEP, u32 _pass = D3DSTENCILOP_KEEP,
                        u32 _zfail = D3DSTENCILOP_KEEP);
    IC void set_Z(u32 _enable);
    IC void set_ZFunc(u32 _func);
    IC void set_AlphaRef(u32 _value);
    IC void set_ColorWriteEnable(
        u32 _mask = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE |
            D3DCOLORWRITEENABLE_ALPHA);
    IC void set_CullMode(u32 _mode);
    u32 get_CullMode() { return cull_mode; }
    IC void set_FillMode(u32 _mode);
    void set_ClipPlanes(u32 _enable, Fplane* _planes = nullptr, u32 count = 0);
    void set_ClipPlanes(u32 _enable, Fmatrix* _xform = nullptr, u32 fmask = 0xff);
    IC void set_Scissor(Irect* rect = nullptr);
    IC void SetViewport(const D3D_VIEWPORT& viewport) const;

    IC void SetTextureFactor(u32 factor) const;
    IC void SetAmbient(u32 ambient) const;

    // constants
    ICF ref_constant get_c(LPCSTR n)
    {
        if (ctable)
            return ctable->get(n);
        return nullptr;
    }

    ICF ref_constant get_c(const shared_str& n)
    {
        if (ctable)
            return ctable->get(n);
        return nullptr;
    }

    // constants - direct (fast)
    template<typename... Args>
    ICF void set_c(R_constant* C, Args&&... args)
    {
        if (!C)
            return;
#ifdef USE_OGL
        if (!HW.SeparateShaderObjectsSupported)
            VERIFY(C->pp.program == pp);
#endif
        constants.set(C, std::forward<Args>(args)...);
    }

    template<typename... Args>
    ICF void set_ca(R_constant* C, Args&&... args)
    {
        if (!C)
            return;
#ifdef USE_OGL
        if (!HW.SeparateShaderObjectsSupported)
            VERIFY(C->pp.program == pp);
#endif
        constants.seta(C, std::forward<Args>(args)...);
    }

    // constants - raw string (slow)
    template<typename... Args>
    ICF void set_c(cpcstr name, Args&&... args)
    {
        if (!ctable)
            return;
        set_c(ctable->get(name)._get(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    ICF void set_ca(cpcstr name, Args&&... args)
    {
        if (!ctable)
            return;
        set_ca(ctable->get(name)._get(), std::forward<Args>(args)...);
    }

    // constants - shared_str (average)
    template<typename... Args>
    ICF void set_c(const shared_str& name, Args&& ... args)
    {
        if (!ctable)
            return;
        set_c(ctable->get(name)._get(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    ICF void set_ca(const shared_str& name, Args&& ... args)
    {
        if (!ctable)
            return;
        set_ca(ctable->get(name)._get(), std::forward<Args>(args)...);
    }

    // Rendering
    ICF void Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
    ICF void Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC);

    ICF void render_instanced(D3DPRIMITIVETYPE prim_type, u32 base_vertex, u32 start_index, u32 num_indices, u32 instance_count);

    ICF void submit()
    {
#ifdef USE_DX11
        VERIFY(context_id != CHW::IMM_CTX_ID);
        ID3D11CommandList* pCommandList{ nullptr };
        CHK_DX(HW.get_context(context_id)->FinishCommandList(false, &pCommandList));
        HW.get_context(CHW::IMM_CTX_ID)->ExecuteCommandList(pCommandList, false);
        _RELEASE(pCommandList);
#endif
    }

    void gpu_mark_begin(const wchar_t* name);
    void gpu_mark_end();

    // Device create / destroy / frame signaling
    void OnFrameBegin();
    void OnFrameEnd();
    void OnDeviceCreate();
    void OnDeviceDestroy();
    void SetupStates();

    // Debug render
    void dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc);
    void dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC);
    void dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2);
    void dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value);
#ifdef DEBUG
    void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt);
    void dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt);

    void dbg_DrawAABB(Fvector& T, float sx, float sy, float sz, u32 C)
    {
        Fvector half_dim;
        half_dim.set(sx, sy, sz);
        Fmatrix TM;
        TM.translate(T);
        dbg_DrawOBB(TM, half_dim, C);
    }

    void dbg_DrawOBB(Fmatrix& T, Fvector& half_dim, u32 C);
    void dbg_DrawTRI(Fmatrix& T, Fvector* p, u32 C) { dbg_DrawTRI(T, p[0], p[1], p[2], C); }
    void dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C);
    void dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C);
    void dbg_DrawEllipse(Fmatrix& T, u32 C);
#endif
    void dbg_OverdrawBegin();
    void dbg_OverdrawEnd();

    CBackend()
        : xforms(*this)
        , tree(*this)
        , hemi(*this)
#if defined(USE_DX11)
        , LOD(*this)
        , constants(*this)
        , StateManager(*this)
#endif
    {
        Invalidate();
    }

private:
    // Debug Draw
    void InitializeDebugDraw();
    void DestroyDebugDraw();

    // DX9 doesn't need this
#ifndef USE_DX9
    ref_geom vs_L;
    ref_geom vs_TL;
#endif

#if defined(USE_DX11)
private:
    //	DirectX 11+ internal functionality
    // void CreateConstantBuffers();
    // void DestroyConstantBuffers();
    void ApplyVertexLayout();
    void ApplyRTandZB();
    void ApplyPrimitieTopology(D3D_PRIMITIVE_TOPOLOGY Topology);
    bool CBuffersNeedUpdate(ref_cbuffer buf1[MaxCBuffers], ref_cbuffer buf2[MaxCBuffers], u32& uiMin, u32& uiMax);

private:
    ID3DBlob* m_pInputSignature{ nullptr };
    ID3DUserDefinedAnnotation* pAnnotation{ nullptr };

    bool m_bChangedRTorZB;

public:
    dx11StateManager StateManager;
    dx11ShaderResourceStateCache SRVSManager;
#endif // USE_DX11
};
#pragma warning(pop)

#define RCache RImplementation.get_imm_context().cmd_list

#endif
