#include "StdAfx.h"
#include "string_table.h"
#include "Level.h"
#include "ui/UIMessagesWindow.h"
#include "map_manager.h"
#include "map_location.h"
#include "game_cl_capture_the_artefact.h"

#define FRIEND_RADION_LOCATION "mp_friend_radio_location"

void game_cl_CaptureTheArtefact::OnSpeechMessage(NET_Packet& P)
{
    if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
        return;
    u16 PlayerID = P.r_u16();
    game_PlayerState* ps = GetPlayerByGameID(PlayerID);
    if (!ps)
        return;

    u8 MenuID = P.r_u8();
    if (MenuID >= m_aMessageMenus.size())
        return;

    cl_MessageMenu* pMenu = &(m_aMessageMenus[MenuID]);
    u8 PhraseID = P.r_u8();
    if (PhraseID >= pMenu->m_aMessages.size())
        return;

    cl_Menu_Message* pMMessage = &(pMenu->m_aMessages[PhraseID]);

    if (ps->team == local_player->team)
    {
        if (CurrentGameUI())
            CurrentGameUI()->m_pMessagesWnd->AddChatMessage(*StringTable().translate(*(pMMessage->pMessage)), ps->getName());

        if (!Level().MapManager().HasMapLocation(FRIEND_RADION_LOCATION, ps->GameID))
        {
            (Level().MapManager().AddMapLocation(FRIEND_RADION_LOCATION, ps->GameID))->EnablePointer();
        }
    }

    u8 VariantID = P.r_u8();
    if (pMMessage->aVariants.empty())
        return;
    if (VariantID && VariantID >= pMMessage->aVariants.size())
        return;
    TEAMSOUND& Variant = pMMessage->aVariants[VariantID];

    // only one string :(
    cl_Message_Sound* pMSound = &(Variant[ps->team]);

    if (ps->team == local_player->team)
    {
        if (ps == local_player)
        {
            pMSound->mSound_Voice.play_at_pos(NULL, Fvector().set(0, 0, 0), sm_2D, 0);
        }
        else
        {
            pMSound->mSound_Radio.play_at_pos(NULL, Fvector().set(0, 0, 0), sm_2D, 0);
        }
        Msg("%s said: %s", ps->getName(), *StringTable().translate(pMMessage->pMessage));
    }
    else
    {
        IGameObject* pObj = Level().Objects.net_Find(ps->GameID);
        if (pObj)
        {
            pMSound->mSound_Voice.play_at_pos(pObj, pObj->Position());
        };
    };
};
