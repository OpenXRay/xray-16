#include "stdafx.h"
#include "dx11ConstantBuffer.h"

#include "Layers/xrRender/BufferUtils.h"

namespace xray::render::RENDER_NAMESPACE
{
dx11ConstantBuffer::~dx11ConstantBuffer()
{
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        RImplementation.Resources->_DeleteConstantBuffer(id, this);
    }
    //	Flush();
    _RELEASE(m_pBuffer);
    xr_free(m_pBufferData);
}

dx11ConstantBuffer::dx11ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable) : m_bChanged(true)
{
    D3D_SHADER_BUFFER_DESC Desc;

    CHK_DX(pTable->GetDesc(&Desc));

    m_strBufferName._set(Desc.Name);
    m_eBufferType = Desc.Type;
    m_uiBufferSize = Desc.Size;

    //	Fill member list with variable descriptions
    m_MembersList.resize(Desc.Variables);
    m_MembersNames.resize(Desc.Variables);
    for (u32 i = 0; i < Desc.Variables; ++i)
    {
        ID3DShaderReflectionVariable* pVar;
        ID3DShaderReflectionType* pType;

        D3D_SHADER_VARIABLE_DESC var_desc;

        pVar = pTable->GetVariableByIndex(i);
        VERIFY(pVar);
        pType = pVar->GetType();
        VERIFY(pType);
        pType->GetDesc(&m_MembersList[i]);
        //	Buffers with the same layout can contain totally different members
        CHK_DX(pVar->GetDesc(&var_desc));
        m_MembersNames[i] = var_desc.Name;
    }

    m_uiMembersCRC = crc32(&m_MembersList[0], Desc.Variables * sizeof(m_MembersList[0]));

    // Debug: Check for undersized constant buffers
    if (Desc.Size % 16 != 0)
    {
        Msg("! [ConstantBuffer] WARNING: Buffer '%s' size=%d is not multiple of 16 (Variables=%d)",
            Desc.Name, Desc.Size, Desc.Variables);
        for (u32 i = 0; i < Desc.Variables; ++i)
        {
            Msg("  Variable[%d]: %s", i, m_MembersNames[i].c_str());
        }
    }

    R_CHK(BufferUtils::CreateConstantBuffer(&m_pBuffer, Desc.Size));
    VERIFY(m_pBuffer);
    m_pBufferData = xr_malloc(Desc.Size);
    VERIFY(m_pBufferData);

#ifdef DEBUG
    if (m_pBuffer)
    {
        m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(Desc.Name), Desc.Name);
    }
#endif
}

dx11ConstantBuffer::dx11ConstantBuffer(const char* name, u32 size) : m_bChanged(true)
{
    VERIFY(name);
    VERIFY(size > 0);

    m_strBufferName._set(name);
    m_eBufferType = D3D_CT_CBUFFER;
    m_uiBufferSize = size;

    // For Slang-created buffers, we don't have detailed member information
    // The buffer is created with the total size from Slang reflection
    m_MembersList.clear();
    m_MembersNames.clear();
    m_uiMembersCRC = 0;  // No member CRC for Slang buffers

    R_CHK(BufferUtils::CreateConstantBuffer(&m_pBuffer, size));
    VERIFY(m_pBuffer);
    m_pBufferData = xr_malloc(size);
    VERIFY(m_pBufferData);

#ifdef DEBUG
    if (m_pBuffer)
    {
        m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(name), name);
    }
#endif
}

bool dx11ConstantBuffer::Similar(dx11ConstantBuffer& _in)
{
    if (m_strBufferName._get() != _in.m_strBufferName._get())
        return false;

    if (m_eBufferType != _in.m_eBufferType)
        return false;

    if (m_uiMembersCRC != _in.m_uiMembersCRC)
        return false;

    if (m_MembersList.size() != _in.m_MembersList.size())
        return false;

    if (memcmp(&m_MembersList[0], &_in.m_MembersList[0], m_MembersList.size() * sizeof(m_MembersList[0])))
        return false;

    VERIFY(m_MembersNames.size() == _in.m_MembersNames.size());

    int iMemberNum = m_MembersNames.size();
    for (int i = 0; i < iMemberNum; ++i)
    {
        if (m_MembersNames[i].c_str() != _in.m_MembersNames[i].c_str())
            return false;
    }

    return true;
}

void dx11ConstantBuffer::Flush(u32 context_id)
{
    if (m_bChanged)
    {
        void* pData;
#ifdef USE_DX11
        D3D11_MAPPED_SUBRESOURCE pSubRes;
        CHK_DX(HW.get_context(context_id)->Map(m_pBuffer, 0, D3D_MAP_WRITE_DISCARD, 0, &pSubRes));
        pData = pSubRes.pData;
#else
        CHK_DX(m_pBuffer->Map(D3D_MAP_WRITE_DISCARD, 0, &pData));
#endif
        VERIFY(pData);
        VERIFY(m_pBufferData);
        CopyMemory(pData, m_pBufferData, m_uiBufferSize);
#ifdef USE_DX11
        HW.get_context(context_id)->Unmap(m_pBuffer, 0);
#else
        m_pBuffer->Unmap();
#endif
        m_bChanged = false;
    }
}
} // namespace xray::render::RENDER_NAMESPACE
