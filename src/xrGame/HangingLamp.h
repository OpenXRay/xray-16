// DummyObject.h: interface for the CHangingLamp class.
//
//////////////////////////////////////////////////////////////////////

#ifndef HangingLampH
#define HangingLampH
#pragma once

#include "GameObject.h"
#include "PhysicsShellHolder.h"
#include "PHSkeleton.h"

// refs
class CLAItem;
class CPhysicsElement;
class CSE_ALifeObjectHangingLamp;
class CPHElement;
class CHangingLamp : public CPhysicsShellHolder, public CPHSkeleton
{ // need m_pPhysicShell
    typedef CPhysicsShellHolder inherited;

private:
    u16 light_bone;
    u16 ambient_bone;

    ref_light light_render;
    ref_light light_ambient;
    CLAItem* lanim;
    float ambient_power;
    BOOL m_bState;

    ref_glow glow_render;

    float fHealth;
    float fBrightness;
    void CreateBody(CSE_ALifeObjectHangingLamp* lamp);
    void Init();
    void RespawnInit();
    bool Alive() { return fHealth > 0.f; }
public:
    CHangingLamp();
    virtual ~CHangingLamp();
    void TurnOn();
    void TurnOff();
    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void shedule_Update(u32 dt); // Called by sheduler
    virtual void UpdateCL(); // Called each frame, so no need for dt

    virtual void SpawnInitPhysics(CSE_Abstract* D);
    virtual CPhysicsShellHolder* PPhysicsShellHolder() { return PhysicsShellHolder(); };
    virtual void CopySpawnInit();
    virtual void net_Save(NET_Packet& P);
    virtual BOOL net_SaveRelevant();
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    virtual BOOL renderable_ShadowGenerate() { return TRUE; }
    virtual BOOL renderable_ShadowReceive() { return TRUE; }
    virtual void Hit(SHit* pHDS);
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);
    virtual BOOL UsedAI_Locations();

    virtual void Center(Fvector& C) const;
    virtual float Radius() const;
};

#endif // HangingLampH
