#pragma once

namespace xray::render::RENDER_NAMESPACE
{
template <class IDeviceState, class StateDecs>
class dx11StateCache
{
    //	Public interface
public:
    dx11StateCache();
    ~dx11StateCache();

    void ClearStateArray();

    IDeviceState* GetState(SimulatorStates& state_code);
    IDeviceState* GetState(StateDecs& desc);
    //	Can be called on device destruction only!
    //	dx11State holds weak links on manager's states and
    //	won't understand that state was destroyed
    // void	FlushStates();
    //	Private functionality

    //	Private declarations
private:
    struct StateRecord
    {
        u32 m_crc;
        IDeviceState* m_pState;
    };

private:
    void CreateState(StateDecs desc, IDeviceState** ppIState);
    IDeviceState* FindState(const StateDecs& desc, u32 StateCRC);

    //	Private data
private:
    //	This must be cleared on device destroy
    xr_vector<StateRecord> m_StateArray;
};

extern dx11StateCache<ID3DRasterizerState, D3D_RASTERIZER_DESC> RSManager;
extern dx11StateCache<ID3DDepthStencilState, D3D_DEPTH_STENCIL_DESC> DSSManager;
extern dx11StateCache<ID3DBlendState, D3D_BLEND_DESC> BSManager;
} // namespace xray::render::RENDER_NAMESPACE

#include "dx11StateCacheImpl.h"
