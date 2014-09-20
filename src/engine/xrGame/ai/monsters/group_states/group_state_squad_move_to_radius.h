#pragma once
#include "../state.h"
#include "../states/state_data.h"

template<typename _Object>
class CStateGroupSquadMoveToRadiusEx : public CState<_Object> {
	typedef CState<_Object> inherited;

protected:
	
	SStateDataMoveToPointEx data;

public:
						CStateGroupSquadMoveToRadiusEx	(_Object *obj) : inherited(obj, &data) {}
	virtual				~CStateGroupSquadMoveToRadiusEx	() {}
	virtual void		initialize					();
	virtual	void		execute						();
	virtual bool		check_completion			();
	virtual void		remove_links				(CObject* object) { inherited::remove_links(object);}
};

template<typename _Object>
class CStateGroupSquadMoveToRadius : public CState<_Object> {
	typedef CState<_Object> inherited;

protected:

	SStateDataMoveToPointEx data;

public:
	CStateGroupSquadMoveToRadius	(_Object *obj) : inherited(obj, &data) {}
	virtual				~CStateGroupSquadMoveToRadius	() {}
	virtual void		initialize					();
	virtual	void		execute						();
	virtual bool		check_completion			();
	virtual void		remove_links				(CObject* object) { inherited::remove_links(object);}
};


#include "group_state_squad_move_to_radius_inline.h"
