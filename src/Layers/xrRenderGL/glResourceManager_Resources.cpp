#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"
#include "../xrRender/tss.h"
#include "../xrRender/Blender.h"
#include "../xrRender/Blender_Recorder.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(const SPass& proto)
{
    for (SPass* pass : v_passes)
        if (pass->equal(proto))
            return pass;

    SPass* P = v_passes.emplace_back(xr_new<SPass>());
    P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    P->state = proto.state;
    P->ps = proto.ps;
    P->vs = proto.vs;
    P->gs = proto.gs;
    P->pp = proto.pp;
    P->constants = proto.constants;
    P->T = proto.T;
#ifdef _EDITOR
    P->M = proto.M;
#endif
    P->C = proto.C;

    return P;
}

//--------------------------------------------------------------------------------------------------------------

SDeclaration* CResourceManager::_CreateDecl(const D3DVERTEXELEMENT9* dcl)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (!D->dcl_code.empty() && dcl_equal(dcl, &D->dcl_code.front()))
            return D;
    }

    SDeclaration* D = v_declarations.emplace_back(xr_new<SDeclaration>());
    glGenVertexArrays(1, &D->dcl);

    u32 dcl_size = GetDeclLength(dcl) + 1;
    D->dcl_code.assign(dcl, dcl + dcl_size);
    ConvertVertexDeclaration(dcl, D);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

//--------------------------------------------------------------------------------------------------------------

SPP* CResourceManager::_CreatePP(pcstr vs, pcstr ps, pcstr gs, pcstr hs, pcstr ds)
{
    pcstr skinning{}, samples{};
    switch (RImplementation.m_skinning)
    {
    case -1: skinning = "";   break;
    case  0: skinning = "_0"; break;
    case  1: skinning = "_1"; break;
    case  2: skinning = "_2"; break;
    case  3: skinning = "_3"; break;
    case  4: skinning = "_4"; break;
    default: NODEFAULT;
    }
    switch (RImplementation.m_MSAASample)
    {
    case -1: samples = "";   break;
    case  0: samples = "_0"; break;
    case  1: samples = "_1"; break;
    case  2: samples = "_2"; break;
    case  3: samples = "_3"; break;
    case  4: samples = "_4"; break;
    case  5: samples = "_5"; break;
    case  6: samples = "_6"; break;
    case  7: samples = "_7"; break;
    default: NODEFAULT;
    }

    string256 name{};
    strconcat(name, vs, skinning, "|", ps, samples, "|", gs/*, "|", hs, "|", ds*/); // XXX: Tesselation

    const auto iterator = m_pp.find(name);

    if (iterator != m_pp.end())
        return iterator->second;

    SPP* pp = xr_new<SPP>();

    pp->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    m_pp.emplace(pp->set_name(name), pp);

    return pp;
}

static GLuint GLGeneratePipeline(pcstr name, GLuint ps, GLuint vs, GLuint gs)
{
    GLuint pp;
    CHK_GL(glGenProgramPipelines(1, &pp));
    R_ASSERT(pp);
    CHK_GL(glUseProgramStages(pp, GL_FRAGMENT_SHADER_BIT, ps));
    CHK_GL(glUseProgramStages(pp, GL_VERTEX_SHADER_BIT,   vs));
    CHK_GL(glUseProgramStages(pp, GL_GEOMETRY_SHADER_BIT, gs));
    CHK_GL(glValidateProgramPipeline(pp));
    return pp;
}

static GLuint GLLinkMonolithicProgram(pcstr name, GLuint ps, GLuint vs, GLuint gs)
{
    const GLuint program = glCreateProgram();
    R_ASSERT(program);
    if (GLEW_VERSION_4_3)
        CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
    // XXX: support caching for monolithic programs
    //if (HW.ShaderBinarySupported)
    //    CHK_GL(glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, (GLint)GL_TRUE));

    CHK_GL(glAttachShader(program, ps));
    CHK_GL(glAttachShader(program, vs));
    if (gs)
        CHK_GL(glAttachShader(program, gs));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target"));
    CHK_GL(glBindFragDataLocation(program, 0, "SV_Target0"));
    CHK_GL(glBindFragDataLocation(program, 1, "SV_Target1"));
    CHK_GL(glBindFragDataLocation(program, 2, "SV_Target2"));
    CHK_GL(glLinkProgram(program));
    CHK_GL(glDetachShader(program, ps));
    CHK_GL(glDetachShader(program, vs));
    if (gs)
        CHK_GL(glDetachShader(program, gs));

    GLint status{};
    CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (GLboolean(status) == GL_FALSE)
    {
        show_compile_errors(name, program, 0);
        CHK_GL(glDeleteProgram(program));
        return 0; // 0 means error
    }
    return program;
}

bool CResourceManager::_LinkPP(SPass& pass)
{
    auto& pp = *pass.pp;
    if (pp.pp)
        return true;

    if (HW.SeparateShaderObjectsSupported)
        pp.pp = GLGeneratePipeline(pp.cName.c_str(), pass.ps->sh, pass.vs->sh, pass.gs->sh);
    else
    {
        pp.pp = GLLinkMonolithicProgram(pp.cName.c_str(), pass.ps->sh, pass.vs->sh, pass.gs->sh);
        pp.constants.parse(&pp.pp, RC_dest_all);

        pass.ps = nullptr;
        pass.vs = nullptr;
        pass.gs = nullptr;
    }

    return pp.pp != 0;
}

void CResourceManager::_DeletePP(const SPP* pp)
{
    if (0 == (pp->dwFlags & xr_resource_flagged::RF_REGISTERED))
        return;

    const pstr N = const_cast<pstr>(*pp->cName);
    auto iterator = m_pp.find(N);

    if (iterator != m_pp.end())
    {
        m_pp.erase(iterator);
        return;
    }

    Msg("! ERROR: Failed to find program pipeline '%s'", pp->cName.c_str());
}

//--------------------------------------------------------------------------------------------------------------

SVS* CResourceManager::_CreateVS(cpcstr shader, u32 flags /*= 0*/)
{
    string_path name;
    xr_strcpy(name, shader);
    switch (RImplementation.m_skinning)
    {
    case 0:
        xr_strcat(name, "_0");
        break;
    case 1:
        xr_strcat(name, "_1");
        break;
    case 2:
        xr_strcat(name, "_2");
        break;
    case 3:
        xr_strcat(name, "_3");
        break;
    case 4:
        xr_strcat(name, "_4");
        break;
    }

    return CreateShader<SVS>(name, shader, flags);
}

void CResourceManager::_DeleteVS(const SVS* vs) { DestroyShader(vs); }

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
    switch (RImplementation.m_MSAASample)
    {
    case 0:
        xr_strcat(name, "_0");
        break;
    case 1:
        xr_strcat(name, "_1");
        break;
    case 2:
        xr_strcat(name, "_2");
        break;
    case 3:
        xr_strcat(name, "_3");
        break;
    case 4:
        xr_strcat(name, "_4");
        break;
    case 5:
        xr_strcat(name, "_5");
        break;
    case 6:
        xr_strcat(name, "_6");
        break;
    case 7:
        xr_strcat(name, "_7");
        break;
    }

    return CreateShader<SPS>(name, _name);
}

void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }

//--------------------------------------------------------------------------------------------------------------
SGS* CResourceManager::_CreateGS(LPCSTR Name) { return CreateShader<SGS>(Name); }
void CResourceManager::_DeleteGS(const SGS* gs) { DestroyShader(gs); }

SHS* CResourceManager::_CreateHS(LPCSTR Name) { return CreateShader<SHS>(Name); }
void CResourceManager::_DeleteHS(const SHS* HS) { DestroyShader(HS); }

SDS* CResourceManager::_CreateDS(LPCSTR Name) { return CreateShader<SDS>(Name); }
void CResourceManager::_DeleteDS(const SDS* DS) { DestroyShader(DS); }

SCS* CResourceManager::_CreateCS(LPCSTR Name) { return CreateShader<SCS>(Name); }
void CResourceManager::_DeleteCS(const SCS* CS) { DestroyShader(CS); }

//--------------------------------------------------------------------------------------------------------------

