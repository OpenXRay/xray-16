///////////////////////////////////////////////////////////////
// BlackGraviArtifact.h
// BlackGraviArtefact - гравитационный артефакт, 
// такой же как и обычный, но при получении хита
///////////////////////////////////////////////////////////////

#pragma once
#include "GraviArtifact.h"
#include "../xrEngine/feel_touch.h"
#include "PhysicsShellHolder.h"
DEFINE_VECTOR (CPhysicsShellHolder*, GAME_OBJECT_LIST, GAME_OBJECT_LIST_it);

class CBlackGraviArtefact: public CGraviArtefact,
						   public Feel::Touch
{
private:
	collide::rq_results		rq_storage;

private:
	typedef CGraviArtefact inherited;
public:
	CBlackGraviArtefact(void);
	virtual ~CBlackGraviArtefact(void);

	virtual BOOL net_Spawn			(CSE_Abstract* DC);

	virtual void Load				(LPCSTR section);

	virtual	void Hit				(SHit* pHDS);

	virtual void feel_touch_new		(CObject* O);
	virtual void feel_touch_delete	(CObject* O);
	virtual BOOL feel_touch_contact	(CObject* O);

protected:
	virtual void	net_Relcase		(CObject* O );
	virtual void	UpdateCLChild	();

	//гравитационный удар по всем объектам в зоне дос€гаемости
	void GraviStrike();

	GAME_OBJECT_LIST m_GameObjectList;


	//которого артефакт активизируетс€
	float m_fImpulseThreshold;
	//радиус действи€ артефакта
	float m_fRadius;
	//импульс передаваемый окружающим предметам
	float m_fStrikeImpulse;

	//флаг, того что артефакт получил хит 
	//и теперь может совершить бросок
	bool m_bStrike;	

	shared_str	m_sParticleName;
};
