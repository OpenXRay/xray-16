#include "StdAfx.h"
#include "rewarding_events_handlers.h"
#include "reward_event_handler.h"
#include "Common/object_broker.h"

namespace award_system
{
rewarding_event_handlers::rewarding_event_handlers(
    game_state_accumulator* pstate_accum, event_action_delegate_t ea_delegate)
    : m_player_state_accum(pstate_accum), m_reward_action(ea_delegate), m_null_hanlder(NULL)
{
}

rewarding_event_handlers::~rewarding_event_handlers() { delete_data(m_events_store); }
void rewarding_event_handlers::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnWeapon_Fire(sender, sender_weapon_id))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnWeapon_Fire(sender, sender_weapon_id);
}

void rewarding_event_handlers::OnBullet_Fire(
    u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnBullet_Fire(sender, sender_weapon_id, position, direction))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnBullet_Fire(sender, sender_weapon_id, position, direction);
}

void rewarding_event_handlers::OnBullet_Hit(
    IGameObject const* hitter, IGameObject const* victim, IGameObject const* weapon, u16 const bone)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnBullet_Hit(hitter, victim, weapon, bone))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnBullet_Hit(hitter, victim, weapon, bone);
}
void rewarding_event_handlers::OnArtefactSpawned()
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnArtefactSpawned())
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnArtefactSpawned();
}

void rewarding_event_handlers::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerTakeArtefact(ps))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerTakeArtefact(ps);
}

void rewarding_event_handlers::OnPlayerDropArtefact(game_PlayerState const* ps)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerDropArtefact(ps))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerDropArtefact(ps);
}

void rewarding_event_handlers::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerBringArtefact(ps))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerBringArtefact(ps);
}

void rewarding_event_handlers::OnPlayerSpawned(game_PlayerState const* ps)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerSpawned(ps))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerSpawned(ps);
}

void rewarding_event_handlers::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerKilled(killer_id, target_id, weapon_id, kill_type))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerKilled(killer_id, target_id, weapon_id, kill_type);
}
void rewarding_event_handlers::OnPlayerChangeTeam(s8 team)
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerChangeTeam(team))
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerChangeTeam(team);
}

void rewarding_event_handlers::OnPlayerRankChanged()
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnPlayerRankChanged())
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnPlayerRankChanged();
}

void rewarding_event_handlers::OnRoundEnd()
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnRoundEnd())
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnRoundEnd();
}

void rewarding_event_handlers::OnRoundStart()
{
    for (handlers_store_t::iterator i = m_events_store.begin(), ie = m_events_store.end(); i != ie; ++i)
    {
        if (i->second->OnRoundStart())
        {
            m_reward_action(i->first);
            break;
        }
    }
    if (m_null_hanlder)
        m_null_hanlder->OnRoundStart();
}

void rewarding_event_handlers::set_null_handler(reward_event_handler* new_handler)
{
    VERIFY(new_handler);
    m_null_hanlder = new_handler;
}

void rewarding_event_handlers::init() {}
} // namespace award_system
