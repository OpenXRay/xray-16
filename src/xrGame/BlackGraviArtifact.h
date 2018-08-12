///////////////////////////////////////////////////////////////
// BlackGraviArtifact.h
// BlackGraviArtefact - гравитационный артефакт,
// такой же как и обычный, но при получении хита
///////////////////////////////////////////////////////////////

#pragma once
#include "GraviArtifact.h"
#include "xrEngine/Feel_Touch.h"
#include "PhysicsShellHolder.h"
#include "xrCDB/xr_collide_defs.h"

using GAME_OBJECT_LIST = xr_vector<CPhysicsShellHolder*>;

class CBlackGraviArtefact : public CGraviArtefact, public Feel::Touch
{
private:
    collide::rq_results rq_storage;

private:
    typedef CGraviArtefact inherited;

public:
    CBlackGraviArtefact(void);
    virtual ~CBlackGraviArtefact(void);

    virtual BOOL net_Spawn(CSE_Abstract* DC);

    virtual void Load(LPCSTR section);

    virtual void Hit(SHit* pHDS);

    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);

protected:
    virtual void net_Relcase(IGameObject* O);
    virtual void UpdateCLChild();

    //гравитационный удар по всем объектам в зоне досягаемости
    void GraviStrike();

    GAME_OBJECT_LIST m_GameObjectList;

    //которого артефакт активизируется
    float m_fImpulseThreshold;
    //радиус действия артефакта
    float m_fRadius;
    //импульс передаваемый окружающим предметам
    float m_fStrikeImpulse;

    //флаг, того что артефакт получил хит
    //и теперь может совершить бросок
    bool m_bStrike;

    shared_str m_sParticleName;
};
