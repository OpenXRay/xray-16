#include "stdafx.h"
#pragma hdrstop

#include "dx11VertexLayoutCache.h"
#include "Layers/xrRender/SH_Atomic.h"

namespace xray::render::fg
{
extern const char* ToSemanticName(u32 vertexSemantic);

static DXGI_FORMAT VertexFormatToDxgiDX11(u32 vertexFormat)
{
    switch (vertexFormat)
    {
    case VF_FLOAT1:    return DXGI_FORMAT_R32_FLOAT;
    case VF_FLOAT2:    return DXGI_FORMAT_R32G32_FLOAT;
    case VF_FLOAT3:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case VF_FLOAT4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case VF_COLOR:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case VF_UBYTE4:    return DXGI_FORMAT_R8G8B8A8_UINT;
    case VF_SHORT2:    return DXGI_FORMAT_R16G16_SINT;
    case VF_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
    case VF_UBYTE4N:   return DXGI_FORMAT_R8G8B8A8_UNORM;
    case VF_SHORT2N:   return DXGI_FORMAT_R16G16_SNORM;
    case VF_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
    case VF_USHORT2N:  return DXGI_FORMAT_R16G16_UNORM;
    case VF_USHORT4N:  return DXGI_FORMAT_R16G16B16A16_UNORM;
    case VF_FLOAT16_2: return DXGI_FORMAT_R16G16_FLOAT;
    case VF_FLOAT16_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:
        VERIFY(!"VertexFormatToDxgiDX11: unsupported format");
        return DXGI_FORMAT_UNKNOWN;
    }
}

void ConvertVertexDeclarationDX11(
    const xr_vector<VertexElement>& declIn,
    xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut)
{
    const s32 iDeclSize = static_cast<s32>(declIn.size()) - 1;
    declOut.resize(iDeclSize + 1);

    for (s32 i = 0; i < iDeclSize; ++i)
    {
        const VertexElement& descIn = declIn[i];
        D3D_INPUT_ELEMENT_DESC& descOut = declOut[i];

        descOut.SemanticName        = ToSemanticName(descIn.Usage);
        descOut.SemanticIndex       = descIn.UsageIndex;
        descOut.Format              = VertexFormatToDxgiDX11(descIn.Type);
        descOut.InputSlot           = descIn.Stream;
        descOut.AlignedByteOffset   = descIn.Offset;
        descOut.InputSlotClass      = D3D_INPUT_PER_VERTEX_DATA;
        descOut.InstanceDataStepRate = 0;
    }

    if (iDeclSize >= 0)
        ZeroMemory(&declOut[iDeclSize], sizeof(declOut[iDeclSize]));
}

DX11DeclBackendData* GetDX11Data(const SDeclaration* decl)
{
    return static_cast<DX11DeclBackendData*>(decl->backend_data);
}
}
