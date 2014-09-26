#pragma once
#include "../../../CameraEffector.h"

class CPseudogigantStepEffector : public CEffectorCam {
	typedef CEffectorCam inherited;	

	float total;
	float max_amp;
	float period_number;
	float power;

public:
					CPseudogigantStepEffector	(float time, float amp, float periods, float power);
	virtual BOOL	ProcessCam					(SCamEffectorInfo& info);
};
