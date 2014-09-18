#pragma once

//#include "../../../PHCharacter.h"
#include "../../../../xrphysics/IPHCapture.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterDragAbstract CStateMonsterDrag<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterDragAbstract::CStateMonsterDrag(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterDragAbstract::~CStateMonsterDrag()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::initialize()
{
	inherited::initialize();

	object->character_physics_support()->movement()->PHCaptureObject(const_cast<CEntityAlive *>(object->CorpseMan.get_corpse()));
	
	m_failed = false;
	
	IPHCapture *capture = object->character_physics_support()->movement()->PHCapture();
	if (capture && !capture->Failed()) {
		
		const CCoverPoint *point = object->CoverMan->find_cover(object->Position(), 10.f, 30.f);
		if (point) {
			m_cover_position	= point->position();
			m_cover_vertex_id	= point->level_vertex_id();
		} else {
			m_cover_vertex_id	= u32(-1);
		}
	} else m_failed = true;

	m_corpse_start_position = object->CorpseMan.get_corpse()->Position();
	
	object->path().prepare_builder();	

}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::execute()
{
	if (m_failed) return;
	
	// Установить параметры движения
	object->set_action				(ACT_DRAG);
	object->anim().SetSpecParams	(ASP_MOVE_BKWD);

	if (m_cover_vertex_id != u32(-1)) {
		object->path().set_target_point			(m_cover_position, m_cover_vertex_id);
	} else {
		object->path().set_retreat_from_point	(object->CorpseMan.get_corpse()->Position());
	}

	object->path().set_generic_parameters	();
	object->anim().accel_activate			(eAT_Calm);

}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::finalize()
{
	inherited::finalize();	

	// бросить труп
	if (object->character_physics_support()->movement()->PHCapture())
		object->character_physics_support()->movement()->PHReleaseObject();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::critical_finalize()
{
	inherited::critical_finalize();

	// бросить труп
	if (object->character_physics_support()->movement()->PHCapture())
		object->character_physics_support()->movement()->PHReleaseObject();

}

TEMPLATE_SPECIALIZATION
bool CStateMonsterDragAbstract::check_completion()
{
	if (m_failed) {
		return true;
	}

	if (!object->character_physics_support()->movement()->PHCapture())  {
		return true;
	}

	if (m_cover_vertex_id != u32(-1)) {		// valid vertex so wait path end
		if (object->Position().distance_to(m_cover_position) < 2.f) 
			return true;
	} else {								// invalid vertex so check distanced that passed
		if (m_corpse_start_position.distance_to(object->Position()) > 20.f) 
			return true;
	}

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterDragAbstract
