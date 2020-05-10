#pragma once

#include "WeaponCustomPistol.h"

class CUIFrameWindow;
class CUIStatic;
class CBinocularsVision;

class CWeaponBinoculars : public CWeaponCustomPistol
{
private:
    typedef CWeaponCustomPistol inherited;

protected:
    bool m_bVision;

public:
    CWeaponBinoculars();
    virtual ~CWeaponBinoculars();

    void Load(LPCSTR section);

    virtual void OnZoomIn();
    virtual void OnZoomOut();
    virtual void ZoomInc();
    virtual void ZoomDec();
    virtual void net_Destroy();
    virtual bool net_Spawn(CSE_Abstract* DC);
    bool can_kill() const;
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual bool Action(u16 cmd, u32 flags);
    virtual void UpdateCL();
    virtual void render_item_ui();
    virtual bool render_item_ui_query();
    virtual bool use_crosshair() const { return false; }
    virtual bool GetBriefInfo(II_BriefInfo& info);
    virtual void net_Relcase(IGameObject* object);

protected:
    CBinocularsVision* m_binoc_vision;
};
