#pragma once

namespace xray::render::fg
{
namespace dx11StateUtils
{
//	Set description to default values
void ResetDescription(D3D_RASTERIZER_DESC& desc);
void ResetDescription(D3D_DEPTH_STENCIL_DESC& desc);
void ResetDescription(D3D_BLEND_DESC& desc);
void ResetDescription(D3D_SAMPLER_DESC& desc);

//	State comparison (memcmp doesn't work due to padding bytes in structure)
bool operator==(const D3D_RASTERIZER_DESC& desc1, const D3D_RASTERIZER_DESC& desc2);
bool operator==(const D3D_DEPTH_STENCIL_DESC& desc1, const D3D_DEPTH_STENCIL_DESC& desc2);
bool operator==(const D3D_BLEND_DESC& desc1, const D3D_BLEND_DESC& desc2);
bool operator==(const D3D_SAMPLER_DESC& desc1, const D3D_SAMPLER_DESC& desc2);

//	Calculate hash values
u32 GetHash(const D3D_RASTERIZER_DESC& desc);
u32 GetHash(const D3D_DEPTH_STENCIL_DESC& desc);
u32 GetHash(const D3D_BLEND_DESC& desc);
u32 GetHash(const D3D_SAMPLER_DESC& desc);

//	Modify state to meet DX11 automatic modifications
void ValidateState(D3D_RASTERIZER_DESC& desc);
void ValidateState(D3D_DEPTH_STENCIL_DESC& desc);
void ValidateState(D3D_BLEND_DESC& desc);
void ValidateState(D3D_SAMPLER_DESC& desc);
} // namespace dx11StateUtils
} // namespace xray::render::fg
