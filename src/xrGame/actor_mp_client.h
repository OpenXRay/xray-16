#ifndef ACTOR_MP_CLIENT_H
#define ACTOR_MP_CLIENT_H

#include "Actor.h"
#include "actor_mp_state.h"
#include "anticheat_dumpable_object.h"

class CActorMP : public IAnticheatDumpable, public CActor
{
private:
    typedef CActor inherited;

private:
    actor_mp_state_holder m_state_holder;
    // bool					m_i_am_dead;
    float prev_cam_inert_value;
    static const float cam_inert_value;

private:
    void fill_state(actor_mp_state& state);
    void process_packet(net_update& N);
    void postprocess_packet(net_update_A& packet);

public:
    CActorMP();
    virtual void net_Export(NET_Packet& packet);
    virtual void net_Import(NET_Packet& packet);
    virtual BOOL net_Relevant();
    virtual void OnEvent(NET_Packet& packet, u16 type);
    virtual void Die(IGameObject* killer);
    virtual void DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const;
    shared_str const GetAnticheatSectionName() const { return "mp_actor"; };
    virtual void On_SetEntity();
    virtual void On_LostEntity();

protected:
    virtual void cam_Set(EActorCameras style);
    void use_booster(NET_Packet& packet);
};

#endif // ACTOR_MP_CLIENT_H
