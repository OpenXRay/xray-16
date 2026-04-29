#pragma once

#if defined(USE_OGL)
#include "../xrRenderGL/glState.h"
#endif

namespace xray::render::fg
{
class SimulatorStates
{
private:
    struct State
    {
        u32 type; // 0=RS, 1=TSS
        u32 v1, v2, v3;

        void set_RS(u32 a, u32 b)
        {
            type = 0;
            v1 = a;
            v2 = b;
            v3 = 0;
        }

        void set_TSS(u32 a, u32 b, u32 c)
        {
            type = 1;
            v1 = a;
            v2 = b;
            v3 = c;
        }

        void set_SAMP(u32 a, u32 b, u32 c)
        {
            type = 2;
            v1 = a;
            v2 = b;
            v3 = c;
        }
    };

private:
    xr_vector<State> States;

public:
    SimulatorStates() = default;

    void set_RS(u32 a, u32 b);
    void set_TSS(u32 a, u32 b, u32 c);
    void set_SAMP(u32 a, u32 b, u32 c);
    BOOL equal(SimulatorStates& S);
    void clear();
    void record(ID3DState*& state);

    // Query render state value (returns false if not found)
    bool get_RS(u32 rsType, u32& outValue) const
    {
        for (const auto& S : States)
        {
            if (S.type == 0 && S.v1 == rsType)
            {
                outValue = S.v2;
                return true;
            }
        }
        return false;
    }

    // Check if alpha testing is enabled
    bool IsAlphaTestEnabled() const
    {
        u32 val = 0;
        return get_RS(D3DRS_ALPHATESTENABLE, val) && val != 0;
    }

    // Check if alpha blending is enabled (non-opaque)
    bool IsAlphaBlendEnabled() const
    {
        u32 val = 0;
        return get_RS(D3DRS_ALPHABLENDENABLE, val) && val != 0;
    }
#if defined(USE_DX11)
    void UpdateState(dx11State& state) const;
    void UpdateDesc(D3D_RASTERIZER_DESC& desc) const;
    void UpdateDesc(D3D_DEPTH_STENCIL_DESC& desc) const;
    void UpdateDesc(D3D_BLEND_DESC& desc) const;
    void UpdateDesc(D3D_SAMPLER_DESC descArray[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
        bool SamplerUsed[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT], int iBaseSamplerIndex) const;
#endif
};
} // namespace xray::render::fg
