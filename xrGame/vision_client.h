////////////////////////////////////////////////////////////////////////////
//	Module 		: vision_client.h
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : vision client
////////////////////////////////////////////////////////////////////////////

#ifndef VISION_CLIENT_H
#define VISION_CLIENT_H

#include "../xrEngine/feel_vision.h"

class CObject;
class CEntity;
class CVisualMemoryManager;

class vision_client :
	public ISheduled,
	public Feel::Vision
{
private:
	typedef ISheduled				inherited;

private:
	CEntity							*m_object;
	CVisualMemoryManager			*m_visual;

private:
	u32								m_state;
	u32								m_time_stamp;
	Fvector							m_position;

private:
	IC		const CEntity			&object					() const;

private:
			void					eye_pp_s01				();
			void					eye_pp_s2				();

public:
									vision_client			(CEntity *object,const u32 &update_interval);
	virtual							~vision_client			();

public:
	virtual float					shedule_Scale			();
	virtual void					shedule_Update			(u32 dt);
	virtual	shared_str				shedule_Name			() const;
	virtual bool					shedule_Needed			();

public:
	virtual	float					feel_vision_mtl_transp	(CObject *object, u32 element);

public:
	virtual	BOOL					feel_vision_isRelevant	(CObject *object) = 0;
	virtual	void					camera					(
										Fvector &position,
										Fvector &direction,
										Fvector &normal,
										float &field_of_view,
										float &aspect_ratio,
										float &near_plane,
										float &far_plane
									) = 0;


public:
	virtual	void					reinit					();
	virtual	void					reload					(LPCSTR section);
			void					remove_links			(CObject *object);

public:
	IC		CVisualMemoryManager	&visual					() const;
};

#include "vision_client_inline.h"

#endif // VISION_CLIENT_H