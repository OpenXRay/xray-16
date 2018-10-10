#pragma once
#include "CustomZone.h"

class CRadioactiveZone : public CCustomZone
{
private:
    typedef CCustomZone inherited;

public:
    CRadioactiveZone(void);
    virtual ~CRadioactiveZone(void);

    virtual void Load(LPCSTR section);
    virtual void Affect(SZoneObjectInfo* O);
    virtual void feel_touch_new(IGameObject* O);
    virtual void UpdateWorkload(u32 dt); // related to fast-mode optimizations
    virtual bool feel_touch_contact(IGameObject* O);
    float nearest_shape_radius(SZoneObjectInfo* O);

protected:
    virtual bool BlowoutState();
};
