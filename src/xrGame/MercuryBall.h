///////////////////////////////////////////////////////////////
// MercuryBall.h
// MercuryBall - переливающийся и колыхающийся шар
// перекатывается с места на место
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"

class CMercuryBall : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CMercuryBall(void);
    virtual ~CMercuryBall(void);

    virtual void Load(LPCSTR section);

protected:
    virtual void UpdateCLChild();

    //время последнего обновления поведения шара
    ALife::_TIME_ID m_timeLastUpdate;
    //время между апдейтами
    ALife::_TIME_ID m_timeToUpdate;

    //диапазон импульсов катания шара
    float m_fImpulseMin;
    float m_fImpulseMax;
};

/*

#pragma once
#include "GameObject.h"
#include "PhysicsShell.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Ртутный шар
// Появляется после выброса, держится недолго, после чего испаряется.
// Цены:  от 50 до 200 рублей, в зависимости от размера
// Специфика: опасное аномальное образование, хранить только в защищенном контейнере,
// например в капсуле R1.
class CMercuryBall : public CGameObject {
typedef	CGameObject	inherited;
public:
    CMercuryBall(void);
    virtual ~CMercuryBall(void);

    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);


    virtual BOOL			net_Spawn			(CSE_Abstract* DC);
};
*/
