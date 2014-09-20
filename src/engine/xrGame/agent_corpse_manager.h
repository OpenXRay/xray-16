////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_corpse_manager.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent corpse manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "member_corpse.h"

class CAgentManager;
class CMemberOrder;

class CAgentCorpseManager {
public:
	typedef xr_vector<CMemberCorpse> MEMBER_CORPSES;

private:
	MEMBER_CORPSES			m_corpses;
	CAgentManager			*m_object;

protected:
	IC		CAgentManager	&object					() const;
			bool			process_corpse			(CMemberOrder &member);

public:
	IC						CAgentCorpseManager		(CAgentManager *object);
	IC		void			register_corpse			(CAI_Stalker *corpse);
	IC		MEMBER_CORPSES	&corpses				();
	IC		void			clear					();
			void			react_on_member_death	();
			void			update					();
			void			remove_links			(CObject *object);
};

#include "agent_corpse_manager_inline.h"