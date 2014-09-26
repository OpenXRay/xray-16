///////////////////////////////////////////////////////////////
// GrenadeLauncher.h
// GrenadeLauncher - апгрейд оружия поствольный гранатомет
///////////////////////////////////////////////////////////////

#pragma once
#include "inventory_item_object.h"

class CGrenadeLauncher : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CGrenadeLauncher (void);
	virtual ~CGrenadeLauncher(void);

	virtual BOOL net_Spawn			(CSE_Abstract* DC);
	virtual void Load				(LPCSTR section);
	virtual void net_Destroy		();

	virtual void OnH_A_Chield		();
	virtual void OnH_B_Independent	(bool just_before_destroy);

	virtual void UpdateCL			();
	virtual void renderable_Render	();

	float	GetGrenadeVel() {return m_fGrenadeVel;}

protected:
	//стартовая скорость вылета подствольной гранаты
	float m_fGrenadeVel;
};