////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_time_manager.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife time manager class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_interfaces.h"
#include "alife_space.h"

class CALifeTimeManager {
private:
	ALife::_TIME_ID					m_game_time;
	float							m_time_factor;
	float							m_normal_time_factor;
	u32								m_start_time;

private:
	ALife::_TIME_ID					m_start_game_time;

public:
									CALifeTimeManager		(LPCSTR		section);
	virtual							~CALifeTimeManager		();
	virtual void					save					(IWriter	&memory_stream);
	virtual void					load					(IReader	&file_stream);
			void					init					(LPCSTR		section);
	IC		void					set_time_factor			(float		time_factor);
	IC		ALife::_TIME_ID			start_game_time			() const;
	IC		ALife::_TIME_ID			game_time				() const;
	IC		float					time_factor				() const;
	IC		float					normal_time_factor		() const;
	IC		void					change_game_time		(u32 value);
};

#include "alife_time_manager_inline.h"