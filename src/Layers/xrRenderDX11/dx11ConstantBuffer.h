#pragma once

namespace xray::render::RENDER_NAMESPACE
{
struct R_constant;
struct R_constant_load;

class dx11ConstantBuffer : public xr_resource_named
{
public:
    dx11ConstantBuffer(ID3DShaderReflectionConstantBuffer* pTable);
    dx11ConstantBuffer(const char* name, u32 size);  // Slang reflection constructor
    ~dx11ConstantBuffer();

    bool Similar(dx11ConstantBuffer& _in);
    ID3DBuffer* GetBuffer() { return m_pBuffer; }
    void* GetBufferData() { return m_pBufferData; }
    u32 GetBufferSize() const { return m_uiBufferSize; }
    const char* GetBufferName() const { return m_strBufferName.c_str(); }
    void Flush(u32 context_id);

    // CB Slot Decoding Helpers
    // X-Ray encodes CB slots as: bits 0-3 = buffer index, bits 4-6 = shader type
    // See r_constants.h for CB_Buffer* enums
    static u32 DecodeShaderType(u32 encodedSlot)
    {
        return encodedSlot & 0x70; // CB_BufferTypeMask
    }

    static u32 DecodeBufferIndex(u32 encodedSlot)
    {
        return encodedSlot & 0xF; // CB_BufferIndexMask
    }

    // Returns the actual HLSL register index (b0, b1, b2, etc.)
    static u32 DecodeBindingSlot(u32 encodedSlot)
    {
        return DecodeBufferIndex(encodedSlot);
    }

    // Get shader type name for debugging
    static const char* GetShaderTypeName(u32 shaderType)
    {
        switch (shaderType)
        {
        case 0x10: return "PS"; // CB_BufferPixelShader
        case 0x20: return "VS"; // CB_BufferVertexShader
        case 0x30: return "GS"; // CB_BufferGeometryShader
        case 0x40: return "HS"; // CB_BufferHullShader
        case 0x50: return "DS"; // CB_BufferDomainShader
        case 0x60: return "CS"; // CB_BufferComputeShader
        default: return "Unknown";
        }
    }

    //	Set copy data into constant buffer
    //	Plain buffer member
    void set(R_constant* C, R_constant_load& L, const Fmatrix& A);
    void set(R_constant* C, R_constant_load& L, const Fvector4& A);
    void set(R_constant* C, R_constant_load& L, float A);
    void set(R_constant* C, R_constant_load& L, int A);
    void set(R_constant* C, R_constant_load& L, u32 A);
    //	Array buffer member
    void seta(R_constant* C, R_constant_load& L, u32 e, const Fmatrix& A);
    void seta(R_constant* C, R_constant_load& L, u32 e, const Fvector4& A);

    void* AccessDirect(R_constant_load& L, size_t DataSize);

private:
    Fvector4* Access(u16 offset);

private:
    shared_str m_strBufferName;
    D3D_CBUFFER_TYPE m_eBufferType;

    //	Buffer data description
    u32 m_uiMembersCRC;
    xr_vector<D3D_SHADER_TYPE_DESC> m_MembersList;
    xr_vector<shared_str> m_MembersNames;

    ID3DBuffer* m_pBuffer;
    u32 m_uiBufferSize; //	Cache buffer size for debug validation
    void* m_pBufferData;
    bool m_bChanged;

    static const u32 lineSize = sizeof(Fvector4);

    //	Never try to copy objects of this class due to the pointer and autoptr members
    dx11ConstantBuffer(const dx11ConstantBuffer&);
    dx11ConstantBuffer& operator=(dx11ConstantBuffer&);
};

typedef resptr_core<dx11ConstantBuffer, resptr_base<dx11ConstantBuffer>> ref_cbuffer;
} // namespace xray::render::RENDER_NAMESPACE
