#pragma once
#include "control_combase.h"

struct SControlCriticalWoundData : public ControlCom::IComData {
	LPCSTR	animation;
};

class CControlCriticalWound : public CControl_ComCustom<SControlCriticalWoundData> {
	typedef CControl_ComCustom<SControlCriticalWoundData> inherited;

public:
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);
	virtual void	activate				();
	virtual void	on_release				();
	virtual bool	check_start_conditions	();
};

