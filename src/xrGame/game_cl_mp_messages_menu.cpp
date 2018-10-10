#include "StdAfx.h"
#include "game_cl_mp.h"
#include "ui/UISpeechMenu.h"
#include "xrMessages.h"
#include "Level.h"
#include "ui/UIMessagesWindow.h"
#include "string_table.h"
#include "map_manager.h"
#include "map_location.h"
#include "UIGameCustom.h"

void game_cl_mp::AddMessageMenu(LPCSTR menu_section, LPCSTR snd_path, LPCSTR team_prefix)
{
    if (!menu_section)
        return;
    if (!pSettings->section_exist(menu_section))
        return;

    m_aMessageMenus.push_back(cl_MessageMenu());
    cl_MessageMenu* pNewMenu = &(m_aMessageMenus.back());
    pNewMenu->m_pSpeechMenu = new CUISpeechMenu(menu_section);
    pNewMenu->m_aMessages.clear();
    for (u32 i = 0; i < 10; i++)
    {
        shared_str LineName;
        LineName.printf("phrase_%d", i);
        if (!pSettings->line_exist(menu_section, *LineName))
            break;
        //---------------------------------------------------------
        string4096 Line;
        xr_strcpy(Line, pSettings->r_string(menu_section, *LineName));
        u32 count = _GetItemCount(Line);
        if (!count)
            continue;
        //---------------------------------------------------------
        pNewMenu->m_aMessages.push_back(cl_Menu_Message());
        cl_Menu_Message* pNewMessage = &(pNewMenu->m_aMessages.back());
        pNewMessage->aVariants.clear();
        //---------------------------------------------------------
        string4096 Phrase, SoundName;
        _GetItem(Line, 0, Phrase);
        pNewMessage->pMessage = Phrase;
        _GetItem(Line, 1, SoundName);
        //---------------------------------------------------------
        for (u32 s = 1; s <= 16; s++)
        {
            string_path FileName_Voice, FileName_Radio, fn;
            xr_sprintf(FileName_Voice, "%s%s%d" DELIMITER "voice_%s%d", snd_path, team_prefix, 1, SoundName, s);
            if (!FS.exist(fn, "$game_sounds$", FileName_Voice, ".ogg"))
                break;

            pNewMessage->aVariants.push_back(TEAMSOUND());
            TEAMSOUND* pNewTeamSound = &(pNewMessage->aVariants.back());
            pNewTeamSound->clear();
            for (int t = 1; t <= GetTeamCount(); t++)
            {
                xr_sprintf(FileName_Voice, "%s%s%d" DELIMITER "voice_%s%d", snd_path, team_prefix, t, SoundName, s);
                xr_sprintf(FileName_Radio, "%s%s%d" DELIMITER "radio_%s%d", snd_path, team_prefix, t, SoundName, s);
                if (FS.exist(fn, "$game_sounds$", FileName_Voice, ".ogg") &&
                    FS.exist(fn, "$game_sounds$", FileName_Radio, ".ogg"))
                {
                    pNewTeamSound->push_back(cl_Message_Sound());
                    cl_Message_Sound* pMsgSound = &(pNewTeamSound->back());
                    pMsgSound->mSound_Voice.create(
                        FileName_Voice, st_Effect, sg_SourceType); // Msg("-- %s Loaded", FileName_Voice);
                    pMsgSound->mSound_Radio.create(
                        FileName_Radio, st_Effect, sg_SourceType); // Msg("-- %s Loaded", FileName_Radio);
                }
                else
                {
                    R_ASSERT(0);
                }
            }
        }
        //-----------------------------------------------
    };
    //-----------------------------------------------
};

void game_cl_mp::LoadMessagesMenu(LPCSTR menus_section)
{
    if (!menus_section)
        return;
    if (!pSettings->section_exist(menus_section))
        return;
    shared_str Sounds_Path = pSettings->r_string(menus_section, "sounds_path");
    shared_str Team_Prefix = READ_IF_EXISTS(pSettings, r_string, menus_section, "team_prefix", "");
    m_aMessageMenus.clear();
    //-----------------------------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        shared_str LineName;
        LineName.printf("menu_%d", i);
        if (!pSettings->line_exist(menus_section, *LineName))
            break;
        shared_str menu_section = pSettings->r_string(menus_section, *LineName);
        AddMessageMenu(*menu_section, *Sounds_Path, *Team_Prefix);
    }
};

void game_cl_mp::DestroyMessagesMenus()
{
    for (u32 m = 0; m < m_aMessageMenus.size(); m++)
    {
        cl_MessageMenu* pMMenu = &(m_aMessageMenus[m]);
        xr_delete(pMMenu->m_pSpeechMenu);
        for (u32 i = 0; i < pMMenu->m_aMessages.size(); i++)
        {
            cl_Menu_Message* pMMessage = &(pMMenu->m_aMessages[i]);
            for (u32 v = 0; v < pMMessage->aVariants.size(); v++)
            {
                TEAMSOUND* pVar = &(pMMessage->aVariants[v]);
                for (u32 t = 0; t < pVar->size(); t++)
                {
                    cl_Message_Sound* pTeamVar = &((*pVar)[t]);
                    if (pTeamVar->mSound_Radio._feedback())
                        pTeamVar->mSound_Radio.stop();
                    if (pTeamVar->mSound_Voice._feedback())
                        pTeamVar->mSound_Voice.stop();
                    pTeamVar->mSound_Radio.destroy();
                    pTeamVar->mSound_Voice.destroy();
                }
            }
        }
    }
};

void game_cl_mp::OnMessageSelected(CUISpeechMenu* pMenu, u8 PhraseID)
{
    m_cur_MenuID = u32(-1);
    if (m_aMessageMenus.empty())
        return;
    auto it = std::find(m_aMessageMenus.begin(), m_aMessageMenus.end(), pMenu);
    if (it == m_aMessageMenus.end())
        return;
    u8 MenuID = u8((it - m_aMessageMenus.begin()) & 0xff);
    cl_MessageMenu* pMMenu = &(m_aMessageMenus[MenuID]);
    if (PhraseID >= pMMenu->m_aMessages.size())
        return;
    cl_Menu_Message* pMMessage = &(pMMenu->m_aMessages[PhraseID]);
    u8 VariantID = (pMMessage->aVariants.size() <= 1) ? 0 : u8(::Random.randI(pMMessage->aVariants.size()) & 0xff);
    //	Msg ("Variant %d from %d", VariantID, pMMessage->aVariants.size());
    //-------------------------------------------------------------------
    NET_Packet P;
    u_EventGen(P, GE_GAME_EVENT, local_player->GameID);
    P.w_u16(GAME_EVENT_SPEECH_MESSAGE);
    P.w_u8(MenuID);
    P.w_u8(PhraseID);
    P.w_u8(VariantID);
    u_EventSend(P);
};

#define FRIEND_RADION_LOCATION "mp_friend_radio_location"

void game_cl_mp::OnSpeechMessage(NET_Packet& P)
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
            CurrentGameUI()->m_pMessagesWnd->AddChatMessage(
                StringTable().translate(pMMessage->pMessage.c_str()).c_str(), ps->getName());

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
    cl_Message_Sound* pMSound = &(Variant[ModifyTeam(ps->team)]);

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

void game_cl_mp::HideMessageMenus()
{
    if (m_aMessageMenus.empty())
        return;
    for (u32 i = 0; i < m_aMessageMenus.size(); i++)
    {
        cl_MessageMenu* pMenu = &(m_aMessageMenus[i]);
        if (pMenu->m_pSpeechMenu->IsShown())
            pMenu->m_pSpeechMenu->HideDialog();
    };
};
