#pragma once

#include "xrPhysics/IPHCapture.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterDragAbstract CStateMonsterDrag<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterDragAbstract::CStateMonsterDrag(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
CStateMonsterDragAbstract::~CStateMonsterDrag() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::initialize()
{
    inherited::initialize();

    this->object->character_physics_support()->movement()->PHCaptureObject(
        const_cast<CEntityAlive*>(this->object->CorpseMan.get_corpse()));

    m_failed = false;

    IPHCapture* capture = this->object->character_physics_support()->movement()->PHCapture();
    if (capture && !capture->Failed())
    {
        const CCoverPoint* point = this->object->CoverMan->find_cover(this->object->Position(), 10.f, 30.f);
        if (point)
        {
            m_cover_position = point->position();
            m_cover_vertex_id = point->level_vertex_id();
        }
        else
        {
            m_cover_vertex_id = u32(-1);
        }
    }
    else
        m_failed = true;

    m_corpse_start_position = this->object->CorpseMan.get_corpse()->Position();

    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::execute()
{
    if (m_failed)
        return;

    // Установить параметры движения
    this->object->set_action(ACT_DRAG);
    this->object->anim().SetSpecParams(ASP_MOVE_BKWD);

    if (m_cover_vertex_id != u32(-1))
    {
        this->object->path().set_target_point(m_cover_position, m_cover_vertex_id);
    }
    else
    {
        this->object->path().set_retreat_from_point(this->object->CorpseMan.get_corpse()->Position());
    }

    this->object->path().set_generic_parameters();
    this->object->anim().accel_activate(eAT_Calm);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::finalize()
{
    inherited::finalize();

    // бросить труп
    if (this->object->character_physics_support()->movement()->PHCapture())
        this->object->character_physics_support()->movement()->PHReleaseObject();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterDragAbstract::critical_finalize()
{
    inherited::critical_finalize();

    // бросить труп
    if (this->object->character_physics_support()->movement()->PHCapture())
        this->object->character_physics_support()->movement()->PHReleaseObject();
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterDragAbstract::check_completion()
{
    if (m_failed)
    {
        return true;
    }

    if (!this->object->character_physics_support()->movement()->PHCapture())
    {
        return true;
    }

    if (m_cover_vertex_id != u32(-1))
    { // valid vertex so wait path end
        if (this->object->Position().distance_to(m_cover_position) < 2.f)
            return true;
    }
    else
    { // invalid vertex so check distanced that passed
        if (m_corpse_start_position.distance_to(this->object->Position()) > 20.f)
            return true;
    }

    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterDragAbstract
