#pragma once
#include "game_base.h"
#include "xrCore/client_id.h"
#include "WeaponAmmo.h"

class NET_Packet;
class CGameObject;
class CUIGameCustom;
class CUI;
class CUIDialogWnd;


class game_cl_GameState : public game_GameState, public ScheduledBase
{
    typedef game_GameState inherited;

protected:
    CUIGameCustom* m_game_ui_custom;
    u16 m_u16VotingEnabled;
    bool m_bServerControlHits;

private:
    void switch_Phase(u32 new_phase) { inherited::switch_Phase(new_phase); };

protected:
    virtual void OnSwitchPhase(u32 old_phase, u32 new_phase);

    virtual shared_str shedule_Name() const { return shared_str("game_cl_GameState"); };
    virtual float shedule_Scale() { return 1.0f; };
    virtual bool shedule_Needed() { return true; };

public:
    game_cl_GameState();
    virtual ~game_cl_GameState();
    virtual void net_import_state(NET_Packet& P);
    virtual void net_import_update(NET_Packet& P);
    virtual void net_import_GameTime(NET_Packet& P); // update GameTime only for remote clients
    virtual void net_signal(NET_Packet& P);

    virtual bool OnKeyboardPress(int key);
    virtual bool OnKeyboardRelease(int key);

    virtual CUIGameCustom* createGameUI() { return NULL; };
    virtual void SetGameUI(CUIGameCustom*){};

    virtual void shedule_Update(u32 dt);

    void u_EventGen(NET_Packet& P, u16 type, u16 dest);
    void u_EventSend(NET_Packet& P);

    virtual void OnRender(){};
    virtual bool IsServerControlHits() { return m_bServerControlHits; };
    virtual void OnSpawn(IGameObject* pObj){};
    virtual void OnDestroy(IGameObject* pObj){};

    virtual void SendPickUpEvent(u16 ID_who, u16 ID_what);

    virtual void OnConnected();
};
