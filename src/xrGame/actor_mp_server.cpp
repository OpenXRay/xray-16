#include "StdAfx.h"
#include "actor_mp_server.h"

CSE_ActorMP::CSE_ActorMP(LPCSTR section) : inherited(section) { m_ready_to_update = false; }
void CSE_ActorMP::STATE_Read(NET_Packet& packet, u16 size)
{
    inherited::STATE_Read(packet, size);

#ifdef DEBUG
    Msg("--- Actor %d[%s] STATE_Read, health is: %2.04f", this->ID, this->name_replace(),
        m_state_holder.state().health);
#endif // #ifdef DEBUG
}

void CSE_ActorMP::STATE_Write(NET_Packet& packet)
{
    inherited::STATE_Write(packet);
#ifdef DEBUG
    Msg("--- Actor %d[%s] STATE_Write, health is: %2.04f", this->ID, this->name_replace(),
        m_state_holder.state().health);
#endif // #ifdef DEBUG
}

BOOL CSE_ActorMP::Net_Relevant()
{
    if (get_health() <= 0)
        return (false);
    return (inherited::Net_Relevant());
}

#ifdef XRGAME_EXPORTS
void CSE_ActorMP::on_death(CSE_Abstract* killer)
{
    inherited::on_death(killer);

    actor_mp_state state;
    fill_state(state);
    m_state_holder.relevant(state);
}
#endif
