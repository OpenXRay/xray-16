////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_update_manager.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator update manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_switch_manager.h"
#include "alife_surge_manager.h"
#include "alife_storage_manager.h"

namespace RestrictionSpace {
	enum ERestrictorTypes;
}

class CALifeUpdateManager :
	public CALifeSwitchManager,
	public CALifeSurgeManager,
	public CALifeStorageManager,
	public ISheduled
{
private:
	bool				m_first_time;

protected:
	u64					m_max_process_time;
	float				m_update_monster_factor;
	u32					m_objects_per_update;
	bool				m_changing_level;

public:
			void __stdcall	update				();

protected:
			void		new_game				(LPCSTR	save_name);
			void		init_ef_storage			() const;
	virtual	void		reload					(LPCSTR section);

public:
						CALifeUpdateManager		(xrServer *server, LPCSTR section);
	virtual 			~CALifeUpdateManager	();
	virtual	shared_str	shedule_Name			() const		{ return shared_str("alife_simulator"); };
	virtual float		shedule_Scale			();
	virtual void		shedule_Update			(u32 dt);	
	virtual bool		shedule_Needed			()				{return true;};
			void		update_switch			();
			void		update_scheduled		(bool init_ef = true);
			void		load					(LPCSTR game_name = 0, bool no_assert = false, bool new_only = false);
			bool		load_game				(LPCSTR game_name, bool no_assert = false);
	IC		float		update_monster_factor	() const;
			bool		change_level			(NET_Packet	&net_packet);
			void		set_process_time		(int microseconds);
			void		objects_per_update		(const u32 &objects_per_update);
			void		set_switch_online		(ALife::_OBJECT_ID id, bool value);
			void		set_switch_offline		(ALife::_OBJECT_ID id, bool value);
			void		set_interactive			(ALife::_OBJECT_ID id, bool value);
			void		jump_to_level			(LPCSTR level_name) const;
			void		teleport_object			(ALife::_OBJECT_ID id, GameGraph::_GRAPH_ID game_vertex_id, u32 level_vertex_id, const Fvector &position);
			void		add_restriction			(ALife::_OBJECT_ID id, ALife::_OBJECT_ID restriction_id, const RestrictionSpace::ERestrictorTypes &restriction_type);
			void		remove_restriction		(ALife::_OBJECT_ID id, ALife::_OBJECT_ID restriction_id, const RestrictionSpace::ERestrictorTypes &restriction_type);
			void		remove_all_restrictions	(ALife::_OBJECT_ID id, const RestrictionSpace::ERestrictorTypes &restriction_type);
};

#include "alife_update_manager_inline.h"