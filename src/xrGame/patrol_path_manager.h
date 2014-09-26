////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_manager.h
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Patrol path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "level.h"
#include "script_engine.h"
#include "patrol_path_storage.h"
#include "patrol_path.h"
#include "patrol_path_manager_space.h"
#include "script_callback_ex.h"

template <typename _return_type>
class CScriptCallbackEx;

class CRestrictedObject;
class CGameObject;

using namespace PatrolPathManager;

class CPatrolPathManager {
private:
	friend struct CAccessabilityEvaluator;

private:
	typedef CScriptCallbackEx<bool>	CExtrapolateCallback;

private:
	const CPatrolPath				*m_path;
	shared_str						m_path_name;
	EPatrolStartType				m_start_type;
	EPatrolRouteType				m_route_type;
	bool							m_actuality;
	bool							m_failed;
	bool							m_completed;
	bool							m_random;
	u32								m_curr_point_index;
	u32								m_prev_point_index;
	u32								m_start_point_index;
	Fvector							m_dest_position;
	CExtrapolateCallback			m_extrapolate_callback;
	CRestrictedObject				*m_object;
	CGameObject						*m_game_object;

protected:
	IC			bool				random					() const;
	IC			bool				accessible				(const Fvector &position) const;
	IC			bool				accessible				(u32 vertex_id) const;
	IC			bool				accessible				(const CPatrolPath::CVertex *vertex) const;

public:
	IC								CPatrolPathManager		(CRestrictedObject *object, CGameObject *game_object);
		virtual						~CPatrolPathManager		();
		virtual	void				reinit					();
	IC			CExtrapolateCallback&extrapolate_callback	();
	IC			void				make_inactual			();
	IC			const CPatrolPath	*get_path				() const;
	IC			void				set_path				(const CPatrolPath *path, shared_str path_name);
	IC			void				set_path				(shared_str path_name);
	IC			void				set_path				(shared_str path_name, const EPatrolStartType patrol_start_type = ePatrolStartTypeNearest, const EPatrolRouteType patrol_route_type = ePatrolRouteTypeContinue, bool random = true);
	IC			void				set_start_type			(const EPatrolStartType patrol_start_type);
	IC			void				set_route_type			(const EPatrolRouteType patrol_route_type);
	IC			void				set_random				(bool random);
	IC			bool				actual					() const;
				shared_str			path_name				() const;
				void				set_previous_point		(int point_index);
				void				set_start_point			(int point_index);
	IC			bool				completed				() const;
	IC			bool				failed					() const;
				void				select_point			(const Fvector &position, u32 &dest_vertex_id);
	IC			const Fvector		&destination_position	() const;
	IC			u32					get_current_point_index	() const;
	IC			CRestrictedObject	&object					() const;
				bool				extrapolate_path		();

private:
				u32					get_next_point			(u32 prev_point_index);
				void				reset					();
};

#include "patrol_path_manager_inline.h"