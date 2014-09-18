////////////////////////////////////////////////////////////////////////////
//	Module 		: level_path_builder.h
//  Modified 	: 21.02.2005
//  Modified 	: 21.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Level path builder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "movement_manager.h"
#include "level_path_manager.h"
#include "detail_path_builder.h"

class CLevelPathBuilder : public CDetailPathBuilder {
private:
	typedef CDetailPathBuilder	inherited;

private:
	Fvector						m_temp;
	u32							m_start_vertex_id;
	u32							m_dest_vertex_id;
	const Fvector				*m_precise_position;
	u32							m_last_fail_time;
	bool						m_extrapolate_path;
	bool						m_use_delay_after_fail;

private:
	enum {
		time_to_wait_after_fail	= u32(2000),
	};

public:
	IC						CLevelPathBuilder	(CMovementManager *object) :
		inherited				( object ),
		m_last_fail_time		( 0 ),
		m_use_delay_after_fail	( true )
	{
	}

	IC		const u32		&dest_vertex_id		() const
	{
		return					(m_dest_vertex_id);
	}

	IC		void			use_delay_after_fail( bool const value )
	{
		m_use_delay_after_fail	= value;
	}

	IC		void			setup				(const u32 &start_vertex_id, const u32 &dest_vertex_id, bool extrapolate_path, const Fvector *precise_position)
	{
		VERIFY				(ai().level_graph().valid_vertex_id(start_vertex_id));
		m_start_vertex_id	= start_vertex_id;
		
		VERIFY				(ai().level_graph().valid_vertex_id(dest_vertex_id));
		m_dest_vertex_id	= dest_vertex_id;

		m_extrapolate_path		= extrapolate_path;
		if (!precise_position)
			m_precise_position	= 0;
		else {
			m_temp			= *precise_position;
			m_precise_position	= &m_temp;
		}
	}

			void			register_to_process	()
	{
		m_object->m_wait_for_distributed_computation	= true;
		if ( Device.dwTimeGlobal < m_last_fail_time + time_to_wait_after_fail )
			return;

		Device.seqParallel.push_back	(fastdelegate::FastDelegate0<>(this,&CLevelPathBuilder::process));
	}

			void			process_impl		()
	{
		m_object->m_wait_for_distributed_computation	= false;
		m_object->level_path().build_path	(m_start_vertex_id,m_dest_vertex_id);

		if (m_object->level_path().failed()) {
			if ( m_use_delay_after_fail )
				m_last_fail_time			= Device.dwTimeGlobal;

			m_object->m_path_state			= CMovementManager::ePathStateBuildLevelPath;
			return;
		}

		m_object->level_path().select_intermediate_vertex();
		
		m_object->m_path_state				= CMovementManager::ePathStateBuildDetailPath;
		
		m_object->detail().set_state_patrol_path(m_extrapolate_path);
		m_object->detail().set_start_position(m_object->object().Position());
		m_object->detail().set_start_direction(Fvector().setHP(-m_object->m_body.current.yaw,0));
		
		if (m_precise_position)
			m_object->detail().set_dest_position(*m_precise_position);
		
		inherited::setup					(m_object->level_path().path(),m_object->level_path().intermediate_index());
		inherited::process_impl				(false);
	}

			void __stdcall	process				()
	{
		if ( Device.dwTimeGlobal < m_last_fail_time + time_to_wait_after_fail )
			return;

		m_object->build_level_path			();
	}

	IC		void			remove			()
	{
		if (m_object->m_wait_for_distributed_computation)
			m_object->m_wait_for_distributed_computation	= false;

		Device.remove_from_seq_parallel	(
			fastdelegate::FastDelegate0<>(
				this,
				&CLevelPathBuilder::process
			)
		);
	}
};
