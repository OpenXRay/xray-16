///////////////////////////////////////////////////////////////
// Silencer.h
// Silencer - апгрейд оружия глушитель 
///////////////////////////////////////////////////////////////

#pragma once
#include "inventory_item_object.h"

class CSilencer : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CSilencer (void);
	virtual ~CSilencer(void);

	virtual BOOL net_Spawn			(CSE_Abstract* DC);
	virtual void Load				(LPCSTR section);
	virtual void net_Destroy		();

	virtual void OnH_A_Chield		();
	virtual void OnH_B_Independent	(bool just_before_destroy);

	virtual void UpdateCL			();
	virtual void renderable_Render	();

};