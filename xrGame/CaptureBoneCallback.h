#pragma once;


#include "../xrphysics/physicsshell.h"

struct	CPHCaptureBoneCallback :
	public NearestToPointCallback
{
	virtual	bool operator() ( u16 bid )		= 0;
	virtual	bool operator() ( CPhysicsElement* e )	
	{
		return (*this) ( e->m_SelfID );
	};
};