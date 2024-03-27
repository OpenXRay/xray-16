// TextureManager.h: interface for the CTextureManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ResourceManagerH
#define ResourceManagerH
#pragma once

#include "Shader.h"
#include "tss_def.h"
#include "TextureDescrManager.h"
#include "xrScriptEngine/script_engine.hpp"
// refs
struct lua_State;

class dx11ConstantBuffer;

// defs
class ECORE_API CResourceManager
{
private:
    struct str_pred
    {
        bool operator()(LPCSTR x, LPCSTR y) const { return xr_strcmp(x, y) < 0; }
    };
    struct texture_detail
    {
        const char* T;
        R_constant_setup* cs;
    };

public:
    using map_Blender = xr_map<const char*, IBlender*, str_pred>;
    using map_Texture = xr_map<const char*, CTexture*, str_pred>;
    using map_Matrix = xr_map<const char*, CMatrix*, str_pred>;
    using map_Constant = xr_map<const char*, CConstant*, str_pred>;
    using map_RT = xr_map<const char*, CRT*, str_pred>;
    //	DX10 cut DEFINE_MAP_PRED(const char*,CRTC*,			map_RTC,		map_RTCIt,			str_pred);
    using map_VS = xr_map<const char*, SVS*, str_pred>;

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    using map_GS = xr_map<const char*, SGS*, str_pred>;
#endif

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    using map_HS = xr_map<const char*, SHS*, str_pred>;
    using map_DS = xr_map<const char*, SDS*, str_pred>;
    using map_CS = xr_map<const char*, SCS*, str_pred>;
#endif
#if defined(USE_OGL)
    using map_PP = xr_map<const char*, SPP*, str_pred>;
#endif

    using map_PS = xr_map<const char*, SPS*, str_pred>;
    using map_TD = xr_map<const char*, texture_detail, str_pred>;

private:
    // data
    map_Blender m_blenders;
    map_Texture m_textures;
    map_Matrix m_matrices;
    map_Constant m_constants;
    map_RT m_rtargets;
    //	DX10 cut map_RTC												m_rtargets_c;
    map_VS m_vs;
    map_PS m_ps;

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    map_GS m_gs;
#endif

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    map_DS m_ds;
    map_HS m_hs;
    map_CS m_cs;
#endif
#if defined(USE_OGL)
    map_PP m_pp;
#endif

    map_TD m_td;

    xr_vector<SState*> v_states;
    xr_vector<SDeclaration*> v_declarations;
    xr_vector<SGeometry*> v_geoms;
    xr_vector<R_constant_table*> v_constant_tables;

#if defined(USE_DX11) || defined(USE_DX12)
    xr_vector<dx11ConstantBuffer*> v_constant_buffer[R__NUM_CONTEXTS];
    xr_vector<SInputSignature*> v_input_signature;
#endif

    // lists
    xr_vector<STextureList*> lst_textures;
    xr_vector<SMatrixList*> lst_matrices;
    xr_vector<SConstantList*> lst_constants;

    // main shader-array
    xr_vector<SPass*> v_passes;
    xr_vector<ShaderElement*> v_elements;
    xr_vector<Shader*> v_shaders;
    Lock v_shaders_lock;

    xr_vector<ref_texture> m_necessary;
    // misc
public:
    CTextureDescrMngr m_textures_description;
    //.	CInifile*											m_textures_description;
    xr_vector<std::pair<shared_str, R_constant_setup*>> v_constant_setup;
    BOOL bDeferredLoad;
    bool m_shader_fallback_allowed;
    CScriptEngine ScriptEngine;
    Lock ScriptEngineLock;

private:
    void LS_Load();
    void LS_Unload();

public:

    // Miscelaneous
    void _ParseList(sh_list& dest, LPCSTR names);
    IBlender* _GetBlender(LPCSTR Name);
    IBlender* _FindBlender(LPCSTR Name);
    void _GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps);
    void _DumpMemoryUsage();
    //.	BOOL							_GetDetailTexture	(LPCSTR Name, LPCSTR& T, R_constant_setup* &M);

    map_Blender& _GetBlenders() { return m_blenders; }
    // Debug
    void DBG_VerifyGeoms();
    void DBG_VerifyTextures();

    // Editor cooperation
    void ED_UpdateBlender(LPCSTR Name, IBlender* data);
    void ED_UpdateMatrix(LPCSTR Name, CMatrix* data);
    void ED_UpdateConstant(LPCSTR Name, CConstant* data);
#ifdef _EDITOR
    void ED_UpdateTextures(AStringVec* names);
#endif

    // Low level resource creation
    CTexture* _CreateTexture(LPCSTR Name);
    void _DeleteTexture(const CTexture* T);

    CMatrix* _CreateMatrix(LPCSTR Name);
    void _DeleteMatrix(const CMatrix* M);

    CConstant* _CreateConstant(LPCSTR Name);
    void _DeleteConstant(const CConstant* C);

    R_constant_table* _CreateConstantTable(R_constant_table& C);
    void _DeleteConstantTable(const R_constant_table* C);

#if defined(USE_DX11) || defined(USE_DX12)
    dx11ConstantBuffer* _CreateConstantBuffer(u32 context_id, ID3DShaderReflectionConstantBuffer* pTable);
    void _DeleteConstantBuffer(u32 context_id, const dx11ConstantBuffer* pBuffer);

    SInputSignature* _CreateInputSignature(ID3DBlob* pBlob);
    void _DeleteInputSignature(const SInputSignature* pSignature);
#endif

#if defined(USE_DX12)
    CRT* _CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, u32 slices_num = 1, Flags32 flags = {}, Fcolor color = {0.0f, 0.0f, 0.0f, 0.0f});
#else 
    CRT* _CreateRT(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, u32 slices_num = 1, Flags32 flags = {});
#endif 
    void _DeleteRT(const CRT* RT);

//	DX10 cut CRTC*							_CreateRTC			(LPCSTR Name, u32 size,	D3DFORMAT f);
//	DX10 cut void							_DeleteRTC			(const CRTC*	RT	);

#if defined(USE_OGL)
    SPP* _CreatePP(pcstr vs, pcstr ps, pcstr gs, pcstr hs, pcstr ds);
    bool _LinkPP(SPass& pass);
    void _DeletePP(const SPP* p);
#endif

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    SGS* _CreateGS(LPCSTR Name);
    void _DeleteGS(const SGS* GS);
#endif

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    SHS* _CreateHS(LPCSTR Name);
    void _DeleteHS(const SHS* HS);

    SDS* _CreateDS(LPCSTR Name);
    void _DeleteDS(const SDS* DS);

    SCS* _CreateCS(LPCSTR Name);
    void _DeleteCS(const SCS* CS);
#endif

    SPS* _CreatePS(LPCSTR Name);
    void _DeletePS(const SPS* PS);

    SVS* _CreateVS(cpcstr shader, u32 flags = 0);
    void _DeleteVS(const SVS* VS);

    SPass* _CreatePass(const SPass& proto);
    void _DeletePass(const SPass* P);

    // Shader compiling / optimizing
    SState* _CreateState(SimulatorStates& Code);
    void _DeleteState(const SState* SB);

    SDeclaration* _CreateDecl(const VertexElement* dcl);
    void _DeleteDecl(const SDeclaration* dcl);

    STextureList* _CreateTextureList(STextureList& L);
    void _DeleteTextureList(const STextureList* L);

    SMatrixList* _CreateMatrixList(SMatrixList& L);
    void _DeleteMatrixList(const SMatrixList* L);

    SConstantList* _CreateConstantList(SConstantList& L);
    void _DeleteConstantList(const SConstantList* L);

    ShaderElement* _CreateElement(ShaderElement&& L);
    void _DeleteElement(const ShaderElement* L);

    Shader* _cpp_Create(LPCSTR s_shader, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    Shader* _cpp_Create(
        IBlender* B, LPCSTR s_shader = nullptr, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    Shader* _lua_Create(LPCSTR s_shader, LPCSTR s_textures);
    BOOL _lua_HasShader(LPCSTR s_shader);

    CResourceManager() : bDeferredLoad(TRUE)
    {
#if RENDER == R_R1 || RENDER == R_R2
        m_shader_fallback_allowed = !!strstr(Core.Params, "-lack_of_shaders");
#else // For another renderers we should always allow fallback
        m_shader_fallback_allowed = true;
#endif
    }

    ~CResourceManager();

    void OnDeviceCreate(IReader* F);
    void OnDeviceCreate(LPCSTR name);
    void OnDeviceDestroy(BOOL bKeepTextures);

    void reset_begin();
    void reset_end();

    void CompatibilityCheck();

    // Creation/Destroying
    Shader* Create(LPCSTR s_shader = nullptr, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    Shader* Create(
        IBlender* B, LPCSTR s_shader = nullptr, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    void Delete(const Shader* S);
    void RegisterConstantSetup(LPCSTR name, R_constant_setup* s)
    {
        v_constant_setup.emplace_back(shared_str(name), s);
    }

    SGeometry* CreateGeom(const VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib);
    SGeometry* CreateGeom(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib);

    void DeleteGeom(const SGeometry* VS);
    void DeferredLoad(BOOL E) { bDeferredLoad = E; }
    void DeferredUpload();
    void DeferredUnload();
    void Evict();
    void StoreNecessaryTextures();
    void DestroyNecessaryTextures();
    void Dump(bool bBrief);

private:
    template <typename T>
    T& GetShaderMap();

    template <typename T>
    T* CreateShader(cpcstr name, pcstr filename = nullptr, u32 flags = 0);

    template <typename T>
    bool DestroyShader(const T* sh);

    template <typename T>
    bool reclaim(xr_vector<T*>& vec, const T* ptr)
    {
        auto it = vec.begin();
        const auto end = vec.cend();

        for (; it != end; ++it)
        {
            if (*it == ptr)
            {
                vec.erase(it);
                return true;
            }
        }
        return false;
    }
};

#endif // ResourceManagerH
