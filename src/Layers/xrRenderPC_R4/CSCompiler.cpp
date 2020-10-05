////////////////////////////////////////////////////////////////////////////
//	Created		: 22.05.2009
//	Author		: Mykhailo Parfeniuk
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CSCompiler.h"
#include "ComputeShader.h"

CSCompiler::CSCompiler(ComputeShader& target) : m_Target(target), m_cs(0) {}
CSCompiler& CSCompiler::begin(const char* name)
{
    compile(name);
    return *this;
}

CSCompiler& CSCompiler::defSampler(LPCSTR ResourceName)
{
    D3D11_SAMPLER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT
    if (0 == xr_strcmp(ResourceName, "smp_nofilter"))
    {
        // i_Address( stage, D3DTADDRESS_CLAMP);
        // i_Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        return defSampler(ResourceName, desc);
    }

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
    if (0 == xr_strcmp(ResourceName, "smp_rtlinear"))
    {
        // i_Address( stage, D3DTADDRESS_CLAMP);
        // i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        return defSampler(ResourceName, desc);
    }

    //	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
    if (0 == xr_strcmp(ResourceName, "smp_linear"))
    {
        // i_Address( stage, D3DTADDRESS_WRAP);
        // i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        return defSampler(ResourceName, desc);
    }

    //	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC
    if (0 == xr_strcmp(ResourceName, "smp_base"))
    {
        // i_Address( stage, D3DTADDRESS_WRAP);
        // i_dx10FilterAnizo( stage, TRUE);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.MaxAnisotropy = 8;
        return defSampler(ResourceName, desc);
    }

    //	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR
    if (0 == xr_strcmp(ResourceName, "smp_material"))
    {
        // i_Address( stage, D3DTADDRESS_CLAMP);
        // i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        // RS.SetSAMP(stage,D3DSAMP_ADDRESSW,	D3DTADDRESS_WRAP);
        desc.AddressU = desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        return defSampler(ResourceName, desc);
    }

    if (0 == xr_strcmp(ResourceName, "smp_smap"))
    {
        // i_Address( stage, D3DTADDRESS_CLAMP);
        // i_Filter(stage, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
        // RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFILTER, TRUE);
        // RS.SetSAMP(stage, XRDX10SAMP_COMPARISONFUNC, D3D_COMPARISON_LESS_EQUAL);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        desc.ComparisonFunc = D3D_COMPARISON_LESS_EQUAL;
        return defSampler(ResourceName, desc);
    }

    if (0 == xr_strcmp(ResourceName, "smp_jitter"))
    {
        // i_Address( stage, D3DTADDRESS_WRAP);
        // i_Filter(stage, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        return defSampler(ResourceName, desc);
    }

    VERIFY(0);

    return *this;
}

CSCompiler& CSCompiler::defSampler(LPCSTR ResourceName, const D3D_SAMPLER_DESC& def)
{
    VERIFY(ResourceName);

    ref_constant C = m_constants.get(ResourceName);
    if (!C)
        return *this;

    R_ASSERT(C->type == RC_sampler);
    u32 stage = C->samp.index;

    if (stage >= m_Samplers.size())
        m_Samplers.resize(stage + 1);

    R_CHK(HW.pDevice->CreateSamplerState(&def, &m_Samplers[stage]));

    return *this;
}

void fix_texture_name(pstr);

CSCompiler& CSCompiler::defOutput(LPCSTR ResourceName, const ref_rt& rt)
{
    VERIFY(ResourceName);
    if (!rt)
        return *this;

    ref_constant C = m_constants.get(ResourceName);
    if (!C)
        return *this;

    R_ASSERT(C->type == RC_dx11UAV);
    u32 stage = C->samp.index;

    if (stage >= m_Textures.size())
        m_Textures.resize(stage + 1);

    m_Outputs[stage] = rt->pUAView; //!!!dangerous view can be deleted

    return *this;
}

CSCompiler& CSCompiler::defTexture(LPCSTR ResourceName, ref_texture texture)
{
    VERIFY(ResourceName);
    if (!texture)
        return *this;

    // Find index
    ref_constant C = m_constants.get(ResourceName);
    if (!C)
        return *this;

    R_ASSERT(C->type == RC_dx10texture);
    u32 stage = C->samp.index;

    if (stage >= m_Textures.size())
        m_Textures.resize(stage + 1);

    m_Textures[stage] = texture->get_SRView(); //!!!dangerous view can be deleted

    return *this;
}

void CSCompiler::end()
{
    for (size_t i = 0; i < m_Textures.size(); ++i)
        m_Textures[i]->AddRef();

    for (size_t i = 0; i < m_Outputs.size(); ++i)
        m_Outputs[i]->AddRef();

    // Samplers create by us, thou they should not be AddRef'ed

    m_Target.Construct(
        m_cs, RImplementation.Resources->_CreateConstantTable(m_constants), m_Samplers, m_Textures, m_Outputs);
}

void CSCompiler::compile(const char* name)
{
    if (0 == xr_stricmp(name, "null"))
    {
        m_cs = 0;
        return;
    }

    string_path cname;
    strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), name, ".cs");
    FS.update_path(cname, "$game_shaders$", cname);

    IReader* file = FS.r_open(cname);
    R_ASSERT2(file, cname);

    // Select target
    LPCSTR c_target = "cs_5_0";
    LPCSTR c_entry = "main";

    HRESULT const _hr = GEnv.Render->shader_compile(name, file, c_entry,
        c_target, D3D10_SHADER_PACK_MATRIX_ROW_MAJOR, (void*&)m_cs);

    FS.r_close(file);

    VERIFY(SUCCEEDED(_hr));

    CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");
}
