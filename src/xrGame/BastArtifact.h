///////////////////////////////////////////////////////////////
// BastArtifact.h
// BastArtefact - артефакт мочалка
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"
#include "xrEngine/Feel_Touch.h"

#include "entity_alive.h"

struct SGameMtl;
struct dContact;

using ALIVE_LIST = xr_vector<CEntityAlive*>;

class CBastArtefact : public CArtefact, public Feel::Touch
{
private:
    typedef CArtefact inherited;

public:
    CBastArtefact(void);
    virtual ~CBastArtefact(void);

    virtual void Load(LPCSTR section);
    virtual void shedule_Update(u32 dt);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();

    virtual void Hit(SHit* pHDS);

    virtual bool Useful() const;

    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);

    bool IsAttacking() { return NULL != m_AttakingEntity; }
protected:
    virtual void UpdateCLChild();

    static void ObjectContactCallback(
        bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/);
    //столкновение мочалки с сущностью
    void BastCollision(CEntityAlive* pEntityAlive);

    //параметры артефакта

    //пороговое значение импульса после получения
    //которого артефакт активизируется
    float m_fImpulseThreshold;

    float m_fEnergy;
    float m_fEnergyMax;
    float m_fEnergyDecreasePerTime;
    shared_str m_sParticleName;

    float m_fRadius;
    float m_fStrikeImpulse;

    //флаг, того что артефакт получил хит
    //и теперь может совершить бросок
    bool m_bStrike;

    //список живых существ в зоне досягаемости артефакта
    ALIVE_LIST m_AliveList;
    //то, что мы ударили
    CEntityAlive* m_pHitedEntity;
    //то что атакуем
    CEntityAlive* m_AttakingEntity;

public:
    virtual void setup_physic_shell();
};
