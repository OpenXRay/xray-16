#pragma once

#include "Layers/xrRender/VertexLayout.h"

namespace xray::render::fg
{
struct SDeclaration;

struct DX11DeclBackendData
{
    xr_map<ID3DBlob*, ID3DInputLayout*> vs_to_layout;
    xr_vector<D3D_INPUT_ELEMENT_DESC>   dx11_dcl_code;
};

DX11DeclBackendData* GetDX11Data(const SDeclaration* decl);

void ConvertVertexDeclarationDX11(
    const xr_vector<VertexElement>& declIn,
    xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut);
}
