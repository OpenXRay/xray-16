#pragma once
#include "xrEngine/CameraDefs.h"
struct SPPInfo;

// постпроцесс
class ENGINE_API CEffectorPP : public SBaseEffector
{
    EEffectorPPType eType;
    bool bFreeOnRemove;

protected:
    float fLifeTime;

public:
    CEffectorPP(EEffectorPPType type, f32 lifeTime, bool free_on_remove = true);
    CEffectorPP() : bFreeOnRemove(true), fLifeTime(0.0f), bOverlap(true) {};
    virtual ~CEffectorPP();
    BENCH_SEC_SCRAMBLEVTBL1
    virtual BOOL Process(SPPInfo& PPInfo);
    virtual BOOL Valid() { return fLifeTime > 0.0f; }
    IC EEffectorPPType Type() const { return eType; }
    IC bool FreeOnRemove() const { return bFreeOnRemove; }
    IC void SetType(EEffectorPPType t) { eType = t; }
    virtual void Stop(float speed) { fLifeTime = 0.0f; };
    bool bOverlap;
};
