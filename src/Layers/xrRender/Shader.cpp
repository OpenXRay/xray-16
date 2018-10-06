// Shader.cpp: implementation of the CShader class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "Shader.h"
#include "ResourceManager.h"
// XXX: render scripts should call these destructors before resource manager gets destroyed
STextureList::~STextureList() { RImplementation.Resources->_DeleteTextureList(this); }
SMatrixList::~SMatrixList() { RImplementation.Resources->_DeleteMatrixList(this); }
SConstantList::~SConstantList() { RImplementation.Resources->_DeleteConstantList(this); }
SPass::~SPass() { RImplementation.Resources->_DeletePass(this); }
ShaderElement::~ShaderElement() { RImplementation.Resources->_DeleteElement(this); }
SGeometry::~SGeometry() { RImplementation.Resources->DeleteGeom(this); }
Shader::~Shader() { RImplementation.Resources->Delete(this); }
//////////////////////////////////////////////////////////////////////////
void resptrcode_shader::create(LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    _set(RImplementation.Resources->Create(s_shader, s_textures, s_constants, s_matrices));
}
void resptrcode_shader::create(IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
    _set(RImplementation.Resources->Create(B, s_shader, s_textures, s_constants, s_matrices));
}

//////////////////////////////////////////////////////////////////////////
#ifdef USE_OGL
void resptrcode_geom::create(u32 FVF, GLuint vb, GLuint ib)
#else
void resptrcode_geom::create(u32 FVF, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
#endif // USE_OGL
{
    _set(RImplementation.Resources->CreateGeom(FVF, vb, ib));
}
#ifdef USE_OGL
void resptrcode_geom::create(D3DVERTEXELEMENT9* decl, GLuint vb, GLuint ib)
#else
void resptrcode_geom::create(D3DVERTEXELEMENT9* decl, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib)
#endif // USE_OGL
{
    _set(RImplementation.Resources->CreateGeom(decl, vb, ib));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BOOL SPass::equal(const SPass& other)
{
    if (state != other.state)
        return FALSE;
    if (ps != other.ps)
        return FALSE;
    if (vs != other.vs)
        return FALSE;
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    if (gs != other.gs)
        return FALSE;
#ifdef USE_DX11
    if (hs != other.hs)
        return FALSE;
    if (ds != other.ds)
        return FALSE;
    if (cs != other.cs)
        return FALSE;
#endif
#endif //	USE_DX10
    if (constants != other.constants)
        return FALSE; // is this nessesary??? (ps+vs already combines)

    if (T != other.T)
        return FALSE;
    if (C != other.C)
        return FALSE;
#ifdef _EDITOR
    if (M != other.M)
        return FALSE;
#endif
    return TRUE;
}

//
ShaderElement::ShaderElement()
{
    flags.iPriority = 1;
    flags.bStrictB2F = FALSE;
    flags.bEmissive = FALSE;
    flags.bDistort = FALSE;
    flags.bWmark = FALSE;
}

BOOL ShaderElement::equal(ShaderElement& S)
{
    if (flags.iPriority != S.flags.iPriority)
        return FALSE;
    if (flags.bStrictB2F != S.flags.bStrictB2F)
        return FALSE;
    if (flags.bEmissive != S.flags.bEmissive)
        return FALSE;
    if (flags.bWmark != S.flags.bWmark)
        return FALSE;
    if (flags.bDistort != S.flags.bDistort)
        return FALSE;
    if (passes.size() != S.passes.size())
        return FALSE;
    for (u32 p = 0; p < passes.size(); p++)
        if (passes[p] != S.passes[p])
            return FALSE;
    return TRUE;
}

BOOL Shader::equal(Shader* S, int index)
{
    if(nullptr == E[index] && nullptr == S->E[index])
        return TRUE;
    if(nullptr == E[index] || nullptr == S->E[index])
        return FALSE;

    return (E[index] == S->E[index]);
}

BOOL Shader::equal(Shader* S)
{
    for (int i = 0; i < 4; i++)
    {
        if (!equal(S, i))
            return FALSE;
    }
    return TRUE;
}

void STextureList::clear()
{
    iterator it = begin();
    iterator it_e = end();
    for (; it != it_e; ++it)
        (*it).second.destroy();

    erase(begin(), end());
}

u32 STextureList::find_texture_stage(const shared_str& TexName) const
{
    u32 dwTextureStage = 0;

    STextureList::const_iterator _it = this->begin();
    STextureList::const_iterator _end = this->end();
    for (; _it != _end; _it++)
    {
        const std::pair<u32, ref_texture>& loader = *_it;

        //	Shadowmap texture always uses 0 texture unit
        if (loader.second->cName == TexName)
        {
            //	Assign correct texture
            dwTextureStage = loader.first;
            break;
        }
    }

    VERIFY(_it != _end);

    return dwTextureStage;
}
