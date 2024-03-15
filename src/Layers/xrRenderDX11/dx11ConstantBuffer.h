#ifndef dx11ConstantBuffer_included
#define dx11ConstantBuffer_included
#pragma once

struct R_constant;
struct R_constant_load;

class dx11ConstantBuffer : public xr_resource_named
{
    friend class dx11ConstantBufferAllocator;

public:
    dx11ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable);
    ~dx11ConstantBuffer();

    bool Similar(dx11ConstantBuffer& _in);
    
    ID3DBuffer* GetBuffer() const
    {
        return m_buffer; 
    }

#if defined(USE_DX12) 
    bool IsUsedBuffer() const 
    { 
        return m_base_ptr != nullptr; 
    }

    u32 GetBufferNum() const
    {
        return m_offset / DX12::CONSTANT_BUFFER_ELEMENT_SIZE;
    }
    
    u32 GetBufferSize() const 
    { 
        return m_size / DX12::CONSTANT_BUFFER_ELEMENT_SIZE;
    }

    void *GetBufferCPUPtr() const
    {
        R_ASSERT(m_base_ptr);
        return (void*)((uintptr_t)m_base_ptr + m_offset);     
    }
#endif
    
    void Flush(u32 context_id);

    //	Set copy data into constant buffer
    //	Plain buffer member
    void set(R_constant* C, R_constant_load& L, const Fmatrix& A);
    void set(R_constant* C, R_constant_load& L, const Fvector4& A);
    void set(R_constant* C, R_constant_load& L, float A);
    void set(R_constant* C, R_constant_load& L, int A);
    //	Array buffer member
    void seta(R_constant* C, R_constant_load& L, u32 e, const Fmatrix& A);
    void seta(R_constant* C, R_constant_load& L, u32 e, const Fvector4& A);

    void* AccessDirect(R_constant_load& L, size_t DataSize);

private:

    Fvector4* Access(u16 offset);

private:
    shared_str m_bufferName;
    D3D_CBUFFER_TYPE m_bufferType;

    //	Buffer data description
    u32 m_uiMembersCRC;
    xr_vector<D3D_SHADER_TYPE_DESC> m_MembersList;
    xr_vector<shared_str> m_MembersNames;

    ID3DBuffer* m_buffer;
    u32 m_bufferSize; //	Cache buffer size for debug validation
    void* m_bufferData;
   
#if defined(USE_DX12)
    void* m_allocator;
    void* m_base_ptr;
    u32 m_offset, m_size; 
#endif

    bool m_bufferChanged;

    static const u32 lineSize = sizeof(Fvector4);

    //	Never try to copy objects of this class due to the pointer and autoptr members
    dx11ConstantBuffer(const dx11ConstantBuffer&);
    dx11ConstantBuffer& operator=(dx11ConstantBuffer&);
};

typedef resptr_core<dx11ConstantBuffer, resptr_base<dx11ConstantBuffer>> ref_cbuffer;

#endif //	dx11ConstantBuffer_included
