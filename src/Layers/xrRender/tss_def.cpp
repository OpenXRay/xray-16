#include "stdafx.h"
#pragma hdrstop

#include "tss_def.h"

#ifdef USE_OGL
#include "../xrRenderGL/glState.h"
#endif

// TODO: DX10: Implement equivalent for SimulatorStates::record for DX10
void SimulatorStates::record(ID3DState*& state)
{
#ifdef USE_OGL
    state = ID3DState::Create();
    for (SimulatorStates::State& S : States)
    {
        // Update aniso value
        if (S.type == 2 && S.v2 == D3DSAMP_MAXANISOTROPY)
            S.v3 = ps_r__tf_Anisotropic;

        // Update states
        switch (S.type)
        {
        case 0:	state->UpdateRenderState(S.v1, S.v2); break;
        //case 1: VERIFY(!"Texture environment not supported"); break;
        case 2: state->UpdateSamplerState(S.v1, S.v2, S.v3); break;
        }
    }
#elif !defined(USE_DX9)
    // VERIFY(!"SimulatorStates::record not implemented!");
    state = ID3DState::Create(*this);
#else
    CHK_DX(HW.pDevice->BeginStateBlock());
    for (u32 it = 0; it < States.size(); it++)
    {
        State& S = States[it];
        switch (S.type)
        {
        case 0: CHK_DX(HW.pDevice->SetRenderState((D3DRENDERSTATETYPE)S.v1, S.v2)); break;
        case 1: CHK_DX(HW.pDevice->SetTextureStageState(S.v1, (D3DTEXTURESTAGESTATETYPE)S.v2, S.v3)); break;
        case 2:
        {
            CHK_DX(HW.pDevice->SetSamplerState(S.v1, (D3DSAMPLERSTATETYPE)S.v2,
                ((D3DSAMPLERSTATETYPE)S.v2==D3DSAMP_MAGFILTER && S.v3==D3DTEXF_ANISOTROPIC) ? D3DTEXF_LINEAR : S.v3));
        }
        break;
        }
    }
    CHK_DX(HW.pDevice->EndStateBlock(&state));
#endif
}

void SimulatorStates::set_RS(u32 a, u32 b)
{
    // Search duplicates
    for (int t = 0; t < int(States.size()); t++)
    {
        State& S = States[t];
        if ((0 == S.type) && (a == S.v1))
        {
            States.erase(States.begin() + t);
            break;
        }
    }

    // Register
    State st;
    st.set_RS(a, b);
    States.push_back(st);
}

void SimulatorStates::set_TSS(u32 a, u32 b, u32 c)
{
    // Search duplicates
    for (int t = 0; t < int(States.size()); t++)
    {
        State& S = States[t];
        if ((1 == S.type) && (a == S.v1) && (b == S.v2))
        {
            States.erase(States.begin() + t);
            break;
        }
    }

    // Register
    State st;
    st.set_TSS(a, b, c);
    States.push_back(st);
}

void SimulatorStates::set_SAMP(u32 a, u32 b, u32 c)
{
    // Search duplicates
    for (int t = 0; t < int(States.size()); t++)
    {
        State& S = States[t];
        if ((2 == S.type) && (a == S.v1) && (b == S.v2))
        {
            States.erase(States.begin() + t);
            break;
        }
    }

    // Register
    State st;
    st.set_SAMP(a, b, c);
    States.push_back(st);
}

BOOL SimulatorStates::equal(SimulatorStates& S)
{
    if (States.size() != S.States.size())
        return FALSE;
    if (0 != memcmp(&*States.begin(), &*S.States.begin(), States.size() * sizeof(State)))
        return FALSE;
    return TRUE;
}

void SimulatorStates::clear() { States.clear(); }

#if !defined(USE_DX9) && !defined(USE_OGL)
#include "Layers/xrRenderDX10/dx10StateUtils.h"

void SimulatorStates::UpdateState(dx10State& state) const
{
    for (u32 it = 0; it < States.size(); it++)
    {
        const State& S = States[it];
        if (S.type == 0)
        {
            switch (S.v1)
            {
            case D3DRS_STENCILREF: state.UpdateStencilRef(S.v2); break;
            case D3DRS_ALPHAREF: state.UpdateAlphaRef(S.v2); break;
            }
        }
    }
}

void SimulatorStates::UpdateDesc(D3D_RASTERIZER_DESC& desc) const
{
    for (u32 it = 0; it < States.size(); it++)
    {
        const State& S = States[it];
        if (S.type == 0)
        {
            // CHK_DX(HW.pDevice->SetRenderState        ((D3DRENDERSTATETYPE)S.v1,S.v2));
            switch (S.v1)
            {
            case D3DRS_FILLMODE:
                if (S.v2 == D3DFILL_SOLID)
                    desc.FillMode = D3D_FILL_SOLID;
                else
                {
                    VERIFY(S.v2 == D3DFILL_WIREFRAME);
                    desc.FillMode = D3D_FILL_WIREFRAME;
                }
                break;

            case D3DRS_CULLMODE:
                desc.CullMode = dx10StateUtils::ConvertCullMode((D3DCULL)S.v2);
                break;
            /*
            switch (S.v2)
            {
            case D3DCULL_NONE:
                desc.CullMode = D3Dxx_CULL_NONE;
                break;
            case D3DCULL_CW:
                desc.CullMode = D3Dxx_CULL_FRONT;
                break;
            case D3DCULL_CCW:
                desc.CullMode = D3Dxx_CULL_BACK;
                break;
            default:
                VERIFY(!"Unexpected cull mode!");
            }
        break;
        */

            //  desc.FrontCounterClockwise = FALSE;

            //  TODO: DX10: Check how to scale unit for depth bias
            case D3DRS_DEPTHBIAS:
                VERIFY(0);
                break;

            //  desc.DepthBiasClamp = 0.0f;

            //  TODO: DX10: Check slope scaled depth bias is used
            case D3DRS_SLOPESCALEDEPTHBIAS:
                // desc.SlopeScaledDepthBias = 0.0f;
                VERIFY(0);
                break;

            //  desc.DepthClipEnable = TRUE;

            case D3DRS_SCISSORTESTENABLE:
                desc.ScissorEnable = S.v2;
                break;

                // desc.MultisampleEnable = FALSE;
                // desc.AntialiasedLineEnable = FALSE;
            }
        }

        // case 1:
        //
        // CHK_DX(HW.pDevice->SetTextureStageState  (S.v1,(D3DTEXTURESTAGESTATETYPE)S.v2,S.v3));
        //  TODO: DX10: Enable
        //  VERIFY(!"DirectX 10 doesn't support texture stage states. Implement shader instead!");
        //  break;
    }
}

void SimulatorStates::UpdateDesc(D3D_DEPTH_STENCIL_DESC& desc) const
{
    for (u32 it = 0; it < States.size(); it++)
    {
        const State& S = States[it];
        if (S.type == 0)
        {
            switch (S.v1)
            {
            case D3DRS_ZENABLE: desc.DepthEnable = S.v2 ? 1 : 0; break;

            case D3DRS_ZWRITEENABLE:
                desc.DepthWriteMask = S.v2 ? D3D_DEPTH_WRITE_MASK_ALL : D3D_DEPTH_WRITE_MASK_ZERO;
                break;

            case D3DRS_ZFUNC: desc.DepthFunc = dx10StateUtils::ConvertCmpFunction((D3DCMPFUNC)S.v2); break;

            case D3DRS_STENCILENABLE: desc.StencilEnable = S.v2 ? 1 : 0; break;

            case D3DRS_STENCILMASK: desc.StencilReadMask = (u8)S.v2; break;

            case D3DRS_STENCILWRITEMASK: desc.StencilWriteMask = (u8)S.v2; break;

            case D3DRS_STENCILFAIL:
                desc.FrontFace.StencilFailOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_STENCILZFAIL:
                desc.FrontFace.StencilDepthFailOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_STENCILPASS:
                desc.FrontFace.StencilPassOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_STENCILFUNC:
                desc.FrontFace.StencilFunc = dx10StateUtils::ConvertCmpFunction((D3DCMPFUNC)S.v2);
                break;

            case D3DRS_CCW_STENCILFAIL:
                desc.BackFace.StencilFailOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_CCW_STENCILZFAIL:
                desc.BackFace.StencilDepthFailOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_CCW_STENCILPASS:
                desc.BackFace.StencilPassOp = dx10StateUtils::ConvertStencilOp((D3DSTENCILOP)S.v2);
                break;

            case D3DRS_CCW_STENCILFUNC:
                desc.BackFace.StencilFunc = dx10StateUtils::ConvertCmpFunction((D3DCMPFUNC)S.v2);
                break;
            }
        }
    }
}

void SimulatorStates::UpdateDesc(D3D_BLEND_DESC& desc) const
{
    for (u32 it = 0; it < States.size(); it++)
    {
        const State& S = States[it];
        if (S.type == 0)
        {
            switch (S.v1)
            {
            case XRDX10RS_ALPHATOCOVERAGE:
                for (int i = 0; i < 8; ++i)
                    desc.AlphaToCoverageEnable = S.v2 ? 1 : 0;
                break;

            case D3DRS_SRCBLEND:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].SrcBlend = dx10StateUtils::ConvertBlendArg((D3DBLEND)S.v2);
                break;

            case D3DRS_DESTBLEND:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].DestBlend = dx10StateUtils::ConvertBlendArg((D3DBLEND)S.v2);
                break;

            // D3DRS_ALPHAFUNC

            case D3DRS_BLENDOP:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].BlendOp = dx10StateUtils::ConvertBlendOp((D3DBLENDOP)S.v2);
                break;

            case D3DRS_SRCBLENDALPHA:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].SrcBlendAlpha = dx10StateUtils::ConvertBlendArg((D3DBLEND)S.v2);
                break;

            case D3DRS_DESTBLENDALPHA:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].DestBlendAlpha = dx10StateUtils::ConvertBlendArg((D3DBLEND)S.v2);
                break;

            case D3DRS_BLENDOPALPHA:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].BlendOpAlpha = dx10StateUtils::ConvertBlendOp((D3DBLENDOP)S.v2);
                break;

            case D3DRS_ALPHABLENDENABLE:
                for (int i = 0; i < 8; ++i)
                    desc.RenderTarget[i].BlendEnable = S.v2 ? 1 : 0;
                break;

            case D3DRS_COLORWRITEENABLE: desc.RenderTarget[0].RenderTargetWriteMask = (u8)S.v2; break;

            case D3DRS_COLORWRITEENABLE1: desc.RenderTarget[1].RenderTargetWriteMask = (u8)S.v2; break;

            case D3DRS_COLORWRITEENABLE2: desc.RenderTarget[2].RenderTargetWriteMask = (u8)S.v2; break;

            case D3DRS_COLORWRITEENABLE3: desc.RenderTarget[3].RenderTargetWriteMask = (u8)S.v2; break;
            }
        }
    }
}

void SimulatorStates::UpdateDesc(D3D_SAMPLER_DESC descArray[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
    bool SamplerUsed[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT], int iBaseSamplerIndex) const
{
    const int MipfilterLinear = 0x01;
    const int MagfilterLinear = 0x04;
    const int MinfilterLinear = 0x10;
    const int AllfilterLinear = 0x15;
    const int FilterAnisotropic = 0x40;
    const int FilterComparison = 0x80;

    for (u32 it = 0; it < States.size(); it++)
    {
        const State& S = States[it];
        if (S.type == 2)
        {
            int iSamplerIndex = int(S.v1);
            iSamplerIndex -= iBaseSamplerIndex;

            if ((iSamplerIndex >= D3D_COMMONSHADER_SAMPLER_SLOT_COUNT) || iSamplerIndex < 0)
                continue;

            SamplerUsed[iSamplerIndex] = true;
            D3D_SAMPLER_DESC& desc = descArray[iSamplerIndex];

            switch (S.v2)
            {
            // D3D_FILTER Filter;
            case D3DSAMP_MAGFILTER: /* D3DTEXTUREFILTER filter to use for magnification */
                switch (S.v3)
                {
                case D3DTEXF_NONE:
                case D3DTEXF_POINT: desc.Filter = (D3D_FILTER)(desc.Filter & (~MagfilterLinear)); break;
                case D3DTEXF_LINEAR:
                    desc.Filter = (D3D_FILTER)(desc.Filter | MagfilterLinear);
                    //desc.Filter |= MagfilterLinear;
                    break;
                default: NODEFAULT;
                }
                break;

            case D3DSAMP_MINFILTER: /* D3DTEXTUREFILTER filter to use for minification */
                switch (S.v3)
                {
                case D3DTEXF_NONE:
                case D3DTEXF_POINT:
                    // desc.Filter &= ~MinfilterLinear;
                    desc.Filter = (D3D_FILTER)(desc.Filter & (~MinfilterLinear));
                    break;
                case D3DTEXF_LINEAR:
                    desc.Filter = (D3D_FILTER)(desc.Filter | MinfilterLinear);
                    // desc.Filter |= MinfilterLinear;
                    break;
                default: NODEFAULT;
                }
                break;

            case D3DSAMP_MIPFILTER: /* D3DTEXTUREFILTER filter to use between mipmaps during minification */
                switch (S.v3)
                {
                case D3DTEXF_NONE:
                case D3DTEXF_POINT:
                    desc.Filter = (D3D_FILTER)(desc.Filter & (~MipfilterLinear));
                    // desc.Filter &= ~MipfilterLinear;
                    break;
                case D3DTEXF_LINEAR:
                    desc.Filter = (D3D_FILTER)(desc.Filter | MipfilterLinear);
                    // desc.Filter |= MipfilterLinear;
                    break;
                default: NODEFAULT;
                }
                break;

            case XRDX10SAMP_ANISOTROPICFILTER:
                if (S.v3)
                    desc.Filter = (D3D_FILTER)(desc.Filter | FilterAnisotropic);
                // desc.Filter |= FilterAnisotropic;
                else
                    desc.Filter = (D3D_FILTER)(desc.Filter & (~FilterAnisotropic));
                // desc.Filter &= ~FilterAnisotropic;
                break;

            case XRDX10SAMP_COMPARISONFILTER:
                if (S.v3)
                    desc.Filter = (D3D_FILTER)(desc.Filter | FilterComparison);
                // desc.Filter |= FilterComparison;
                else
                    desc.Filter = (D3D_FILTER)(desc.Filter & (~FilterComparison));
                // desc.Filter &= ~FilterComparison;
                break;

            // D3Dxx_TEXTURE_ADDRESS_MODE AddressU;
            case D3DSAMP_ADDRESSU: /* D3DTEXTUREADDRESS for U coordinate */
                desc.AddressU = dx10StateUtils::ConvertTextureAddressMode(D3DTEXTUREADDRESS(S.v3));
                break;

            case D3DSAMP_ADDRESSV: /* D3DTEXTUREADDRESS for V coordinate */
                desc.AddressV = dx10StateUtils::ConvertTextureAddressMode(D3DTEXTUREADDRESS(S.v3));
                break;

            case D3DSAMP_ADDRESSW: /* D3DTEXTUREADDRESS for W coordinate */
                desc.AddressW = dx10StateUtils::ConvertTextureAddressMode(D3DTEXTUREADDRESS(S.v3));
                break;

            // FLOAT MipLODBias
            case D3DSAMP_MIPMAPLODBIAS:
                desc.MipLODBias = *((float*)(&(S.v3)));
                break;

            // UINT MaxAnisotropy;
            case D3DSAMP_MAXANISOTROPY:
                desc.MaxAnisotropy = S.v3;
                break;

            // D3Dxx_COMPARISON_FUNC ComparisonFunc;
            case XRDX10SAMP_COMPARISONFUNC:
                desc.ComparisonFunc = (D3D_COMPARISON_FUNC)S.v3;
                break;

            // FLOAT BorderColor[4];
            case D3DSAMP_BORDERCOLOR:
            {
                desc.BorderColor[0] = ((S.v3 >> 16) & 0xff) / 255.0f;
                desc.BorderColor[1] = ((S.v3 >> 8) & 0xff) / 255.0f;
                desc.BorderColor[2] = ((S.v3) & 0xff) / 255.0f;
                desc.BorderColor[3] = ((S.v3 >> 24) & 0xff) / 255.0f;
            }
            break;

            // FLOAT MinLOD;
            case XRDX10SAMP_MINLOD:
                desc.MinLOD = (FLOAT)S.v3;
                break;

            // FLOAT MaxLOD;
            case D3DSAMP_MAXMIPLEVEL: desc.MaxLOD = (FLOAT)S.v3; break;
            }
        }
    }

    // Validate data
    for (int i = 0; i < D3D_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        D3D_SAMPLER_DESC& desc = descArray[i];
        if (desc.Filter & FilterAnisotropic)
        {
            desc.Filter = (D3D_FILTER)(desc.Filter | AllfilterLinear);
            // desc.Filter |= AllfilterLinear;
        }

        VERIFY(desc.MinLOD <= desc.MaxLOD);
        if (desc.MinLOD > desc.MaxLOD)
            desc.MaxLOD = desc.MinLOD;
    }
}

#endif // !USE_DX9 && !USE_OGL
