//////////////////////////////////////////////////////////////////////////
// GraviZone.h:		гравитационная аномалия
//////////////////////////////////////////////////////////////////////////
//					состоит как бы из 2х зон
//					одна затягивает объект, другая взрывает и
//					все неживые объекты (предметы и трупы)
//					поднимает в воздух и качает там какое-то
//					время
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "CustomZone.h"
#include "ai/monsters/telekinesis.h"

class CBaseGraviZone : public CCustomZone
{
    typedef CCustomZone inherited;

public:
    CBaseGraviZone(void);
    virtual ~CBaseGraviZone(void);

    virtual void Load(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Relcase(IGameObject* O);

    //воздействие зоной на объект
    virtual void Affect(SZoneObjectInfo* O);
    virtual void AffectPull(CPhysicsShellHolder* GO, const Fvector& throw_in_dir, float dist);
    virtual void AffectPullAlife(CEntityAlive* EA, const Fvector& throw_in_dir, float dist);
    virtual void AffectPullDead(CPhysicsShellHolder* GO, const Fvector& throw_in_dir, float dist);
    virtual void AffectThrow(SZoneObjectInfo* O, CPhysicsShellHolder* GO, const Fvector& throw_in_dir, float dist);
    virtual void ThrowInCenter(Fvector& C);
    virtual bool CheckAffectField(CPhysicsShellHolder* GO, float dist_to_radius);
    virtual void shedule_Update(u32 dt);
    virtual bool BlowoutState();
    virtual bool IdleState();

    virtual float BlowoutRadiusPercent(CPhysicsShellHolder* /*GO*/) { return m_fBlowoutRadiusPercent; }
protected:
    virtual CTelekinesis& Telekinesis() = 0;

protected:
    //сила импульса втягивания в зону (для веса 100 кг)
    float m_fThrowInImpulse;
    //сила импульса втягивания в зону для живых существ
    float m_fThrowInImpulseAlive;
    //коэфф. затягивания (чем меньше, тем плавнее затягивает)
    float m_fThrowInAtten;
    //радиус действия выброса (в процентах от всего)
    float m_fBlowoutRadiusPercent;

    //параметры телекинеза
    float m_fTeleHeight;
    u32 m_dwTimeToTele;
    u32 m_dwTelePause;
    u32 m_dwTeleTime;

    //имя партиклов телекинеза
    void PlayTeleParticles(CGameObject* pObject);
    void StopTeleParticles(CGameObject* pObject);

    shared_str m_sTeleParticlesBig;
    shared_str m_sTeleParticlesSmall;
};

class CGraviZone : public CBaseGraviZone
{
    typedef CBaseGraviZone inherited;
    CTelekinesis m_telekinesis;

protected:
    virtual CTelekinesis& Telekinesis() { return m_telekinesis; }
public:
    CGraviZone(void) {}
    virtual ~CGraviZone(void) {}
};
