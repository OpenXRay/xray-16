#pragma once

#include "Layers/xrRender/RenderContext/PipelineState.h"

#if defined(USE_DX11)
#include <d3d11.h>
#endif

namespace xray::render {

#if defined(USE_DX11)

inline ng::CullMode ConvertCullMode(D3D11_CULL_MODE d3dCull) {
    switch (d3dCull) {
        case D3D11_CULL_NONE: return ng::CullMode::None;
        case D3D11_CULL_FRONT: return ng::CullMode::Front;
        case D3D11_CULL_BACK: return ng::CullMode::Back;
        default: return ng::CullMode::Back;
    }
}

inline ng::FillMode ConvertFillMode(D3D11_FILL_MODE d3dFill) {
    switch (d3dFill) {
        case D3D11_FILL_WIREFRAME: return ng::FillMode::Wireframe;
        case D3D11_FILL_SOLID: return ng::FillMode::Solid;
        default: return ng::FillMode::Solid;
    }
}

inline ng::StencilOp ConvertStencilOp(D3D11_STENCIL_OP d3dOp) {
    switch (d3dOp) {
        case D3D11_STENCIL_OP_KEEP: return ng::StencilOp::Keep;
        case D3D11_STENCIL_OP_ZERO: return ng::StencilOp::Zero;
        case D3D11_STENCIL_OP_REPLACE: return ng::StencilOp::Replace;
        case D3D11_STENCIL_OP_INCR_SAT: return ng::StencilOp::IncrementSaturate;
        case D3D11_STENCIL_OP_DECR_SAT: return ng::StencilOp::DecrementSaturate;
        case D3D11_STENCIL_OP_INVERT: return ng::StencilOp::Invert;
        case D3D11_STENCIL_OP_INCR: return ng::StencilOp::Increment;
        case D3D11_STENCIL_OP_DECR: return ng::StencilOp::Decrement;
        default: return ng::StencilOp::Keep;
    }
}

inline ng::ComparisonFunc ConvertComparisonFunc(D3D11_COMPARISON_FUNC d3dFunc) {
    switch (d3dFunc) {
        case D3D11_COMPARISON_NEVER: return ng::ComparisonFunc::Never;
        case D3D11_COMPARISON_LESS: return ng::ComparisonFunc::Less;
        case D3D11_COMPARISON_EQUAL: return ng::ComparisonFunc::Equal;
        case D3D11_COMPARISON_LESS_EQUAL: return ng::ComparisonFunc::LessEqual;
        case D3D11_COMPARISON_GREATER: return ng::ComparisonFunc::Greater;
        case D3D11_COMPARISON_NOT_EQUAL: return ng::ComparisonFunc::NotEqual;
        case D3D11_COMPARISON_GREATER_EQUAL: return ng::ComparisonFunc::GreaterEqual;
        case D3D11_COMPARISON_ALWAYS: return ng::ComparisonFunc::Always;
        default: return ng::ComparisonFunc::Less;
    }
}

inline ng::BlendFactor ConvertBlendFactor(D3D11_BLEND d3dBlend) {
    switch (d3dBlend) {
        case D3D11_BLEND_ZERO: return ng::BlendFactor::Zero;
        case D3D11_BLEND_ONE: return ng::BlendFactor::One;
        case D3D11_BLEND_SRC_COLOR: return ng::BlendFactor::SrcColor;
        case D3D11_BLEND_INV_SRC_COLOR: return ng::BlendFactor::InvSrcColor;
        case D3D11_BLEND_SRC_ALPHA: return ng::BlendFactor::SrcAlpha;
        case D3D11_BLEND_INV_SRC_ALPHA: return ng::BlendFactor::InvSrcAlpha;
        case D3D11_BLEND_DEST_ALPHA: return ng::BlendFactor::DstAlpha;
        case D3D11_BLEND_INV_DEST_ALPHA: return ng::BlendFactor::InvDstAlpha;
        case D3D11_BLEND_DEST_COLOR: return ng::BlendFactor::DstColor;
        case D3D11_BLEND_INV_DEST_COLOR: return ng::BlendFactor::InvDstColor;
        case D3D11_BLEND_SRC_ALPHA_SAT: return ng::BlendFactor::SrcAlphaSat;
        case D3D11_BLEND_BLEND_FACTOR: return ng::BlendFactor::BlendFactor;
        case D3D11_BLEND_INV_BLEND_FACTOR: return ng::BlendFactor::InvBlendFactor;
        default: return ng::BlendFactor::One;
    }
}

inline ng::BlendOp ConvertBlendOp(D3D11_BLEND_OP d3dOp) {
    switch (d3dOp) {
        case D3D11_BLEND_OP_ADD: return ng::BlendOp::Add;
        case D3D11_BLEND_OP_SUBTRACT: return ng::BlendOp::Subtract;
        case D3D11_BLEND_OP_REV_SUBTRACT: return ng::BlendOp::RevSubtract;
        case D3D11_BLEND_OP_MIN: return ng::BlendOp::Min;
        case D3D11_BLEND_OP_MAX: return ng::BlendOp::Max;
        default: return ng::BlendOp::Add;
    }
}

inline ng::ColorWriteMask ConvertColorWriteMask(u8 d3dMask) {
    ng::ColorWriteMask mask = ng::ColorWriteMask::None;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_RED)   mask = mask | ng::ColorWriteMask::Red;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_GREEN) mask = mask | ng::ColorWriteMask::Green;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_BLUE)  mask = mask | ng::ColorWriteMask::Blue;
    if (d3dMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) mask = mask | ng::ColorWriteMask::Alpha;
    return mask;
}

nvrhi::Format ConvertDxgiFormatToNvrhi(DXGI_FORMAT dxgiFormat);

#endif

}
