#pragma once

#include "CustomZone.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "ZoneVisual.h"
#include "xrPhysics/PHUpdateObject.h"

class CAmebaZone :
    public CVisualZone,
    public CPHUpdateObject
{
    typedef CVisualZone inherited;
    float m_fVelocityLimit;

public:
    CAmebaZone();
    ~CAmebaZone();
    void Affect(SZoneObjectInfo* O) override;

protected:
    void PhTune(float step) override;
    void PhDataUpdate(float step) override {}
    bool BlowoutState() override;
    void SwitchZoneState(EZoneState new_state) override;
    void Load(LPCSTR section) override;
    float distance_to_center(CGameObject* O);
};
