#ifndef dx10SamplerStateCache_included
#define dx10SamplerStateCache_included
#pragma once

class dx10SamplerStateCache
{
public:
    enum
    {
        hInvalidHandle = 0xFFFFFFFF
    };

    //	State handle
    typedef u32 SHandle;
    typedef xr_vector<SHandle> HArray;

public:
    dx10SamplerStateCache();
    ~dx10SamplerStateCache();

    void ClearStateArray();

    SHandle GetState(D3D_SAMPLER_DESC& desc);

    void VSApplySamplers(HArray& samplers);
    void PSApplySamplers(HArray& samplers);
    void GSApplySamplers(HArray& samplers);
#ifdef USE_DX11
    void HSApplySamplers(HArray& samplers);
    void DSApplySamplers(HArray& samplers);
    void CSApplySamplers(HArray& samplers);
#endif

    void SetMaxAnisotropy(u32 uiMaxAniso);
    void SetMipLODBias(float uiMipLODBias);

private:
    typedef ID3DSamplerState IDeviceState;
    typedef D3D_SAMPLER_DESC StateDecs;

    struct StateRecord
    {
        u32 m_crc;
        IDeviceState* m_pState;
    };

private:
    void CreateState(StateDecs desc, IDeviceState** ppIState);
    SHandle FindState(const StateDecs& desc, u32 StateCRC);

    void PrepareSamplerStates(HArray& samplers, ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT]) const;

    //	Private data
private:
    //	This must be cleared on device destroy
    xr_vector<StateRecord> m_StateArray;

    u32 m_uiMaxAnisotropy;
    float m_uiMipLODBias;
};

extern dx10SamplerStateCache SSManager;

#endif //	dx10SamplerStateCache_included
