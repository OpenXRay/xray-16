#include "stdafx.h"
#include "dx11ConstantBuffer.h"

#include "Layers/xrRender/BufferUtils.h"

#if defined (XR_PLATFORM_WINDOWS)
#define USE_CPU_SSE
#endif

/////////////////////////////////////////////////////////////////////////////////////

static inline bool CopyData(void* dst, const void* src, size_t size)
{
    bool requires_flush = true;
#if defined(USE_CPU_SSE)
    if ((((uintptr_t)dst | (uintptr_t)src | size) & 0xf) == 0u)
    {
        __m128* d = (__m128*)dst;
        const __m128* s = (const __m128*)src;
        const __m128* e = (const __m128*)src + (size >> 4);
        while (s < e)
        {
            _mm_stream_ps((float*)(d++), _mm_load_ps((const float*)(s++)));
        }
        _mm_sfence();
        requires_flush = false;
    }
    else
#endif
    {
        memcpy(dst, src, size);
    }
   
    return requires_flush;
}

/////////////////////////////////////////////////////////////////////////////////////

dx11ConstantBuffer::~dx11ConstantBuffer()
{
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        RImplementation.Resources->_DeleteConstantBuffer(id, this);
    }
    //	Flush();
#if defined(USE_DX12)
    HW.DeallocateConstantBuffer(this);
#else
    _RELEASE(m_buffer);
#endif
    xr_free(m_bufferData);
}

dx11ConstantBuffer::dx11ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable)
    : 
    m_buffer(nullptr),
#if defined(USE_DX12)
    m_base_ptr(nullptr), 
    m_allocator(nullptr), 
    m_offset(0), 
    m_size(0),
#endif
    m_bufferChanged(true)
{
    D3D_SHADER_BUFFER_DESC Desc;

    CHK_DX(pTable->GetDesc(&Desc));

    m_bufferName._set(Desc.Name);
    m_bufferType = Desc.Type;
    m_bufferSize = Desc.Size;

#if defined(USE_DX12)
    m_size = (Desc.Size + 255) & ~255;
#endif

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

#if !defined(USE_DX12)
    R_CHK(BufferUtils::CreateConstantBuffer(&m_buffer, Desc.Size));
    VERIFY(m_buffer);
#endif

    m_bufferData = xr_malloc(Desc.Size);
    VERIFY(m_bufferData);
    ZeroMemory(m_bufferData, Desc.Size);

#ifdef DEBUG
#if !defined(USE_DX12)
    if (m_buffer)
    {
        m_buffer->SetPrivateData(WKPDID_D3DDebugObjectName, xr_strlen(Desc.Name), Desc.Name);
    }
#endif
#endif
}

bool dx11ConstantBuffer::Similar(dx11ConstantBuffer& _in)
{
    if (m_bufferName._get() != _in.m_bufferName._get())
        return false;

    if (m_bufferType != _in.m_bufferType)
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
    if (m_bufferChanged)
    {
        void* pData = nullptr;
#if defined(USE_DX12)
        pData = HW.AllocateConstantBuffer(this);
#elif defined(USE_DX11) 
        D3D11_MAPPED_SUBRESOURCE pSubRes;
        CHK_DX(HW.get_context(context_id)->Map(m_buffer, 0, D3D_MAP_WRITE_DISCARD, 0, &pSubRes));
        pData = pSubRes.pData;
#else
        CHK_DX(m_buffer->Map(D3D_MAP_WRITE_DISCARD, 0, &pData));
#endif
        VERIFY(pData);
        VERIFY(m_bufferData);
        CopyData(pData, m_bufferData, m_bufferSize);
#if defined(USE_DX12)
#elif defined(USE_DX11)
        HW.get_context(context_id)->Unmap(m_buffer, 0);
#else
        m_buffer->Unmap();
#endif
        m_bufferChanged = false;
    }
}
