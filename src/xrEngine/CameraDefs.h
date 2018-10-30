#pragma once
#ifndef CAMERA_DEFS_H_INCLUDED
#define CAMERA_DEFS_H_INCLUDED

#include "xrCore/fastdelegate.h"
#include "xrCore/_vector3d.h"

struct ENGINE_API SBaseEffector
{
    typedef fastdelegate::FastDelegate0<> CB_ON_B_REMOVE;
    CB_ON_B_REMOVE m_on_b_remove_callback;
    virtual ~SBaseEffector() {}
};

struct ENGINE_API SCamEffectorInfo
{
    Fvector p;
    Fvector d;
    Fvector n;
    Fvector r;
    float fFov;
    float fNear;
    float fFar;
    float fAspect;
    float offsetX; // Required for Nvidia Ansel
    float offsetY; // Required for Nvidia Ansel
    bool dont_apply;
    bool affected_on_hud;
    SCamEffectorInfo();
    SCamEffectorInfo& operator=(const SCamEffectorInfo& other)
    {
        p = other.p;
        d = other.d;
        n = other.n;
        r = other.r;
        fFov = other.fFov;
        fNear = other.fNear;
        fFar = other.fFar;
        fAspect = other.fAspect;
        offsetX = other.offsetX;
        offsetY = other.offsetY;
        dont_apply = other.dont_apply;
        affected_on_hud = other.affected_on_hud;
        return *this;
    }
};

enum ECameraStyle
{
    csCamDebug,
    csFirstEye,
    csLookAt,
    csMax,
    csFixed,
    cs_forcedword = u32(-1)
};

enum ECamEffectorType
{
    cefDemo = 0,
    cefAnsel,
    cefNext
};

enum EEffectorPPType
{
    ppeNext = 0,
};

// refs
class ENGINE_API CCameraBase;
class ENGINE_API CEffectorCam;
class ENGINE_API CEffectorPP;

#endif
