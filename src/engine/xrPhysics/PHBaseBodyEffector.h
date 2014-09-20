#ifndef PH_BASE_BODY_EFFECTOR_H
#define PH_BASE_BODY_EFFECTOR_H

//#include "ode_include.h"
#include "../3rd party/ode/include/ode/common.h"
class CPHBaseBodyEffector 
{
protected:
	dBodyID m_body;
public:
	void Init(dBodyID body)
		{
			m_body=body;
		}
};
#endif