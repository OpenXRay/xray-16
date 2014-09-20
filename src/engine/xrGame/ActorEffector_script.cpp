#include "pch_script.h"
#include "ai_space.h"
#include "script_engine.h"
#include "ActorEffector.h"
#include "../xrEngine/ObjectAnimator.h"

void CAnimatorCamEffectorScriptCB::ProcessIfInvalid(SCamEffectorInfo& info)
{
	if(m_bAbsolutePositioning)
	{
		const Fmatrix& m			= m_objectAnimator->XFORM();
		info.d						= m.k;
		info.n						= m.j;
		info.p						= m.c;
		if(m_fov>0.0f)
			info.fFov				= m_fov;
	}
}

BOOL CAnimatorCamEffectorScriptCB::Valid()
{
	BOOL res = inherited::Valid();
	if(!res && cb_name.size() )
	{
		luabind::functor<LPCSTR>			fl;
		R_ASSERT							(ai().script_engine().functor<LPCSTR>(*cb_name,fl));
		fl									();
		cb_name								= "";
	}
	return res;
}
