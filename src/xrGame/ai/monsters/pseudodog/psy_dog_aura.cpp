#include "StdAfx.h"
#include "psy_dog_aura.h"
#include "psy_dog.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"
#include "Level.h"

CPPEffectorPsyDogAura::CPPEffectorPsyDogAura(const SPPInfo& ppi, u32 time_to_fade) : inherited(ppi)
{
    m_time_to_fade = time_to_fade;
    m_effector_state = eStateFadeIn;
    m_time_state_started = Device.dwTimeGlobal;
}

void CPPEffectorPsyDogAura::switch_off()
{
    m_effector_state = eStateFadeOut;
    m_time_state_started = Device.dwTimeGlobal;
}

BOOL CPPEffectorPsyDogAura::update()
{
    // update factor
    if (m_effector_state == eStatePermanent)
    {
        m_factor = 1.f;
    }
    else
    {
        m_factor = float(Device.dwTimeGlobal - m_time_state_started) / float(m_time_to_fade);
        if (m_effector_state == eStateFadeOut)
            m_factor = 1 - m_factor;

        if (m_factor > 1)
        {
            m_effector_state = eStatePermanent;
            m_factor = 1.f;
        }
        else if (m_factor < 0)
        {
            return FALSE;
        }
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

void CPsyDogAura::reinit()
{
    m_time_actor_saw_phantom = 0;
    m_time_phantom_saw_actor = 0;

    m_actor = smart_cast<CActor*>(Level().CurrentEntity());
    VERIFY(m_actor);
}

void CPsyDogAura::update_schedule()
{
    if (!m_object->g_Alive())
        return;

    m_time_phantom_saw_actor = 0;

    // check memory of actor and check memory of phantoms
    CVisualMemoryManager::VISIBLES::const_iterator I = m_actor->memory().visual().objects().begin();
    CVisualMemoryManager::VISIBLES::const_iterator E = m_actor->memory().visual().objects().end();
    for (; I != E; ++I)
    {
        const CGameObject* obj = (*I).m_object;
        if (smart_cast<const CPsyDogPhantom*>(obj))
        {
            if (m_actor->memory().visual().visible_now(obj))
                m_time_actor_saw_phantom = time();
        }
    }

    // check memory and enemy manager of phantoms whether they see actor
    xr_vector<CPsyDogPhantom*>::iterator it = m_object->m_storage.begin();
    for (; it != m_object->m_storage.end(); ++it)
    {
        if ((*it)->EnemyMan.get_enemy() == m_actor)
            m_time_phantom_saw_actor = time();
        else
        {
            ENEMIES_MAP::const_iterator I = (*it)->EnemyMemory.get_memory().begin();
            ENEMIES_MAP::const_iterator E = (*it)->EnemyMemory.get_memory().end();
            for (; I != E; ++I)
            {
                if (I->first == m_actor)
                {
                    m_time_phantom_saw_actor = _max(m_time_phantom_saw_actor, I->second.time);
                }
            }
        }

        if (m_time_phantom_saw_actor == time())
            break;
    }

    bool const close_to_actor = m_actor ? m_object->Position().distance_to(m_actor->Position()) < 30 : false;
    bool const need_be_active =
        ((m_time_actor_saw_phantom + 2000 > time()) || (m_time_phantom_saw_actor + 10000 > time())) && close_to_actor;
    if (active())
    {
        if (!need_be_active)
        {
            m_effector->switch_off();
            m_effector = 0;
        }
    }
    else
    {
        if (need_be_active)
        {
            // create effector
            m_effector = new CPPEffectorPsyDogAura(m_state, 5000);
            Actor()->Cameras().AddPPEffector(m_effector);
        }
    }
}

void CPsyDogAura::on_death()
{
    if (active())
    {
        m_effector->switch_off();
        m_effector = 0;
    }
}
