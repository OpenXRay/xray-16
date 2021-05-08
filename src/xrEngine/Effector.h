#pragma once
#include "CameraDefs.h"
#include "xrCore/xrCore_benchmark_macros.h"
#include "device.h"

class ENGINE_API CEffectorCam : public SBaseEffector
{
protected:
    ECamEffectorType eType;

    friend class CCameraManager;
    float fLifeTime;
    bool bHudAffect;

public:
    CEffectorCam(ECamEffectorType type, float tm)
    {
        eType = type;
        fLifeTime = tm;
        bHudAffect = true;
    };
    CEffectorCam()
    {
        eType = (ECamEffectorType)0;
        fLifeTime = 0.0f;
        bHudAffect = true;
    };
    virtual ~CEffectorCam(){};
    void SetType(ECamEffectorType type) { eType = type; }
    void SetHudAffect(bool val) { bHudAffect = val; }
    bool GetHudAffect() { return bHudAffect; }
    IC ECamEffectorType GetType() { return eType; }
    virtual bool Valid() { return fLifeTime > 0.0f; }
    BENCH_SEC_SCRAMBLEVTBL1

    virtual bool ProcessCam(SCamEffectorInfo& info)
    {
        fLifeTime -= Device.fTimeDelta;
        return Valid();
    };
    ;

    virtual void ProcessIfInvalid(SCamEffectorInfo& info){};
    virtual bool AllowProcessingIfInvalid() { return false; }
    virtual bool AbsolutePositioning() { return false; }
};
