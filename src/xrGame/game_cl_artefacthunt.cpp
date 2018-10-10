#include "StdAfx.h"
#include "game_cl_artefacthunt.h"
#include "xrMessages.h"
#include "Level.h"
#include "UIGameAHunt.h"
#include "map_manager.h"
#include "Common/LevelGameDef.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Actor.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UISkinSelector.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIMapDesc.h"
#include "ui/UIMessageBoxEx.h"
#include "xrUICore/Static/UIStatic.h"
#include "xr_level_controller.h"
#include "Artefact.h"
#include "map_location.h"
#include "Inventory.h"
#include "ActorCondition.h"
#include "ui/TeamInfo.h"
#include "string_table.h"
#include "CustomOutfit.h"
#include "clsid_game.h"
#include "ui/UIActorMenu.h"

#define TEAM0_MENU "artefacthunt_team0"
#define TEAM1_MENU "artefacthunt_team1"
#define TEAM2_MENU "artefacthunt_team2"

#define MESSAGE_MENUS "ahunt_messages_menu"

#include "game_cl_artefacthunt_snd_msg.h"
#include "xrEngine/IGame_Persistent.h"

#include "reward_event_generator.h"

game_cl_ArtefactHunt::game_cl_ArtefactHunt()
{
    m_game_ui = NULL;

    m_bBuyEnabled = FALSE;
    //---------------------------------
    m_Eff_Af_Spawn = "";
    m_Eff_Af_Disappear = "";
    //---------------------------------
    LoadSndMessages();
    //---------------------------------
    m_iSpawn_Cost = READ_IF_EXISTS(pSettings, r_s32, "artefacthunt_gamedata", "spawn_cost", -10000);
}

void game_cl_ArtefactHunt::Init()
{
    //	pInventoryMenu	= new CUIInventoryWnd();
    //	pPdaMenu = new CUIPdaWnd();
    //	pMapDesc = new CUIMapDesc();

    LoadTeamData(TEAM1_MENU);
    LoadTeamData(TEAM2_MENU);

    old_artefactBearerID = 0;
    old_artefactID = 0;
    old_teamInPossession = 0;
    //---------------------------------------------------
    /*	string_path	fn_game;
        if (FS.exist(fn_game, "$level$", "level.game"))
        {
            IReader *F = FS.r_open	(fn_game);
            IReader *O = 0;

            // Load RPoints
            if (0!=(O = F->open_chunk	(RPOINT_CHUNK)))
            {
                for (int id=0; O->find_chunk(id); ++id)
                {
                    RPoint					R;
                    u8						RP_team;
                    u8						RP_type;
                    u16						RP_GameType;

                    O->r_fvector3			(R.P);
                    O->r_fvector3			(R.A);
                    RP_team					= O->r_u8	();	VERIFY(RP_team>=0 && RP_team<4);
                    RP_type					= O->r_u8	();
                    RP_GameType				= O->r_u16	();
                    //u16 res					=
                    //O->r_u8	();

                    if (RP_GameType != GAME_ANY && RP_GameType != GAME_ARTEFACTHUNT)
                    {
                        continue;
                    };
                    switch (RP_type)
                    {
                    case rptTeamBaseParticle:
                        {
                            string256 ParticleStr;
                            xr_sprintf(ParticleStr, "teambase_particle_%d", RP_team);
                            if (pSettings->line_exist("artefacthunt_gamedata", ParticleStr))
                            {
                                Fmatrix			transform;
                                transform.identity();
                                transform.setXYZ(R.A);
                                transform.translate_over(R.P);
                                CParticlesObject* pStaticParticles			=
       CParticlesObject::Create(pSettings->r_string("artefacthunt_gamedata", ParticleStr),FALSE,false);
                                pStaticParticles->UpdateParent	(transform,zero_vel);
                                pStaticParticles->Play			();
                                Level().m_StaticParticles.push_back		(pStaticParticles);
                            };
                        }break;
                    };
                };
                O->close();
            }

            FS.r_close	(F);
        }*/
    //-------------------------------------------------------
    if (pSettings->line_exist("artefacthunt_gamedata", "artefact_spawn_effect"))
        m_Eff_Af_Spawn = pSettings->r_string("artefacthunt_gamedata", "artefact_spawn_effect");
    if (pSettings->line_exist("artefacthunt_gamedata", "artefact_disappear_effect"))
        m_Eff_Af_Disappear = pSettings->r_string("artefacthunt_gamedata", "artefact_disappear_effect");
};

game_cl_ArtefactHunt::~game_cl_ArtefactHunt()
{
    /*
    pMessageSounds[0].destroy();
    pMessageSounds[1].destroy();
    pMessageSounds[2].destroy();
    pMessageSounds[3].destroy();
    pMessageSounds[4].destroy();
    pMessageSounds[5].destroy();
    pMessageSounds[6].destroy();
    pMessageSounds[7].destroy();
    */
}

BOOL bBearerCantSprint = TRUE;
void game_cl_ArtefactHunt::net_import_state(NET_Packet& P)
{
    inherited::net_import_state(P);

    P.r_u8(artefactsNum);
    P.r_u16(artefactBearerID);
    P.r_u8(teamInPossession);
    P.r_u16(artefactID);
    bBearerCantSprint = !!P.r_u8();

    iReinforcementTime = P.r_s32();
    if (iReinforcementTime > 0)
    {
        P.r_s32(dReinforcementTime);
        dReinforcementTime += Level().timeServer();
    }
    else
        dReinforcementTime = 0;
}

#include "string_table.h"

void game_cl_ArtefactHunt::TranslateGameMessage(u32 msg, NET_Packet& P)
{
    CStringTable& st = StringTable();
    string512 Text;
    string512 tmp;
    //	LPSTR	Color_Teams[3]		= {"%c[255,255,255,255]", "%c[255,64,255,64]", "%c[255,64,64,255]"};
    char Color_Main[] = "%c[255,192,192,192]";
    char Color_Artefact[] = "%c[255,255,255,0]";
    //	LPSTR	TeamsNames[3]		= {"Zero Team", "Team Green", "Team Blue"};

    switch (msg)
    {
    //-------------------UI MESSAGES
    case GAME_EVENT_ARTEFACT_TAKEN: // ahunt
    {
        u16 PlayerID, Team;
        P.r_u16(PlayerID);
        P.r_u16(Team);

        game_PlayerState* pPlayer = GetPlayerByGameID(PlayerID);
        if (!pPlayer)
            break;

        if (m_reward_generator)
            m_reward_generator->OnPlayerTakeArtefact(pPlayer);

        xr_sprintf(tmp, "%s%s", "%s%s %s", *st.translate("mp_has_tak_art"));

        xr_sprintf(Text, tmp, CTeamInfo::GetTeam_color_tag(int(Team)), pPlayer->getName(), Color_Main, Color_Artefact);

        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);

        if (!Game().local_player)
            break;
        if (Game().local_player->GameID == PlayerID)
            PlaySndMessage(ID_AF_TEAM1_TAKE + ModifyTeam(Game().local_player->team));
        else if (Game().local_player->team == Team)
            PlaySndMessage(ID_AF_TEAM1_TAKE_R + ModifyTeam(Game().local_player->team));
        else
            PlaySndMessage(ID_AF_TEAM1_TAKE_ENEMY + ModifyTeam(Game().local_player->team));
    }
    break;
    case GAME_EVENT_ARTEFACT_DROPPED: // ahunt
    {
        u16 PlayerID, Team;
        P.r_u16(PlayerID);
        P.r_u16(Team);

        game_PlayerState* pPlayer = GetPlayerByGameID(PlayerID);
        if (!pPlayer)
            break;

        if (m_reward_generator)
            m_reward_generator->OnPlayerDropArtefact(pPlayer);

        xr_sprintf(tmp, "%s%s", "%s%s %s", *st.translate("mp_has_drop_art"));

        xr_sprintf(Text, tmp, CTeamInfo::GetTeam_color_tag(int(Team)), pPlayer->getName(), Color_Main, Color_Artefact);
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);

        //			pMessageSounds[0].play_at_pos(NULL, Fvector().set(0,0,0), sm_2D, 0);
        PlaySndMessage(ID_AF_LOST);
    }
    break;
    case GAME_EVENT_ARTEFACT_ONBASE: // ahunt
    {
        u16 PlayerID, Team;
        P.r_u16(PlayerID);
        P.r_u16(Team);

        game_PlayerState* pPlayer = GetPlayerByGameID(PlayerID);
        if (!pPlayer)
            break;

        if (m_reward_generator)
            m_reward_generator->OnPlayerBringArtefact(pPlayer);

        xr_sprintf(tmp, "%s%s", "%s%s %s", *st.translate("mp_scores"));

        xr_sprintf(Text, tmp, CTeamInfo::GetTeam_color_tag(int(Team)), CTeamInfo::GetTeam_name(int(Team)), Color_Main);
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);

        if (!Game().local_player)
            break;
        if (Game().local_player->GameID == PlayerID)
            PlaySndMessage(ID_AF_TEAM1_ONBASE + ModifyTeam(Game().local_player->team));
        else if (Game().local_player->team == Team)
            PlaySndMessage(ID_AF_TEAM1_ONBASE_R + ModifyTeam(Game().local_player->team));
        else
            PlaySndMessage(ID_AF_TEAM1_ONBASE_ENEMY + ModifyTeam(Game().local_player->team));
    }
    break;
    case GAME_EVENT_ARTEFACT_SPAWNED: // ahunt
    {
        xr_sprintf(Text, "%s%s", Color_Main, *st.translate("mp_art_spowned"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        if (m_reward_generator)
            m_reward_generator->OnArtefactSpawned();

        PlaySndMessage(ID_NEW_AF);
    }
    break;
    case GAME_EVENT_ARTEFACT_DESTROYED: // ahunt
    {
        xr_sprintf(Text, "%s%s", Color_Main, *st.translate("mp_art_destroyed"));
        u16 ArtefactID = P.r_u16();
        //-------------------------------------------
        IGameObject* pObj = Level().Objects.net_Find(ArtefactID);
        if (pObj && xr_strlen(m_Eff_Af_Disappear))
            PlayParticleEffect(m_Eff_Af_Disappear.c_str(), pObj->Position());
        //-------------------------------------------
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
    }
    break;
    default: inherited::TranslateGameMessage(msg, P);
    }
}

void game_cl_ArtefactHunt::SetGameUI(CUIGameCustom* uigame)
{
    inherited::SetGameUI(uigame);
    m_game_ui = smart_cast<CUIGameAHunt*>(uigame);
    R_ASSERT(m_game_ui);
};

CUIGameCustom* game_cl_ArtefactHunt::createGameUI()
{
    if (GEnv.isDedicatedServer)
        return NULL;

    CLASS_ID clsid = CLSID_GAME_UI_ARTEFACTHUNT;
    m_game_ui = smart_cast<CUIGameAHunt*>(NEW_INSTANCE(clsid));
    R_ASSERT(m_game_ui);
    m_game_ui->Load();
    m_game_ui->SetClGame(this);
    LoadMessagesMenu(MESSAGE_MENUS);
    return m_game_ui;
}

void game_cl_ArtefactHunt::GetMapEntities(xr_vector<SZoneMapEntityData>& dst)
{
    inherited::GetMapEntities(dst);

    SZoneMapEntityData D;
    u32 color_enemy_with_artefact = 0xffff0000;
    u32 color_artefact = 0xffffffff;
    u32 color_friend_with_artefact = 0xffffff00;

    s16 local_team = local_player->team;

    IGameObject* pObject = Level().Objects.net_Find(artefactID);
    if (!pObject)
        return;

    CArtefact* pArtefact = smart_cast<CArtefact*>(pObject);
    VERIFY(pArtefact);

    IGameObject* pParent = pArtefact->H_Parent();
    if (!pParent)
    { // Artefact alone
        D.color = color_artefact;
        D.pos = pArtefact->Position();
        dst.push_back(D);
        return;
    };

    if (pParent && pParent->ID() == artefactBearerID && GetPlayerByGameID(artefactBearerID))
    {
        IGameObject* pBearer = Level().Objects.net_Find(artefactBearerID);
        VERIFY(pBearer);
        D.pos = pBearer->Position();

        game_PlayerState* ps = GetPlayerByGameID(artefactBearerID);
        (ps->team == local_team) ? D.color = color_friend_with_artefact : D.color = color_enemy_with_artefact;

        // remove previous record about this actor !!!
        dst.push_back(D);
        return;
    }
}

void game_cl_ArtefactHunt::shedule_Update(u32 dt)
{
    string1024 msg;

    inherited::shedule_Update(dt);

    if (GEnv.isDedicatedServer)
        return;

    if (!m_game_ui)
        return;

    // out game information
    m_game_ui->SetBuyMsgCaption(NULL);
    m_game_ui->SetPressBuyMsgCaption(NULL);

    switch (m_phase)
    {
    case GAME_PHASE_INPROGRESS:
    {
        if (local_player)
        {
            if (local_player->testFlag(GAME_PLAYER_FLAG_ONBASE) &&
                !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            {
                m_bBuyEnabled = TRUE;
            }
            else
            {
                m_bBuyEnabled = FALSE;
            };
        };

        if (local_player && Level().CurrentControlEntity())
        {
            if (smart_cast<CActor*>(Level().CurrentControlEntity()))
            {
                if (m_game_ui)
                    m_game_ui->SetBuyMsgCaption("");
                if (m_bBuyEnabled)
                {
                    if (!(pCurBuyMenu && pCurBuyMenu->IsShown()) && !(pCurSkinMenu && pCurSkinMenu->IsShown()))
                    {
                        xr_sprintf(msg, *StringTable().translate("mp_press_to_buy"), "B");
                        if (m_game_ui)
                            m_game_ui->SetBuyMsgCaption(msg);
                    };
                }

                if (m_game_ui)
                {
                    if (local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
                        m_game_ui->SetPressJumpMsgCaption("mp_press_fire2spectator");
                    else
                        m_game_ui->SetPressJumpMsgCaption(NULL);
                };
            }
            else
            {
                if (m_game_ui)
                    m_game_ui->SetBuyMsgCaption(NULL);
                if (m_bTeamSelected && m_bSkinSelected)
                {
                    if (iReinforcementTime != 0)
                    {
                        if (!m_game_ui->m_pBuySpawnMsgBox->IsShown() &&
                            (local_player->money_for_round + m_iSpawn_Cost) >= 0)
                        {
                            if (m_game_ui)
                                m_game_ui->SetPressJumpMsgCaption("mp_press_jump2pay_spaw");
                        }
                        else
                        {
                            if (m_game_ui)
                                m_game_ui->SetPressJumpMsgCaption(NULL);
                        }
                    }
                    else
                    {
                        if (m_game_ui)
                            m_game_ui->SetPressJumpMsgCaption("mp_press_jump2spawn");
                    };
                }
                else
                {
                    if (!m_bTeamSelected)
                        if (m_game_ui)
                            m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_team");
                        else if (!m_bSkinSelected)
                            if (m_game_ui)
                                m_game_ui->SetPressJumpMsgCaption("mp_press_jump2select_skin");
                }
            };
        }

        if (local_player)
        {
            game_TeamState team0 = teams[0];
            game_TeamState team1 = teams[1];

            if (dReinforcementTime > 0 && Level().CurrentViewEntity() && m_cl_dwWarmUp_Time == 0)
            {
                u32 CurTime = Level().timeServer();
                u32 dTime;
                if (s32(CurTime) > dReinforcementTime)
                    dTime = 0;
                else
                    dTime = iCeil(float(dReinforcementTime - CurTime) / 1000);

                string128 _buff;
                m_game_ui->m_pReinforcementInidcator->SetText(xr_itoa(dTime, _buff, 10));
            }
            else
                m_game_ui->m_pReinforcementInidcator->SetText("0");

            s16 lt = local_player->team;
            if (lt >= 0)
            {
                //					if(m_game_ui) m_game_ui->SetScoreCaption	(teams[0].score, teams[1].score);
            };
        };
        SetScore();
    }
    break;
    case GAME_PHASE_TEAM1_ELIMINATED:
    {
        m_game_ui->SetRoundResultCaption("Team Green ELIMINATED!");
        SetScore();
    }
    break;
    case GAME_PHASE_TEAM2_ELIMINATED:
    {
        m_game_ui->SetRoundResultCaption("Team Blue ELIMINATED!");
        SetScore();
    }
    break;
    default: {
    }
    break;
    };

    if (m_game_ui->m_pBuySpawnMsgBox->IsShown())
    {
        if (m_phase != GAME_PHASE_INPROGRESS ||
            (!local_player || !local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
        {
            m_game_ui->m_pBuySpawnMsgBox->HideDialog();
        };
    };
    //-------------------------------------------
}
void game_cl_ArtefactHunt::SetScore()
{
    game_cl_TeamDeathmatch::SetScore();
    //	game_cl_Deathmatch::SetScore();
    if (Level().CurrentViewEntity() && m_game_ui)
    {
        game_PlayerState* ps = GetPlayerByGameID(Level().CurrentViewEntity()->ID());

        if (ps && m_game_ui)
            m_game_ui->SetRank(ps->team, ps->rank);

        if (ps && m_game_ui)
            m_game_ui->SetFraglimit(ps->frags(), artefactsNum);
    }
}
BOOL game_cl_ArtefactHunt::CanCallBuyMenu()
{
    if (!m_bBuyEnabled)
        return FALSE;
    if (Phase() != GAME_PHASE_INPROGRESS)
        return false;

    if (!is_buy_menu_ready())
        return FALSE;

    if (m_game_ui->m_pUITeamSelectWnd && m_game_ui->m_pUITeamSelectWnd->IsShown())
    {
        return FALSE;
    };
    if (pCurSkinMenu && pCurSkinMenu->IsShown())
    {
        return FALSE;
    };
    if (m_game_ui && m_game_ui->GetActorMenu().IsShown())
    {
        return FALSE;
    }
    /*if (m_game_ui->m_pInventoryMenu && m_game_ui->m_pInventoryMenu->IsShown())
    {
        return FALSE;
    };*/

    CActor* pCurActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!pCurActor || !pCurActor->g_Alive())
        return FALSE;

    return TRUE;
};

bool game_cl_ArtefactHunt::CanBeReady()
{
    if (!local_player)
        return false;
    m_bMenuCalledFromReady = TRUE;

    SetCurrentSkinMenu();
    SetCurrentBuyMenu();

    if (!m_bTeamSelected)
    {
        if (CanCallTeamSelectMenu())
            m_game_ui->m_pUITeamSelectWnd->ShowDialog(true);
        return false;
    };

    if (!m_bSkinSelected)
    {
        if (CanCallSkinMenu())
            pCurSkinMenu->ShowDialog(true);
        return false;
    };

    if (pCurBuyMenu && !pCurBuyMenu->IsShown())
        ClearBuyMenu();

    m_bMenuCalledFromReady = FALSE;
    //	return inherited::CanBeReady();
    return true;
};

pcstr game_cl_ArtefactHunt::getTeamSection(int Team)
{
    switch (Team)
    {
    case 1: return "artefacthunt_team1";
    case 2: return "artefacthunt_team2";
    default: NODEFAULT;
    }
#ifdef DEBUG
    return nullptr;
#endif
};

bool game_cl_ArtefactHunt::PlayerCanSprint(CActor* pActor)
{
    if (artefactBearerID == 0)
        return true;
    if (bBearerCantSprint && pActor->ID() == artefactBearerID)
        return false;
    return true;
};

#define ARTEFACT_NEUTRAL "mp_af_neutral_location"
#define ARTEFACT_FRIEND "mp_af_friend_location"
#define ARTEFACT_ENEMY "mp_af_enemy_location"

void game_cl_ArtefactHunt::UpdateMapLocations()
{
    inherited::UpdateMapLocations();

    if (local_player)
    {
        if (!artefactID)
        {
            if (old_artefactID)
                Level().MapManager().RemoveMapLocationByObjectID(old_artefactID);
        }
        else
        {
            if (!artefactBearerID)
            {
                if (!Level().MapManager().HasMapLocation(ARTEFACT_NEUTRAL, artefactID))
                {
                    Level().MapManager().RemoveMapLocationByObjectID(artefactID);
                    (Level().MapManager().AddMapLocation(ARTEFACT_NEUTRAL, artefactID))->EnablePointer();
                };
            }
            else
            {
                if (teamInPossession == local_player->team)
                {
                    if (!Level().MapManager().HasMapLocation(ARTEFACT_FRIEND, artefactID))
                    {
                        Level().MapManager().RemoveMapLocationByObjectID(artefactID);
                        (Level().MapManager().AddMapLocation(ARTEFACT_FRIEND, artefactID))->EnablePointer();
                    }
                }
                else
                {
                    if (!Level().MapManager().HasMapLocation(ARTEFACT_ENEMY, artefactID))
                    {
                        Level().MapManager().RemoveMapLocationByObjectID(artefactID);
                    };

                    /*bool OutfitWorkDown = false;

                    CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(artefactBearerID));
                    if (pActor)
                    {
                        CCustomOutfit* pOutfit			= pActor->GetOutfit();
                        if (pOutfit && pOutfit->CLS_ID == CLSID_EQUIPMENT_SCIENTIFIC)
                        {
                            if (!pActor->AnyAction())
                            {
                                if (Level().MapManager().HasMapLocation(ARTEFACT_ENEMY, artefactID))
                                {
                                    Level().MapManager().RemoveMapLocationByObjectID(artefactID);
                                }
                                OutfitWorkDown = true;
                            }
                        }
                    }
                    if (!OutfitWorkDown && */
                    if (!Level().MapManager().HasMapLocation(ARTEFACT_ENEMY, artefactID))
                    {
                        (Level().MapManager().AddMapLocation(ARTEFACT_ENEMY, artefactID))->EnablePointer();
                    }
                }
            };
        };
        old_artefactBearerID = artefactBearerID;
        old_artefactID = artefactID;
        old_teamInPossession = teamInPossession;
    };
};

bool game_cl_ArtefactHunt::NeedToSendReady_Spectator(int key, game_PlayerState* ps)
{
    bool res = (GAME_PHASE_PENDING == Phase() && kWPN_FIRE == key) ||
        ((kJUMP == key) && GAME_PHASE_INPROGRESS == Phase() && CanBeReady());

    if ((Phase() == GAME_PHASE_INPROGRESS) && (kJUMP == key) && m_cl_dwWarmUp_Time)
    {
        return res;
    }

    if ((GAME_PHASE_INPROGRESS == Phase()) && (kJUMP == key) && (iReinforcementTime != 0) &&
        (!m_game_ui->m_pBuySpawnMsgBox->IsShown()) && local_player &&
        (local_player->money_for_round + m_iSpawn_Cost) >= 0)
    {
        string1024 BuySpawnText;
        xr_sprintf(
            BuySpawnText, *StringTable().translate("mp_press_yes2pay"), abs(local_player->money_for_round), abs(m_iSpawn_Cost));
        m_game_ui->m_pBuySpawnMsgBox->SetText(BuySpawnText);

        if (m_bTeamSelected && m_bSkinSelected)
            m_game_ui->m_pBuySpawnMsgBox->ShowDialog(true);

        return false;
    };
    return res;
}

void game_cl_ArtefactHunt::OnSpawn(IGameObject* pObj)
{
    inherited::OnSpawn(pObj);
    if (!pObj)
        return;
    CArtefact* pArtefact = smart_cast<CArtefact*>(pObj);
    if (pArtefact)
    {
        if (xr_strlen(m_Eff_Af_Spawn))
            PlayParticleEffect(m_Eff_Af_Spawn.c_str(), pObj->Position());
    };
}

void game_cl_ArtefactHunt::OnDestroy(IGameObject* pObj)
{
    inherited::OnDestroy(pObj);
    if (!pObj)
        return;
};

void game_cl_ArtefactHunt::LoadSndMessages()
{
    LoadSndMessage("ahunt_snd_messages", "artefact_new", ID_NEW_AF);
    LoadSndMessage("ahunt_snd_messages", "artefact_lost", ID_AF_LOST);

    LoadSndMessage("ahunt_snd_messages", "team1_artefact_on_base", ID_AF_TEAM1_ONBASE);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_on_base", ID_AF_TEAM2_ONBASE);
    LoadSndMessage("ahunt_snd_messages", "team1_artefact_on_base_r", ID_AF_TEAM1_ONBASE_R);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_on_base_r", ID_AF_TEAM2_ONBASE_R);
    LoadSndMessage("ahunt_snd_messages", "team1_artefact_on_base_enemy", ID_AF_TEAM1_ONBASE_ENEMY);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_on_base_enemy", ID_AF_TEAM2_ONBASE_ENEMY);

    LoadSndMessage("ahunt_snd_messages", "team1_artefact_take", ID_AF_TEAM1_TAKE);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_take", ID_AF_TEAM2_TAKE);
    LoadSndMessage("ahunt_snd_messages", "team1_artefact_take_r", ID_AF_TEAM1_TAKE_R);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_take_r", ID_AF_TEAM2_TAKE_R);
    LoadSndMessage("ahunt_snd_messages", "team1_artefact_take_enemy", ID_AF_TEAM1_TAKE_ENEMY);
    LoadSndMessage("ahunt_snd_messages", "team2_artefact_take_enemy", ID_AF_TEAM2_TAKE_ENEMY);
};

void game_cl_ArtefactHunt::OnBuySpawnMenu_Ok()
{
    IGameObject* curr = Level().CurrentEntity();
    if (!curr)
        return;
    CGameObject* GO = smart_cast<CGameObject*>(curr);
    NET_Packet P;
    GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
    P.w_u16(GAME_EVENT_PLAYER_BUY_SPAWN);
    GO->u_EventSend(P);
};

void game_cl_ArtefactHunt::OnSellItemsFromRuck()
{
    if (!local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD) ||
        !local_player->testFlag(GAME_PLAYER_FLAG_ONBASE))
        return;
    CActor* pCurActor = smart_cast<CActor*>(Level().Objects.net_Find(local_player->GameID));
    if (!pCurActor)
        return;

    TIItemContainer::const_iterator IRuck = pCurActor->inventory().m_ruck.begin();
    TIItemContainer::const_iterator ERuck = pCurActor->inventory().m_ruck.end();

    NET_Packet P;
    pCurActor->u_EventGen(P, GEG_PLAYER_ITEM_SELL, pCurActor->ID());
    P.w_u16(u16(pCurActor->inventory().m_ruck.size() & 0xffff));
    for (; IRuck != ERuck; ++IRuck)
    {
        PIItem pItem = *IRuck;
        P.w_u16(pItem->object().ID());
    };
    pCurActor->u_EventSend(P);
};

void game_cl_ArtefactHunt::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
    game_cl_GameState::SendPickUpEvent(ID_who, ID_what);
};

void game_cl_ArtefactHunt::OnConnected()
{
    inherited::OnConnected();
    m_game_ui = smart_cast<CUIGameAHunt*>(CurrentGameUI());
}
