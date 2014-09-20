#pragma once
#include "state_defs.h"
#include "control_com_defs.h"

// Lain: added
#ifdef DEBUG
namespace debug { class text_tree; }
#endif

class IStateManagerBase {
public:
	virtual					~IStateManagerBase		()						{};
	virtual void			reinit					()						= 0;
	virtual void			update					()						= 0;
	virtual void			force_script_state		(EMonsterState state)	= 0;
	virtual void			execute_script_state	()						= 0;
	virtual	void			critical_finalize		()						= 0;
	virtual void			remove_links			(CObject *O)			= 0;
	virtual	EMonsterState	get_state_type			()						= 0;

	virtual bool			check_control_start_conditions (ControlCom::EControlType type) = 0;

// Lain: added
#ifdef DEBUG
	virtual void            add_debug_info          (debug::text_tree& root_s) {}
#endif
};
