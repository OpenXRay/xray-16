#pragma once
#include "control_combase.h"

struct SControlThreatenData : public ControlCom::IComData {
	LPCSTR	animation;
	float	time;
};

class CControlThreaten : public CControl_ComCustom<SControlThreatenData> {
	typedef CControl_ComCustom<SControlThreatenData> inherited;

public:
	virtual void	reinit					();
	virtual void	update_schedule			();
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);
	virtual void	activate				();
	virtual void	on_release				();
	virtual bool	check_start_conditions	();
};

