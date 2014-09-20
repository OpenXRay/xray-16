#pragma once
#include "../control_combase.h"

class CBurerFastGravi : public CControl_ComCustom<> {
	typedef CControl_ComCustom<> inherited;

public:
	
	virtual bool	check_start_conditions	();
	virtual void	activate				();
	virtual void	deactivate				();
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);
	
private:	
			void	process_hit				();

};

