////////////////////////////////////////////////////////////////////////////
//	Created		: 21.05.2009
//	Author		: Mykhailo Parfeniuk
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ComputeShader.h"

void ComputeShader::Construct(ID3D11ComputeShader* cs, const ref_ctable& ctable, xr_vector<ID3D11SamplerState*>& Samplers,
    xr_vector<ID3D11ShaderResourceView*>& Textures, xr_vector<ID3D11UnorderedAccessView*>& Outputs)
{
    m_cs = cs;
    m_ctable = ctable;
    m_Textures.swap(Textures);
    m_Outputs.swap(Outputs);
    m_Samplers.swap(Samplers);
}

ComputeShader::~ComputeShader()
{
    for (size_t i = 0; i < m_Textures.size(); ++i)
        m_Textures[i]->Release();

    for (size_t i = 0; i < m_Outputs.size(); ++i)
        m_Outputs[i]->Release();

    for (size_t i = 0; i < m_Samplers.size(); ++i)
        m_Samplers[i]->Release();
}

u32 GetCB(const ref_constant& C) { return (C->destination & RC_dest_pixel_cb_index_mask) >> RC_dest_pixel_cb_index_shift; }
ComputeShader& ComputeShader::set_c(CBackend& cmd_list, shared_str name, const Fvector4& value)
{
    ref_constant c = m_ctable->get(name);
    (m_ctable->m_CBTable[cmd_list.context_id])[GetCB(c)].second->set(&*c, c->ps, value);
    return *this;
}

ComputeShader& ComputeShader::set_c(CBackend& cmd_list, shared_str name, float x, float y, float z, float w)
{
    Fvector4 vec;
    vec.set(x, y, z, w);
    return set_c(cmd_list, name, vec);
}

void ComputeShader::Dispatch(CBackend& cmd_list, u32 dimx, u32 dimy, u32 dimz)
{
    const auto context_id = cmd_list.context_id;

    u32 count = m_ctable->m_CBTable[cmd_list.context_id].size();

    for (u32 i = 0; i < count; ++i)
    {
        (m_ctable->m_CBTable[cmd_list.context_id])[i].second->Flush(context_id);
    }

    ID3DBuffer* tempBuffer[CBackend::MaxCBuffers];

    for (u32 i = 0; i < count; ++i)
    {
        tempBuffer[i] = (m_ctable->m_CBTable[cmd_list.context_id])[i].second->GetBuffer();
    }

    // process constant-loaders
    R_constant_table::c_table::iterator it = m_ctable->table.begin();
    R_constant_table::c_table::iterator end = m_ctable->table.end();
    for (; it != end; ++it)
    {
        R_constant* Cs = &**it;
        if (Cs->handler)
            Cs->handler->setup(cmd_list, Cs);
    }

    HW.get_context(context_id)->CSSetConstantBuffers(0, count, tempBuffer);

    if (!m_Textures.empty())
        HW.get_context(context_id)->CSSetShaderResources(0, m_Textures.size(), &m_Textures[0]);

    if (!m_Samplers.empty())
        HW.get_context(context_id)->CSSetSamplers(0, m_Samplers.size(), &m_Samplers[0]);

    if (!m_Outputs.empty())
    {
        u32 num = 0;
        HW.get_context(context_id)->CSSetUnorderedAccessViews(0, m_Outputs.size(), &m_Outputs[0], &num);
    }

    HW.get_context(context_id)->Dispatch(dimx, dimy, dimz);
}
