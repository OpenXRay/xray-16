////////////////////////////////////////////////////////////////////////////
//	Module 		: dynamic_obstacles_avoider.h
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : dynamic obstacles avoider
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "static_obstacles_avoider.h"

class dynamic_obstacles_avoider : public static_obstacles_avoider {
private:
	typedef static_obstacles_avoider		inherited;

protected:
	virtual	void		query				();
	virtual	bool		process_query		(const bool &change_path_state);

public:
			bool		movement_enabled	() const;
};

#include "dynamic_obstacles_avoider_inline.h"