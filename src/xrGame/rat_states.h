////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_states.h
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat states classes
////////////////////////////////////////////////////////////////////////////

#ifndef RAT_STATES_H_INCLUDED
#define RAT_STATES_H_INCLUDED

#include "rat_state_base.h"
#include "rat_state_manager.h"

struct rat_state_death : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_free_active : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_free_passive : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_attack_range : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_attack_melee : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_under_fire : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_retreat : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_pursuit : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_free_recoil : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_return_home : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_eat_corpse : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

struct rat_state_no_way : public rat_state_base {
	virtual	void	initialize	();
	virtual	void	execute		();
	virtual	void	finalize	();
};

#endif // RAT_STATES_H_INCLUDED