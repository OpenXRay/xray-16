#include "stdafx.h"
#include "game_cl_freemp.h"
#include "clsid_game.h"
#include "xrEngine/xr_level_controller.h"
#include "UIGameFMP.h"
#include "actor_mp_client.h"
#include "VoiceChat.h"
#include "ui/UIMainIngameWnd.h"

game_cl_freemp::game_cl_freemp()
{
}

game_cl_freemp::~game_cl_freemp()
{
}


CUIGameCustom* game_cl_freemp::createGameUI()
{
	if (GEnv.isDedicatedServer)
		return NULL;

	CLASS_ID clsid = CLSID_GAME_UI_FREEMP;
	m_game_ui = smart_cast<CUIGameFMP*> (NEW_INSTANCE(clsid));
	R_ASSERT(m_game_ui);
	m_game_ui->Load();
	m_game_ui->SetClGame(this);
	return					m_game_ui;
}

void game_cl_freemp::SetGameUI(CUIGameCustom* uigame)
{
	inherited::SetGameUI(uigame);
	m_game_ui = smart_cast<CUIGameFMP*>(uigame);
	R_ASSERT(m_game_ui);

	if (m_pVoiceChat)
	{
		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
	}
}


void game_cl_freemp::net_import_state(NET_Packet & P)
{
	inherited::net_import_state(P);
}

void game_cl_freemp::net_import_update(NET_Packet & P)
{
	inherited::net_import_update(P);
}

void game_cl_freemp::shedule_Update(u32 dt)
{
	game_cl_GameState::shedule_Update(dt);

	if (!local_player)
		return;

	if (!GEnv.isDedicatedServer && m_pVoiceChat)
	{
		const bool started = m_pVoiceChat->IsStarted();
		const bool is_dead = !local_player || local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		const bool has_shown_dialogs = CurrentGameUI()->HasShownDialogs();
		if (started && (is_dead || has_shown_dialogs))
		{
			m_pVoiceChat->Stop();
			CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		}
		m_pVoiceChat->Update();
	}

	for (auto cl : players)
	{
		game_PlayerState* ps = cl.second;
		if (!ps || ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)) continue;

		CActor* pActor = smart_cast<CActor*>(Level().Objects.net_Find(ps->GameID));
		if (!pActor || !pActor->g_Alive()) continue;

		pActor->SetName(ps->getName());
		pActor->cName_set(ps->getName());

		if (ps->team != pActor->Community())
		{
			CHARACTER_COMMUNITY	community;
			community.set(ps->team);
			pActor->SetCommunity(community.index());
			pActor->ChangeTeam(community.team(), 0, 0);
		}

		if (local_player->GameID == ps->GameID)
		{
			pActor->set_money((u32)ps->money_for_round, false);
		}
	}
}

void game_cl_freemp::OnRender()
{
	inherited::OnRender();

	if (m_pVoiceChat)
		m_pVoiceChat->OnRender();
}

bool game_cl_freemp::OnKeyboardPress(int key)
{
	switch (key)
	{
	case kJUMP:
	{
		bool b_need_to_send_ready = false;

		IGameObject* curr = Level().CurrentControlEntity();
		if (!curr) return(false);

		bool is_actor = !!smart_cast<CActor*>(curr);
		bool is_spectator = !!smart_cast<CSpectator*>(curr);

		game_PlayerState* ps = local_player;

		if (is_spectator || (is_actor && ps && ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
		{
			b_need_to_send_ready = true;
		}

		if (b_need_to_send_ready)
		{
			CGameObject* GO = smart_cast<CGameObject*>(curr);
			NET_Packet			P;
			GO->u_EventGen(P, GE_GAME_EVENT, GO->ID());
			P.w_u16(GAME_EVENT_PLAYER_READY);
			GO->u_EventSend(P);
			return				true;
		}
		else
		{
			return false;
		}
	}break;

	default:
		break;
	}

	return inherited::OnKeyboardPress(key);
}

bool game_cl_freemp::OnKeyboardRelease(int key)
{
	switch (key)
	{
	case kVOICE_CHAT:
	{
		m_pVoiceChat->Stop();
		CurrentGameUI()->UIMainIngameWnd->SetActiveVoiceIcon(false);
		return true;
	}break;

	default:
		break;
	}

	return inherited::OnKeyboardRelease(key);
}

LPCSTR game_cl_freemp::GetGameScore(string32&	score_dest)
{
	s32 frags = local_player ? local_player->frags() : 0;
	xr_sprintf(score_dest, "[%d]", frags);
	return score_dest;
}

void game_cl_freemp::OnConnected()
{
    if (GEnv.isDedicatedServer)
        return;

	inherited::OnConnected();
	if (m_game_ui)
	{
		R_ASSERT(!GEnv.isDedicatedServer);
		m_game_ui = smart_cast<CUIGameFMP*>	(CurrentGameUI());
		m_game_ui->SetClGame(this);
	}

	luabind::functor<void>	funct;
	R_ASSERT(GEnv.ScriptEngine->functor("mp_game_cl.on_connected", funct));
	funct();
}

void game_cl_freemp::OnScreenResolutionChanged()
{
	if (m_game_ui && m_pVoiceChat)
	{
		m_game_ui->UIMainIngameWnd->SetVoiceDistance(m_pVoiceChat->GetDistance());
	}
}

void game_cl_freemp::OnVoiceMessage(NET_Packet* P)
{
	m_pVoiceChat->ReceiveMessage(P);
}

