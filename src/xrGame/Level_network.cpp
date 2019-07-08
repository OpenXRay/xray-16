#include "pch_script.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "game_cl_base.h"
#include "PHCommander.h"
#include "net_queue.h"
#include "MainMenu.h"
#include "space_restriction_manager.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "stalker_animation_data_storage.h"
#include "client_spawn_manager.h"
#include "seniority_hierarchy_holder.h"
#include "UIGameCustom.h"
#include "string_table.h"
#include "UI/UIGameTutorial.h"
#include "ui/UIPdaWnd.h"

#include "xrPhysics/physicscommon.h"

const int max_objects_size = 2 * 1024;
const int max_objects_size_in_save = 8 * 1024;

extern bool g_b_ClearGameCaptions;

void CLevel::remove_objects()
{
    if (!IsGameTypeSingle())
        Msg("CLevel::remove_objects - Start");
    BOOL b_stored = psDeviceFlags.test(rsDisableObjectsAsCrows);

    int loop = 5;
    while (loop)
    {
        if (OnServer())
        {
            R_ASSERT(Server);
            Server->SLS_Clear();
        }

        if (OnClient())
            ClearAllObjects();
        // XXX: why does one need to do this 20 times?
        for (int i = 0; i < 20; ++i)
        {
            snd_Events.clear();

            // ugly hack for checks that update is twice on frame
            // we need it since we do updates for checking network messages
            ++(Device.dwFrame);
            psDeviceFlags.set(rsDisableObjectsAsCrows, TRUE);
            ClientReceive();
            ProcessGameEvents();
            Objects.Update(false);
#ifdef DEBUG
            Msg("Update objects list...");
#endif // #ifdef DEBUG
            Objects.dump_all_objects();
        }

        if (Objects.o_count() == 0)
            break;
        else
        {
            --loop;
            Msg("Objects removal next loop. Active objects count=%d", Objects.o_count());
        }
    }

    BulletManager().Clear();
    ph_commander().clear();
    ph_commander_scripts().clear();

    if (!GEnv.isDedicatedServer)
        space_restriction_manager().clear();

    psDeviceFlags.set(rsDisableObjectsAsCrows, b_stored);
    g_b_ClearGameCaptions = true;

    GEnv.ScriptEngine->collect_all_garbage();

    stalker_animation_data_storage().clear();

    VERIFY(GEnv.Render);
    GEnv.Render->models_Clear(FALSE);

    GEnv.Render->clear_static_wallmarks();

#ifdef DEBUG
    if (!GEnv.isDedicatedServer)
        if (!client_spawn_manager().registry().empty())
            client_spawn_manager().dump();
#endif // DEBUG
    if (!GEnv.isDedicatedServer)
    {
#ifdef DEBUG
        VERIFY(client_spawn_manager().registry().empty());
#endif
        client_spawn_manager().clear();
    }

    g_pGamePersistent->destroy_particles(false);

    //.	xr_delete									(m_seniority_hierarchy_holder);
    //.	m_seniority_hierarchy_holder				= new CSeniorityHierarchyHolder();
    if (!IsGameTypeSingle())
        Msg("CLevel::remove_objects - End");
}

#ifdef DEBUG
extern void show_animation_stats();
#endif // DEBUG

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

void CLevel::net_Stop()
{
    Msg("- Disconnect");

    if (CurrentGameUI())
    {
        CurrentGameUI()->HideShownDialogs();
        CurrentGameUI()->GetPdaMenu().Reset();
    }

    if (g_tutorial && !g_tutorial->Persistent())
        g_tutorial->Stop();

    if (g_tutorial2 && !g_tutorial->Persistent())
        g_tutorial2->Stop();

    bReady = false;

    remove_objects();

    // WARNING ! remove_objects() uses this flag, so position of this line must e here ..
    game_configured = FALSE;

    IGame_Level::net_Stop();

    if (Server)
    {
        Server->Disconnect();
        xr_delete(Server);
    }

    GEnv.ScriptEngine->collect_all_garbage();

#ifdef DEBUG
    show_animation_stats();
#endif // DEBUG
}

void CLevel::ClientSend()
{
    NET_Packet P;
    u32 start = 0;

    while (1)
    {
        P.w_begin(M_UPDATE);
        start = Objects.net_Export(&P, start, max_objects_size);

        if (P.B.count > 2)
        {
            stats.ClientSendInternal.Begin();
            Send(P);
            stats.ClientSendInternal.End();
        }
        else
            break;
    }
}

u32 CLevel::Objects_net_Save(NET_Packet* _Packet, u32 start, u32 max_object_size)
{
    NET_Packet& Packet = *_Packet;
    u32 position;
    for (; start < Objects.o_count(); start++)
    {
        IGameObject* _P = Objects.o_get_by_iterator(start);
        CGameObject* P = smart_cast<CGameObject*>(_P);
        //		Msg			("save:iterating:%d:%s, size[%d]",P->ID(),*P->cName(), Packet.w_tell() );
        if (P && !P->getDestroy() && P->net_SaveRelevant())
        {
            Packet.w_u16(u16(P->ID()));
            Packet.w_chunk_open16(position);
            //			Msg						("save:saving:%d:%s",P->ID(),*P->cName());
            P->net_Save(Packet);
#ifdef DEBUG
            u32 size = u32(Packet.w_tell() - position) - sizeof(u16);
            //			Msg						("save:saved:%d bytes:%d:%s",size,P->ID(),*P->cName());
            if (size >= 65536)
            {
                xrDebug::Fatal(DEBUG_INFO, "Object [%s][%d] exceed network-data limit\n size=%d, Pend=%d, Pstart=%d",
                    *P->cName(), P->ID(), size, Packet.w_tell(), position);
            }
#endif
            Packet.w_chunk_close16(position);
            //			if (0==(--count))
            //				break;
            if (max_object_size >= (NET_PacketSizeLimit - Packet.w_tell()))
                break;
        }
    }
    return ++start;
}

void CLevel::ClientSave()
{
    NET_Packet P;
    u32 start = 0;

    for (;;)
    {
        P.w_begin(M_SAVE_PACKET);

        start = Objects_net_Save(&P, start, max_objects_size_in_save);

        if (P.B.count > 2)
            Send(P);
        else
            break;
    }
}

void CLevel::Send(NET_Packet& P)
{
    ClientID _clid;
    _clid.set(1);
    Server->OnMessage(P, _clid);
}

void CLevel::net_Update()
{
    if (game_configured)
    {
        // If we have enought bandwidth - replicate client data on to server
        stats.ClientSend.Begin();
        ClientSend();
        stats.ClientSend.End();
    }
    // If server - perform server-update
    if (Server && OnServer())
        Server->Update();
}

struct _NetworkProcessor : public pureFrame
{
    virtual void OnFrame()
    {
        if (g_pGameLevel && !Device.Paused())
            g_pGameLevel->net_Update();
    }
} NET_processor;

pureFrame* g_pNetProcessor = &NET_processor;

bool CLevel::Connect2Server(const char* options)
{
    R_ASSERT(options);

    Msg("* client : connection accepted - <All Ok>");

    return true;
};

void CLevel::ClearAllObjects()
{
    u32 CLObjNum = Level().Objects.o_count();

    bool ParentFound = true;

    while (ParentFound)
    {
        ParentFound = false;
        for (u32 i = 0; i < CLObjNum; i++)
        {
            IGameObject* pObj = Level().Objects.o_get_by_iterator(i);
            if (!pObj->H_Parent())
                continue;
            //-----------------------------------------------------------
            NET_Packet GEN;
            GEN.w_begin(M_EVENT);
            //---------------------------------------------
            GEN.w_u32(Level().timeServer());
            GEN.w_u16(GE_OWNERSHIP_REJECT);
            GEN.w_u16(pObj->H_Parent()->ID());
            GEN.w_u16(u16(pObj->ID()));
            game_events->insert(GEN);
            if (g_bDebugEvents)
                ProcessGameEvents();
            //-------------------------------------------------------------
            ParentFound = true;
//-------------------------------------------------------------
#ifdef DEBUG
            Msg("Rejection of %s[%d] from %s[%d]", *(pObj->cNameSect()), pObj->ID(), *(pObj->H_Parent()->cNameSect()),
                pObj->H_Parent()->ID());
#endif
        };
        ProcessGameEvents();
    };

    CLObjNum = Level().Objects.o_count();

    for (u32 i = 0; i < CLObjNum; i++)
    {
        IGameObject* pObj = Level().Objects.o_get_by_iterator(i);
        if (pObj->H_Parent() != NULL)
        {
            if (IsGameTypeSingle())
            {
                FATAL("pObj->H_Parent() != NULL");
            }
            else
            {
                Msg("! ERROR: object's parent is not NULL");
            }
        }

        //-----------------------------------------------------------
        NET_Packet GEN;
        GEN.w_begin(M_EVENT);
        //---------------------------------------------
        GEN.w_u32(Level().timeServer());
        GEN.w_u16(GE_DESTROY);
        GEN.w_u16(u16(pObj->ID()));
        game_events->insert(GEN);
        if (g_bDebugEvents)
            ProcessGameEvents();
        //-------------------------------------------------------------
        ParentFound = true;
//-------------------------------------------------------------
#ifdef DEBUG
        Msg("Destruction of %s[%d]", *(pObj->cNameSect()), pObj->ID());
#endif
    };
    ProcessGameEvents();
};

void CLevel::OnInvalidHost()
{
    IPureClient::OnInvalidHost();
    if (MainMenu()->GetErrorDialogType() == CMainMenu::ErrNoError)
        MainMenu()->SetErrorDialog(CMainMenu::ErrInvalidHost);
};

void CLevel::OnInvalidPassword()
{
    IPureClient::OnInvalidPassword();
    MainMenu()->SetErrorDialog(CMainMenu::ErrInvalidPassword);
};

void CLevel::OnSessionFull()
{
    IPureClient::OnSessionFull();
    if (MainMenu()->GetErrorDialogType() == CMainMenu::ErrNoError)
        MainMenu()->SetErrorDialog(CMainMenu::ErrSessionFull);
}
