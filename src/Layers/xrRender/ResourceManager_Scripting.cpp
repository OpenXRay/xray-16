#include "stdafx.h"
#pragma hdrstop

#include "xrEngine/Render.h"
#include "ResourceManager.h"
#include "tss.h"
#include "blenders/Blender.h"
#include "blenders/Blender_Recorder.h"
#include "xrScriptEngine/script_engine.hpp"
#include "luabind/return_reference_to_policy.hpp"

using namespace luabind;
using namespace luabind::policy;

#ifdef DEBUG
#define MDB Memory.dbg_check()
#else
#define MDB
#endif

// wrapper
class adopt_sampler
{
    CBlender_Compile* C;
    u32 stage;

public:
    adopt_sampler(CBlender_Compile* _C, u32 _stage) : C(_C), stage(_stage)
    {
        if (u32(-1) == stage)
            C = nullptr;
    }
    adopt_sampler(const adopt_sampler& _C) : C(_C.C), stage(_C.stage)
    {
        if (u32(-1) == stage)
            C = nullptr;
    }

    adopt_sampler& _texture(LPCSTR texture)
    {
        if (C)
            C->i_Texture(stage, texture);
        return *this;
    }
    adopt_sampler& _projective(bool _b)
    {
        if (C)
            C->i_Projective(stage, _b);
        return *this;
    }
    adopt_sampler& _clamp()
    {
        if (C)
            C->i_Address(stage, D3DTADDRESS_CLAMP);
        return *this;
    }
    adopt_sampler& _wrap()
    {
        if (C)
            C->i_Address(stage, D3DTADDRESS_WRAP);
        return *this;
    }
    adopt_sampler& _mirror()
    {
        if (C)
            C->i_Address(stage, D3DTADDRESS_MIRROR);
        return *this;
    }
    adopt_sampler& _f_anisotropic()
    {
        if (C)
            C->i_Filter(stage, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
        return *this;
    }
    adopt_sampler& _f_trilinear()
    {
        if (C)
            C->i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
        return *this;
    }
    adopt_sampler& _f_bilinear()
    {
        if (C)
            C->i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_POINT, D3DTEXF_LINEAR);
        return *this;
    }
    adopt_sampler& _f_linear()
    {
        if (C)
            C->i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        return *this;
    }
    adopt_sampler& _f_none()
    {
        if (C)
            C->i_Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
        return *this;
    }
    adopt_sampler& _fmin_none()
    {
        if (C)
            C->i_Filter_Min(stage, D3DTEXF_NONE);
        return *this;
    }
    adopt_sampler& _fmin_point()
    {
        if (C)
            C->i_Filter_Min(stage, D3DTEXF_POINT);
        return *this;
    }
    adopt_sampler& _fmin_linear()
    {
        if (C)
            C->i_Filter_Min(stage, D3DTEXF_LINEAR);
        return *this;
    }
    adopt_sampler& _fmin_aniso()
    {
        if (C)
            C->i_Filter_Min(stage, D3DTEXF_ANISOTROPIC);
        return *this;
    }
    adopt_sampler& _fmip_none()
    {
        if (C)
            C->i_Filter_Mip(stage, D3DTEXF_NONE);
        return *this;
    }
    adopt_sampler& _fmip_point()
    {
        if (C)
            C->i_Filter_Mip(stage, D3DTEXF_POINT);
        return *this;
    }
    adopt_sampler& _fmip_linear()
    {
        if (C)
            C->i_Filter_Mip(stage, D3DTEXF_LINEAR);
        return *this;
    }
    adopt_sampler& _fmag_none()
    {
        if (C)
            C->i_Filter_Mag(stage, D3DTEXF_NONE);
        return *this;
    }
    adopt_sampler& _fmag_point()
    {
        if (C)
            C->i_Filter_Mag(stage, D3DTEXF_POINT);
        return *this;
    }
    adopt_sampler& _fmag_linear()
    {
        if (C)
            C->i_Filter_Mag(stage, D3DTEXF_LINEAR);
        return *this;
    }
};

// wrapper
class adopt_compiler
{
    CBlender_Compile* C;

public:
    adopt_compiler(CBlender_Compile* _C) : C(_C) {}
    adopt_compiler(const adopt_compiler& _C) : C(_C.C) {}
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
        C->r_Pass(vs, ps, true);
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
    adopt_compiler& _color_write_enable(bool cR, bool cG, bool cB, bool cA)
    {
        C->r_ColorWriteEnable(cR, cG, cB, cA);
        return *this;
    }
    adopt_sampler _sampler(LPCSTR _name)
    {
        u32 s = C->r_Sampler(_name, nullptr);
        return adopt_sampler(C, s);
    }
};

class adopt_blend
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
            class_<adopt_sampler>("_sampler")
                 .def(constructor<const adopt_sampler&>())
                 .def("texture",       &adopt_sampler::_texture,       return_reference_to<1>())
                 .def("project",       &adopt_sampler::_projective,    return_reference_to<1>())
                 .def("clamp",         &adopt_sampler::_clamp,         return_reference_to<1>())
                 .def("wrap",          &adopt_sampler::_wrap,          return_reference_to<1>())
                 .def("mirror",        &adopt_sampler::_mirror,        return_reference_to<1>())
                 .def("f_anisotropic", &adopt_sampler::_f_anisotropic, return_reference_to<1>())
                 .def("f_trilinear",   &adopt_sampler::_f_trilinear,   return_reference_to<1>())
                 .def("f_bilinear",    &adopt_sampler::_f_bilinear,    return_reference_to<1>())
                 .def("f_linear",      &adopt_sampler::_f_linear,      return_reference_to<1>())
                 .def("f_none",        &adopt_sampler::_f_none,        return_reference_to<1>())
                 .def("fmin_none",     &adopt_sampler::_fmin_none,     return_reference_to<1>())
                 .def("fmin_point",    &adopt_sampler::_fmin_point,    return_reference_to<1>())
                 .def("fmin_linear",   &adopt_sampler::_fmin_linear,   return_reference_to<1>())
                 .def("fmin_aniso",    &adopt_sampler::_fmin_aniso,    return_reference_to<1>())
                 .def("fmip_none",     &adopt_sampler::_fmip_none,     return_reference_to<1>())
                 .def("fmip_point",    &adopt_sampler::_fmip_point,    return_reference_to<1>())
                 .def("fmip_linear",   &adopt_sampler::_fmip_linear,   return_reference_to<1>())
                 .def("fmag_none",     &adopt_sampler::_fmag_none,     return_reference_to<1>())
                 .def("fmag_point",    &adopt_sampler::_fmag_point,    return_reference_to<1>())
                 .def("fmag_linear",   &adopt_sampler::_fmag_linear,   return_reference_to<1>()),

            class_<adopt_compiler>("_compiler")
                .def(constructor<const adopt_compiler&>())
                .def("begin",              &adopt_compiler::_pass,               return_reference_to<1>())
                .def("sorting",            &adopt_compiler::_options,            return_reference_to<1>())
                .def("emissive",           &adopt_compiler::_o_emissive,         return_reference_to<1>())
                .def("distort",            &adopt_compiler::_o_distort,          return_reference_to<1>())
                .def("wmark",              &adopt_compiler::_o_wmark,            return_reference_to<1>())
                .def("fog",                &adopt_compiler::_fog,                return_reference_to<1>())
                .def("zb",                 &adopt_compiler::_ZB,                 return_reference_to<1>())
                .def("blend",              &adopt_compiler::_blend,              return_reference_to<1>())
                .def("aref",               &adopt_compiler::_aref,               return_reference_to<1>())
                .def("color_write_enable", &adopt_compiler::_color_write_enable, return_reference_to<1>())
                .def("sampler",            &adopt_compiler::_sampler), // returns sampler-object

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
        if (nullptr == strext(namesp) || 0 != xr_strcmp(strext(namesp), ".s"))
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
        undercorated[i] = (_DELIMITER == s_shader[i]) ? '_' : s_shader[i];

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
        undercorated[i] = (_DELIMITER == d_shader[i]) ? '_' : d_shader[i];
    LPCSTR s_shader = undercorated;

    // Access to template
    C.BT = nullptr;
    C.bEditor = FALSE;
    C.bDetail = FALSE;

    // Prepare
    _ParseList(C.L_textures, s_textures);
    C.detail_texture = nullptr;
    C.detail_scaler = nullptr;

    // Compile element (LOD0 - HQ)
    if (ScriptEngine.object(s_shader, "normal_hq", LUA_TFUNCTION))
    {
        // Analyze possibility to detail this shader
        C.iElement = SE_R1_NORMAL_HQ;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        S.E[C.iElement] = C._lua_Compile(s_shader, C.bDetail ? "normal_hq" : "normal");
    }
    else
    {
        if (ScriptEngine.object(s_shader, "normal", LUA_TFUNCTION))
        {
            C.iElement = SE_R1_NORMAL_HQ;
            C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
            S.E[C.iElement] = C._lua_Compile(s_shader, "normal");
        }
    }

    // Compile element (LOD1)
    if (ScriptEngine.object(s_shader, "normal", LUA_TFUNCTION))
    {
        C.iElement = SE_R1_NORMAL_LQ;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        S.E[C.iElement] = C._lua_Compile(s_shader, "normal");
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_point", LUA_TFUNCTION))
    {
        C.iElement = SE_R1_LPOINT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        S.E[C.iElement] = C._lua_Compile(s_shader, "l_point");
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_spot", LUA_TFUNCTION))
    {
        C.iElement = SE_R1_LSPOT;
        C.bDetail = m_textures_description.GetDetailTexture(C.L_textures[0], C.detail_texture, C.detail_scaler);
        S.E[C.iElement] = C._lua_Compile(s_shader, "l_spot");
    }

    // Compile element
    if (ScriptEngine.object(s_shader, "l_special", LUA_TFUNCTION))
    {
        C.iElement = SE_R1_LMODELS;
        C.bDetail = FALSE;
        S.E[C.iElement] = C._lua_Compile(s_shader, "l_special");
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
    adopt_compiler ac = adopt_compiler(this);
    element(ac, t_0, t_1, t_d);
    r_End();
    ShaderElement* _r = RImplementation.Resources->_CreateElement(E);
    return _r;
}
