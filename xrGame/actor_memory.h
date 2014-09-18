////////////////////////////////////////////////////////////////////////////
//	Module 		: actor_memory.h
//	Created 	: 15.09.2005
//  Modified 	: 15.09.2005
//	Author		: Dmitriy Iassenev
//	Description : actor memory
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "vision_client.h"

class CActor;

class CActorMemory : public vision_client {
private:
	typedef vision_client					inherited;

private:
	CActor			*m_actor;

public:
					CActorMemory			(CActor *actor);
	virtual	BOOL	feel_vision_isRelevant	(CObject *object);
	virtual	void	camera					(
						Fvector &position,
						Fvector &direction,
						Fvector &normal,
						float &field_of_view,
						float &aspect_ratio,
						float &near_plane,
						float &far_plane
					);
};
