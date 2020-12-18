#include "StdAfx.h"
#include "pp_effector_custom.h"
#include "Actor.h"
#include "ActorEffector.h"

#define TRANSLATE_TYPE(val) EEffectorPPType(val ? u32(u64(typeid(this).name())) : u32(u64(this) & u32(-1)))

#pragma warning(push)
// XXX: Do something with that cheap ID generation, remove warning
#pragma warning(disable : 4355 4826) // 'this' : used in base member initializer list

CPPEffectorCustom::CPPEffectorCustom(const SPPInfo& ppi, bool one_instance, bool destroy_from_engine)
    : inherited(TRANSLATE_TYPE(one_instance), flt_max, destroy_from_engine)
{
    m_state = ppi;
    m_factor = 0.f;
    m_type = TRANSLATE_TYPE(one_instance);
}
#pragma warning(pop)

#define SET_VALUE(def, target, factor) (def + (target - def) * factor)

bool CPPEffectorCustom::Process(SPPInfo& pp)
{
    if (!inherited::Process(pp))
        return FALSE;

    // update factor
    if (!update())
        return FALSE;

    pp.lerp(pp_identity, m_state, m_factor);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////

CPPEffectorControlled::CPPEffectorControlled(
    CPPEffectorController* controller, const SPPInfo& ppi, bool one_instance, bool destroy_from_engine)
    : inherited(ppi, one_instance, destroy_from_engine)
{
    m_controller = controller;
}
bool CPPEffectorControlled::update()
{
    m_controller->update_factor();
    return TRUE;
}

CPPEffectorController::CPPEffectorController() {}
CPPEffectorController::~CPPEffectorController()
{
    if (m_effector)
    {
        Actor()->Cameras().RemovePPEffector(m_effector->get_type());
    }
}

void CPPEffectorController::activate()
{
    VERIFY(!m_effector);

    m_effector = create_effector();
    Actor()->Cameras().AddPPEffector(m_effector);
}

void CPPEffectorController::deactivate()
{
    VERIFY(m_effector);

    Actor()->Cameras().RemovePPEffector(m_effector->get_type());
    m_effector = 0;
}

void CPPEffectorController::frame_update()
{
    if (m_effector)
    {
        if (check_completion())
            deactivate();
    }
    else if (check_start_conditions())
        activate();
}
