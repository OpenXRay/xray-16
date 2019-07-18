#pragma once

#include "game_cl_mp.h"

class CScriptGameObject;

class game_cl_mp_script : public game_cl_mp
{
    friend void game_cl_mp_script_script_register(lua_State* luaState);

    using inherited = game_cl_mp;

protected:
    CScriptGameObject* GetObjectByGameID(u16 id);
    game_PlayerState* GetLocalPlayer() { return local_player; };
public:
    game_cl_mp_script();
    virtual bool CanBeReady() { return false; };
    virtual void GetMapEntities(xr_vector<SZoneMapEntityData>& dst){};
    virtual void shedule_Update(u32 dt);
    virtual game_PlayerState* createPlayerState() { return inherited::createPlayerState(nullptr); };
    void EventGen(NET_Packet* P, u16 type, u16 dest);
    void GameEventGen(NET_Packet* P, u16 dest);
    void EventSend(NET_Packet* P);
    LPCSTR GetRoundTime();
};
