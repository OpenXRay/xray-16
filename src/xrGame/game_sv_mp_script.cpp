#include "pch_script.h"
#include "game_sv_mp_script.h"
#include "xrServer_script_macroses.h"
#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "Level.h"
#include "ai_space.h"
#include "script_engine.h"

using namespace luabind;

void game_sv_mp_script::SetHitParams(NET_Packet* P, float impulse, float power)
{
    u32 PowRPos = 16;
    u32 ImpRPos = 34;

    u32 bk = P->B.count;

    P->B.count = PowRPos;
    P->w_float(power);

    P->B.count = ImpRPos;
    P->w_float(impulse);

    P->B.count = bk;
}

float game_sv_mp_script::GetHitParamsPower(NET_Packet* P)
{
    u32 PowRPos = 16;
    u32 bk = P->r_pos;

    P->r_pos = PowRPos;
    float res = P->r_float();
    P->r_pos = bk;
    return res;
}

float game_sv_mp_script::GetHitParamsImpulse(NET_Packet* P)
{
    u32 ImpRPos = 34;
    u32 bk = P->r_pos;

    P->r_pos = ImpRPos;
    float res = P->r_float();
    P->r_pos = bk;
    return res;
}

void game_sv_mp_script::Create(shared_str& options)
{
    inherited::Create(options);
    LPCSTR lpcstr_options = options.c_str();
    Create(lpcstr_options);
}

void game_sv_mp_script::SpawnPlayer(ClientID id, LPCSTR N, LPCSTR SkinName, RPoint rp)
{
    xrClientData* CL = m_server->ID_to_client(id);
    game_PlayerState* ps_who = CL->ps;
    ps_who->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);

    CSE_Abstract* pOldOwner = CL->owner;
    if (pOldOwner && pOldOwner->owner == CL)
    {
        CSE_ALifeCreatureActor* pOldActor = smart_cast<CSE_ALifeCreatureActor*>(pOldOwner);
        CSE_Spectator* pOldSpectator = smart_cast<CSE_Spectator*>(pOldOwner);

        if (pOldActor)
        {
            AllowDeadBodyRemove(id, pOldActor->ID);
            m_CorpseList.push_back(pOldOwner->ID);
        };
        if (pOldSpectator)
        {
            pOldSpectator->owner = (xrClientData*)m_server->GetServerClient();
            NET_Packet P;
            u_EventGen(P, GE_DESTROY, pOldSpectator->ID);
            Level().Send(P, net_flags(TRUE, TRUE));
        };
    }

    // Spawn
    CSE_Abstract* E = spawn_begin(N); // create SE

    E->set_name_replace(get_name_id(id)); // name

    E->s_flags.assign(M_SPAWN_OBJECT_LOCAL | M_SPAWN_OBJECT_ASPLAYER); // flags

    CSE_ALifeCreatureActor* pA = smart_cast<CSE_ALifeCreatureActor*>(E);
    CSE_Spectator* pS = smart_cast<CSE_Spectator*>(E);

    R_ASSERT2(pA || pS, "Respawned Client is not Actor nor Spectator");

    if (pA)
    {
        pA->s_team = u8(ps_who->team);

        if (xr_strlen(SkinName) != 0)
            pA->set_visual(SkinName);

        ps_who->resetFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
        ps_who->RespawnTime = Device.dwTimeGlobal;

        E->o_Position.set(rp.P);
        E->o_Angle.set(rp.A);
    }
    else if (pS)
    {
        Fvector Pos, Angle;
        ///			ps_who->setFlag(GAME_PLAYER_FLAG_CS_SPECTATOR);
        if (!GetPosAngleFromActor(id, Pos, Angle))
            assign_RP(E, ps_who);
        else
        {
            E->o_Angle.set(Angle);
            E->o_Position.set(Pos);
        }
    };

    Msg("* %s respawned as %s", get_name_id(id), (0 == pA) ? "spectator" : "actor");
    spawn_end(E, id);

    ps_who->SetGameID(CL->owner->ID);

    CL->owner->owner = CL;

    signal_Syncronize();
}

void game_sv_mp_script::switch_Phase(u32 new_phase) { inherited::switch_Phase(new_phase); }
void game_sv_mp_script::net_Export_State(NET_Packet& P, ClientID id_to) { inherited::net_Export_State(P, id_to); };
void game_sv_mp_script::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
    inherited::OnEvent(P, type, time, sender);
};
void game_sv_mp_script::OnPlayerConnect(ClientID id_who) { inherited::OnPlayerConnect(id_who); };
void game_sv_mp_script::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
    inherited::OnPlayerDisconnect(id_who, Name, GameID);
};

#pragma warning(push)
#pragma warning(disable : 4709)

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base
{
    typedef T inherited;
    typedef CWrapperBase<T> self_type;
    DEFINE_LUA_WRAPPER_CONST_METHOD_0(type_name, LPCSTR)

    DEFINE_LUA_WRAPPER_METHOD_V0(Update)
    DEFINE_LUA_WRAPPER_METHOD_R2P1_V4(OnEvent, NET_Packet, u16, u32, ClientID)
    DEFINE_LUA_WRAPPER_METHOD_V1(Create, LPCSTR)
    DEFINE_LUA_WRAPPER_METHOD_R2P1_V2(net_Export_State, NET_Packet, ClientID)

    DEFINE_LUA_WRAPPER_METHOD_V0(OnRoundStart)
    //	DEFINE_LUA_WRAPPER_METHOD_V1(OnDelayedRoundEnd, ERoundEnd_Result)
    DEFINE_LUA_WRAPPER_METHOD_V0(OnRoundEnd)

    virtual game_PlayerState* createPlayerState()
    {
        return call_member<game_PlayerState*>(this, "createPlayerState")[adopt(result)];
    }
    static game_PlayerState* createPlayerState_static(inherited* ptr)
    {
        return ptr->self_type::inherited::createPlayerState();
    }

    DEFINE_LUA_WRAPPER_METHOD_R2P3_V3(OnPlayerHitPlayer, u16, u16, NET_Packet)
};

#pragma warning(pop)

#pragma optimize("s", on)
void game_sv_mp::script_register(lua_State* L)
{
    module(L)[class_<game_sv_mp, game_sv_GameState>("game_sv_mp")
                  .def(constructor<>())
                  //.def("SpawnWeaponForActor",		&game_sv_mp::SpawnWeaponForActor)
                  .def("KillPlayer", &game_sv_mp::KillPlayer)
                  .def("SendPlayerKilledMessage", &game_sv_mp::SendPlayerKilledMessage)
                  .def("signal_Syncronize", &game_sv_GameState::signal_Syncronize)];
}

void game_sv_mp_script::script_register(lua_State* L)
{
    typedef CWrapperBase<game_sv_mp_script> WrapType;
    typedef game_sv_mp_script BaseType;

    module(L)[class_<game_sv_mp_script, WrapType, game_sv_mp>("game_sv_mp_script")
                  .def(constructor<>())
                  .def("GetTeamData", &GetTeamData)
                  .def("SpawnPlayer", &SpawnPlayer)
                  .def("switch_Phase", &switch_Phase)
                  .def("SetHitParams", &BaseType::SetHitParams)
                  .def("GetHitParamsPower", &BaseType::GetHitParamsPower)
                  .def("GetHitParamsImpulse", &BaseType::GetHitParamsImpulse)

                  .def("type_name", &BaseType::type_name, &WrapType::type_name_static)
                  .def("Update", &BaseType::Update, &WrapType::Update_static)
                  .def("OnEvent", &BaseType::OnEvent, &WrapType::OnEvent_static)
                  .def("Create", (void (BaseType::*)(LPCSTR))(&BaseType::Create), &WrapType::Create_static)

                  .def("OnPlayerHitPlayer", &BaseType::OnPlayerHitPlayer, &WrapType::OnPlayerHitPlayer_static)

                  .def("OnRoundStart", &BaseType::OnRoundStart, &WrapType::OnRoundStart_static)
                  //			.def("OnDelayedRoundEnd",	&BaseType::OnDelayedRoundEnd,
                  //&WrapType::OnDelayedRoundEnd_static)
                  .def("OnRoundEnd", &BaseType::OnRoundEnd, &WrapType::OnRoundEnd_static)

                  .def("net_Export_State", &BaseType::net_Export_State, &WrapType::net_Export_State_static)
                  .def("createPlayerState", &BaseType::createPlayerState, &WrapType::createPlayerState_static,
                      adopt(result))];
}
