#ifndef ParticleCustom_included
#define ParticleCustom_included
#pragma once

class IParticleCustom
{
public:
    virtual ~IParticleCustom() { ; }
    virtual void OnDeviceCreate() = 0;
    virtual void OnDeviceDestroy() = 0;

    virtual void UpdateParent(const Fmatrix& m, const Fvector& velocity, bool bXFORM) = 0;
    virtual void OnFrame(u32 dt) = 0;

    virtual void Play() = 0;
    virtual void Stop(bool bDefferedStop = TRUE) = 0;
    virtual bool IsPlaying() = 0;

    virtual u32 ParticlesCount() = 0;

    virtual float GetTimeLimit() = 0;
    virtual bool IsLooped() { return GetTimeLimit() < 0.f; }
    virtual const shared_str Name() = 0;
    virtual void SetHudMode(bool b) = 0;
    virtual bool GetHudMode() = 0;
};

#endif //	ParticleCustom_included
