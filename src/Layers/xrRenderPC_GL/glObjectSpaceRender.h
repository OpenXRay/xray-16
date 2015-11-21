#pragma once

#ifdef DEBUG

#include "..\..\Include\xrRender\ObjectSpaceRender.h"

class glObjectSpaceRender : public IObjectSpaceRender
{
public:
	glObjectSpaceRender();
	virtual ~glObjectSpaceRender();
	virtual void Copy(IObjectSpaceRender &_in);

	virtual void dbgRender();
	virtual void dbgAddSphere(const Fsphere &sphere, u32 colour);
	virtual void SetShader();

private:
	ref_shader							m_shDebug;
	clQueryCollision					q_debug;			// MT: dangerous
	xr_vector<std::pair<Fsphere, u32> >	dbg_S;				// MT: dangerous
};

#endif // DEBUG
