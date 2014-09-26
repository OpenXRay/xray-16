#pragma once

#include "control_combase.h"
#include "../../../Include/xrRender/KinematicsAnimated.h"

struct SAnimationSequencerData : public ControlCom::IComData {
	xr_vector<MotionID>	motions;
};
	
class CAnimationSequencer : public CControl_ComCustom<SAnimationSequencerData> {
	u32						m_index;
public:
	virtual void	reset_data				();
	virtual	void	on_capture				();
	virtual void	on_release				();
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);

	virtual bool	check_start_conditions	();

	virtual void	activate				();
private:
			void	play_selected			();
};

