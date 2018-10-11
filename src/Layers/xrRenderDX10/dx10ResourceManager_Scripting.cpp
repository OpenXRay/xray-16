#include "stdafx.h"

#include "xrEngine/Render.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRender/blenders/blender.h"
#include "Layers/xrRender/blenders/blender_recorder.h"
//	adopt_compiler don't have = operator And it can't have = operator
#include "xrScriptEngine/script_engine.hpp"
#include "luabind/return_reference_to_policy.hpp"

using namespace luabind;
using namespace luabind::policy;

#ifdef DEBUG
#define MDB Memory.dbg_check()
#else
#define MDB
#endif

class adopt_dx10options
{
public:
    bool _dx10_msaa_alphatest_atoc()
    {
        return RImplementation.o.dx10_msaa_alphatest == CRender::MSAA_ATEST_DX10_0_ATOC;
    }
};

// wrapper
class adopt_dx10sampler
{
    CBlender_Compile* m_pC;
    u32 m_SI; // Sampler index

public:
    adopt_dx10sampler(CBlender_Compile* C, u32 SamplerIndex) : m_pC(C), m_SI(SamplerIndex)
    {
        if (u32(-1) == m_SI)
            m_pC = 0;
    }

    adopt_dx10sampler(const adopt_dx10sampler& _C) : m_pC(_C.m_pC), m_SI(_C.m_SI)
    {
        if (u32(-1) == m_SI)
            m_pC = 0;
    }

    //	adopt_sampler&			_texture		(LPCSTR texture)		{ if (C) C->i_Texture	(stage,texture);
    // return *this;	}
    //	adopt_sampler&			_projective		(bool _b)				{ if (C) C->i_Projective(stage,_b);
    // return *this;	}
    //	adopt_sampler&			_clamp			()						{ if (C) C->i_Address
    //(stage,D3DTADDRESS_CLAMP);
    // return *this;	}
    //	adopt_sampler&			_wrap			()						{ if (C) C->i_Address
    //(stage,D3DTADDRESS_WRAP);
    // return *this;	}
    //	adopt_sampler&			_mirror			()						{ if (C) C->i_Address
    //(stage,D3DTADDRESS_MIRROR);
    // return *this;	}
    //	adopt_sampler&			_f_anisotropic	()						{ if (C) C->i_Filter
    //(stage,D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,D3DTEXF_ANISOTROPIC);	return *this;	}
    //	adopt_sampler&			_f_trilinear	()						{ if (C) C->i_Filter
    //(stage,D3DTEXF_LINEAR,D3DTEXF_LINEAR,D3DTEXF_LINEAR);		return *this;	}
    //	adopt_sampler&			_f_bilinear		()						{ if (C) C->i_Filter
    //(stage,D3DTEXF_LINEAR,D3DTEXF_POINT,
    // D3DTEXF_LINEAR);		return *this;	}
    //	adopt_sampler&			_f_linear		()						{ if (C) C->i_Filter
    //(stage,D3DTEXF_LINEAR,D3DTEXF_NONE,
    // D3DTEXF_LINEAR);		return *this;	}
    //	adopt_sampler&			_f_none			()						{ if (C) C->i_Filter	(stage,D3DTEXF_POINT,
    // D3DTEXF_NONE,
    // D3DTEXF_POINT);		return *this;	}
    //	adopt_sampler&			_fmin_none		()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_NONE);
    // return *this;	}
    //	adopt_sampler&			_fmin_point		()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_POINT);
    // return *this;	}
    //	adopt_sampler&			_fmin_linear	()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_LINEAR);
    // return *this;	}
    //	adopt_sampler&			_fmin_aniso		()						{ if (C)
    // C->i_Filter_Min(stage,D3DTEXF_ANISOTROPIC);
    // return *this;	}
    //	adopt_sampler&			_fmip_none		()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_NONE);
    // return *this;	}
    //	adopt_sampler&			_fmip_point		()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_POINT);
    // return *this;	}
    //	adopt_sampler&			_fmip_linear	()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_LINEAR);
    // return *this;	}
    //	adopt_sampler&			_fmag_none		()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_NONE);
    // return *this;	}
    //	adopt_sampler&			_fmag_point		()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_POINT);
    // return *this;	}
    //	adopt_sampler&			_fmag_linear	()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_LINEAR);
    // return *this;	}
};
/*
class	adopt_dx10texture
{
    CBlender_Compile*		m_pC;
    u32						m_TI;	//	Sampler index
public:
    adopt_dx10texture	(CBlender_Compile*	C, u32 TextureIndex)	: m_pC(C), m_TI(TextureIndex)		{ if
(u32(-1)==m_TI) m_pC=0;}
    adopt_dx10texture	(const adopt_dx10texture&	_C)				: m_pC(_C.m_pC), m_TI(_C.m_TI)	{ if
(u32(-1)==m_TI) m_pC=0;}
};
*/

#pragma warning(push)
#pragma warning(disable : 4512)
// wrapper
class adopt_compiler
{
    CBlender_Compile* C;
    bool& m_bFirstPass;

    void TryEndPass()
    {
        if (!m_bFirstPass)
            C->r_End();
        m_bFirstPass = false;
    }

public:
    adopt_compiler(CBlender_Compile* _C, bool& bFirstPass) : C(_C), m_bFirstPass(bFirstPass) { m_bFirstPass = true; }
    adopt_compiler(const adopt_compiler& _C) : C(_C.C), m_bFirstPass(_C.m_bFirstPass) {}
    adopt_compiler& _options(int P, bool S)
    {
        C->SetParams(P, S);
        return *this;
    }
    adopt_compiler& _o_emissive(bool E)
    {
        C->SH->flags.bEmissive = E;
        return *this;
    }
    adopt_compiler& _o_distort(bool E)
    {
        C->SH->flags.bDistort = E;
        return *this;
    }
    adopt_compiler& _o_wmark(bool E)
    {
        C->SH->flags.bWmark = E;
        return *this;
    }
    adopt_compiler& _pass(LPCSTR vs, LPCSTR ps)
    {
        TryEndPass();
        C->r_Pass(vs, ps, true);
        return *this;
    }
    adopt_compiler& _passgs(LPCSTR vs, LPCSTR gs, LPCSTR ps)
    {
        TryEndPass();
        C->r_Pass(vs, gs, ps, true);
        return *this;
    }
    adopt_compiler& _fog(bool _fog)
    {
        C->PassSET_LightFog(FALSE, _fog);
        return *this;
    }
    adopt_compiler& _ZB(bool _test, bool _write)
    {
        C->PassSET_ZB(_test, _write);
        return *this;
    }
    adopt_compiler& _blend(bool _blend, u32 abSRC, u32 abDST)
    {
        C->PassSET_ablend_mode(_blend, abSRC, abDST);
        return *this;
    }
    adopt_compiler& _aref(bool _aref, u32 aref)
    {
        C->PassSET_ablend_aref(_aref, aref);
        return *this;
    }
    adopt_compiler& _dx10texture(LPCSTR _resname, LPCSTR _texname)
    {
        C->r_dx10Texture(_resname, _texname);
        return *this;
    }
    adopt_dx10sampler _dx10sampler(LPCSTR _name)
    {
        u32 s = C->r_dx10Sampler(_name);
        return adopt_dx10sampler(C, s);
    }

    //	DX10 specific
    adopt_compiler& _dx10color_write_enable(bool cR, bool cG, bool cB, bool cA)
    {
        C->r_ColorWriteEnable(cR, cG, cB, cA);
        return *this;
    }
    adopt_compiler& _dx10Stencil(bool Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
    {
        C->r_Stencil(Enable, Func, Mask, WriteMask, Fail, Pass, ZFail);
        return *this;
    }
    adopt_compiler& _dx10StencilRef(u32 Ref)
    {
        C->r_StencilRef(Ref);
        return *this;
    }
    adopt_compiler& _dx10ATOC(bool Enable)
    {
        C->RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, Enable);
        return *this;
    }
    adopt_compiler& _dx10ZFunc(u32 Func)
    {
        C->RS.SetRS(D3DRS_ZFUNC, Func);
        return *this;
    }
    // adopt_dx10texture		_dx10texture	(LPCSTR _name)							{	u32 s =
    // C->r_dx10Texture(_name,0);
    // return
    // adopt_dx10sampler(C,s);	}

    adopt_dx10options _dx10Options() { return adopt_dx10options(); };
};
#pragma warning(pop)

class adopt_blend
{
public:
};

class adopt_cmp_func
{
public:
};

class adopt_stencil_op
{
public:
};

// export
void CResourceManager::LS_Load()
{
    // clang-format off
    auto exporterFunc = [](lua_State* luaState)
    {
        module(luaState)
        [
            class_<adopt_dx10options>("_dx10options")
               .def("dx10_msaa_alphatest_atoc", &adopt_dx10options::_dx10_msaa_alphatest_atoc)
               //.def("",					&adopt_dx10options::_dx10Options		),	// returns options-object
            ,

            class_<adopt_dx10sampler>("_dx10sampler")
                //.def("texture",       &adopt_sampler::_texture,       return_reference_to<1>())
                //.def("project",       &adopt_sampler::_projective,    return_reference_to<1>())
                //.def("clamp",         &adopt_sampler::_clamp,         return_reference_to<1>())
                //.def("wrap",          &adopt_sampler::_wrap,          return_reference_to<1>())
                //.def("mirror",        &adopt_sampler::_mirror,        return_reference_to<1>())
                //.def("f_anisotropic", &adopt_sampler::_f_anisotropic, return_reference_to<1>())
                //.def("f_trilinear",   &adopt_sampler::_f_trilinear,   return_reference_to<1>())
                //.def("f_bilinear",    &adopt_sampler::_f_bilinear,    return_reference_to<1>())
                //.def("f_linear",      &adopt_sampler::_f_linear,      return_reference_to<1>())
                //.def("f_none",        &adopt_sampler::_f_none,        return_reference_to<1>())
                //.def("fmin_none",     &adopt_sampler::_fmin_none,     return_reference_to<1>())
                //.def("fmin_point",    &adopt_sampler::_fmin_point,    return_reference_to<1>())
                //.def("fmin_linear",   &adopt_sampler::_fmin_linear,   return_reference_to<1>())
                //.def("fmin_aniso",    &adopt_sampler::_fmin_aniso,    return_reference_to<1>())
                //.def("fmip_none",     &adopt_sampler::_fmip_none,     return_reference_to<1>())
                //.def("fmip_point",    &adopt_sampler::_fmip_point,    return_reference_to<1>())
                //.def("fmip_linear",   &adopt_sampler::_fmip_linear,   return_reference_to<1>())
                //.def("fmag_none",     &adopt_sampler::_fmag_none,     return_reference_to<1>())
                //.def("fmag_point",    &adopt_sampler::_fmag_point,    return_reference_to<1>())
                //.def("fmag_linear",   &adopt_sampler::_fmag_linear,   return_reference_to<1>())
            ,

            class_<adopt_compiler>("_compiler")
                .def(constructor<const adopt_compiler&>())
                .def("begin",                  &adopt_compiler::_pass,       return_reference_to<1>())
                .def("begin",                  &adopt_compiler::_passgs,     return_reference_to<1>())
                .def("sorting",                &adopt_compiler::_options,    return_reference_to<1>())
                .def("emissive",               &adopt_compiler::_o_emissive, return_reference_to<1>())
                .def("distort",                &adopt_compiler::_o_distort,  return_reference_to<1>())
                .def("wmark",                  &adopt_compiler::_o_wmark,    return_reference_to<1>())
                .def("fog",                    &adopt_compiler::_fog,        return_reference_to<1>())
                .def("zb",                     &adopt_compiler::_ZB,         return_reference_to<1>())
                .def("blend",                  &adopt_compiler::_blend,      return_reference_to<1>())
                .def("aref",                   &adopt_compiler::_aref,       return_reference_to<1>())
                //	For compatibility only
                .def("dx10color_write_enable", &adopt_compiler::_dx10color_write_enable, return_reference_to<1>())
                .def("color_write_enable",     &adopt_compiler::_dx10color_write_enable, return_reference_to<1>())
                .def("dx10texture",            &adopt_compiler::_dx10texture,            return_reference_to<1>())
                .def("dx10stencil",            &adopt_compiler::_dx10Stencil,            return_reference_to<1>())
                .def("dx10stencil_ref",        &adopt_compiler::_dx10StencilRef,         return_reference_to<1>())
                .def("dx10atoc",               &adopt_compiler::_dx10ATOC,               return_reference_to<1>())
                .def("dx10zfunc",              &adopt_compiler::_dx10ZFunc,              return_reference_to<1>())

                .def("dx10sampler",            &adopt_compiler::_dx10sampler)  // returns sampler-object
                .def("dx10Options",            &adopt_compiler::_dx10Options), // returns options-object

            class_<adopt_blend>("blend")
                .enum_("blend")
                [
                    value("zero",         int(D3DBLEND_ZERO)),
                    value("one",          int(D3DBLEND_ONE)),
                    value("srccolor",     int(D3DBLEND_SRCCOLOR)),
                    value("invsrccolor",  int(D3DBLEND_INVSRCCOLOR)),
                    value("srcalpha",     int(D3DBLEND_SRCALPHA)),
                    value("invsrcalpha",  int(D3DBLEND_INVSRCALPHA)),
                    value("destalpha",    int(D3DBLEND_DESTALPHA)),
                    value("invdestalpha", int(D3DBLEND_INVDESTALPHA)),
                    value("destcolor",    int(D3DBLEND_DESTCOLOR)),
                    value("invdestcolor", int(D3DBLEND_INVDESTCOLOR)),
                    value("srcalphasat",  int(D3DBLEND_SRCALPHASAT))
                ],

            class_<adopt_cmp_func>("cmp_func")
                .enum_("cmp_func")
                [
                    value("never",        int(D3DCMP_NEVER)),
                    value("less",         int(D3DCMP_LESS)),
                    value("equal",        int(D3DCMP_EQUAL)),
                    value("lessequal",    int(D3DCMP_LESSEQUAL)),
                    value("greater",      int(D3DCMP_GREATER)),
                    value("notequal",     int(D3DCMP_NOTEQUAL)),
                    value("greaterequal", int(D3DCMP_GREATEREQUAL)),
                    value("always",       int(D3DCMP_ALWAYS))
                ],

            class_<adopt_stencil_op>("stencil_op")
                .enum_("stencil_op")
                [
                    value("keep",    int(D3DSTENCILOP_KEEP)),
                    value("zero",    int(D3DSTENCILOP_ZERO)),
                    value("replace", int(D3DSTENCILOP_REPLACE)),
                    value("incrsat", int(D3DSTENCILOP_INCRSAT)),
                    value("decrsat", int(D3DSTENCILOP_DECRSAT)),
                    value("invert",  int(D3DSTENCILOP_INVERT)),
                    value("incr",    int(D3DSTENCILOP_INCR)),
                    value("decr",    int(D3DSTENCILOP_DECR))
                ]
        ];
    };
    // clang-format on

    ScriptEngine.init(exporterFunc, false);
    // load shaders
    const char* shaderPath = RImplementation.getShaderPath();
    xr_vector<char*>* folder = FS.file_list_open("$game_shaders$", shaderPath, FS_ListFiles | FS_RootOnly);
    VERIFY(folder);
    for (u32 it = 0; it < folder->size(); it++)
    {
        string_path namesp, fn;
        xr_strcpy(namesp, (*folder)[it]);
        if (0 == strext(namesp) || 0 != xr_strcmp(strext(namesp), ".s"))
            continue;
        *strext(namesp) = 0;
        if (0 == namesp[0])
            xr_strcpy(namesp, ScriptEngine.GlobalNamespace);
        strconcat(sizeof(fn), fn, shaderPath, (*folder)[it]);
        FS.update_path(fn, "$game_shaders$", fn);
        ScriptEngine.load_file_into_namespace(fn, namesp);
    }
    FS.file_list_close(folder);
}

void CResourceManager::LS_Unload() { ScriptEngine.unload(); }
BOOL CResourceManager::_lua_HasShader(LPCSTR s_shader)
{
    string256 undercorated;
    for (int i = 0, l = xr_strlen(s_shader) + 1; i < l; i++)
        undercorated[i] = ('\\' == s_shader[i]) ? '_' : s_shader[i];

#ifdef _EDITOR
    return ScriptEngine.object(undercorated, "editor", LUA_TFUNCTION);
#else
    return ScriptEngine.object(undercorated, "normal", LUA_TFUNCTION) ||
        ScriptEngine.object(undercorated, "l_special", LUA_TFUNCTION);
#endif
}

Shader* CResourceManager::_lua_Create(LPCSTR d_shader, LPCSTR s_textures)
{
    CBlender_Compile C;
    Shader S;

    // undecorate
    string256 undercorated;
    for (int i = 0, l = xr_strlen(d_shader) + 1; i < l; i++)
        undercorated[i] = ('\\' == d_shader[i]) ? '_' : d_shader[i];
    LPCSTR s_shader = undercorated;

    // Access to template
    C.BT = NULL;
    C.bEditor = FALSE;
    C.bDetail = FALSE;

    // Prepare
    _ParseList(C.L_textures, s_textures);
    C.detail_texture = NULL;
    C.detail_scaler = NULL;

    // Compile element	(LOD0 - HQ)
    if (ScriptEngine.object(s_shader, "normal_hq", LUA_TFUNCTION))
    {
        // Analyze possibility to detail this shader
        C.iElement = 0;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);

        if (C.bDetail)
            S.E[0] = C._lua_Compile(s_shader, "normal_hq");
        else
            S.E[0] = C._lua_Compile(s_shader, "normal");
    }
    else
    {
        if (ScriptEngine.object(s_shader, "normal", LUA_TFUNCTION))
        {
            C.iElement = 0;
            C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
            S.E[0] = C._lua_Compile(s_shader, "normal");
        }
    }

    // Compile element	(LOD1)
    if (ScriptEngine.object(s_shader, "normal", LUA_TFUNCTION))
    {
        C.iElement = 1;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        S.E[1] = C._lua_Compile(s_shader, "normal");
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_point", LUA_TFUNCTION))
    {
        C.iElement = 2;
        C.bDetail = FALSE;
        S.E[2] = C._lua_Compile(s_shader, "l_point");
        ;
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_spot", LUA_TFUNCTION))
    {
        C.iElement = 3;
        C.bDetail = FALSE;
        S.E[3] = C._lua_Compile(s_shader, "l_spot");
        ;
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_special", LUA_TFUNCTION))
    {
        C.iElement = 4;
        C.bDetail = FALSE;
        S.E[4] = C._lua_Compile(s_shader, "l_special");
    }

    // Search equal in shaders array
    for (u32 it = 0; it < v_shaders.size(); it++)
        if (S.equal(v_shaders[it]))
            return v_shaders[it];

    // Create _new_ entry
    Shader* N = new Shader(S);
    N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    v_shaders.push_back(N);
    return N;
}

ShaderElement* CBlender_Compile::_lua_Compile(LPCSTR namesp, LPCSTR name)
{
    ShaderElement E;
    SH = &E;
    RS.Invalidate();

    // Compile
    LPCSTR t_0 = *L_textures[0] ? *L_textures[0] : "null";
    LPCSTR t_1 = (L_textures.size() > 1) ? *L_textures[1] : "null";
    LPCSTR t_d = detail_texture ? detail_texture : "null";
    const object shader = RImplementation.Resources->ScriptEngine.name_space(namesp);
    const functor<void> element = (object)shader[name];
    bool bFirstPass = false;
    adopt_compiler ac = adopt_compiler(this, bFirstPass);
    element(ac, t_0, t_1, t_d);
    r_End();
    ShaderElement* _r = RImplementation.Resources->_CreateElement(E);
    return _r;
}
