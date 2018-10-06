////////////////////////////////////////////////////////////////////////////
// script_game_object_inventory_owner.сpp :	функции для inventory owner
//////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "InventoryOwner.h"
#include "PDA.h"
#include "xrMessages.h"
#include "character_info.h"
#include "GameTask.h"
#include "Actor.h"
#include "Level.h"
#include "date_time.h"
#include "UIGameSP.h"
#include "restricted_object.h"
#include "xrScriptEngine/script_engine.hpp"
#include "attachable_item.h"
#include "script_entity.h"
#include "string_table.h"
#include "alife_registry_wrappers.h"
#include "relation_registry.h"
#include "CustomMonster.h"
#include "ActorCondition.h"
#include "xrAICore/Navigation/level_graph.h"
#include "HudItem.h"
#include "ui/UITalkWnd.h"
#include "Inventory.h"
#include "InfoPortion.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "WeaponMagazined.h"
#include "ai/stalker/ai_stalker.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "stalker_animation_manager.h"
#include "CameraFirstEye.h"
#include "stalker_movement_manager_smart_cover.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "smart_cover_object.h"
#include "smart_cover.h"
#include "CustomDetector.h"
#include "doors_manager.h"
#include "doors_door.h"
#include "Torch.h"
#include "PhysicObject.h"
//Alundaio
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade_root.h"
#include "inventory_item.h"
#include "CustomOutfit.h"
#include "inventory_item_impl.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServerEntities/inventory_space.h"
//-Alundaio

bool CScriptGameObject::GiveInfoPortion(LPCSTR info_id)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    pInventoryOwner->TransferInfo(info_id, true);
    return true;
}

bool CScriptGameObject::DisableInfoPortion(LPCSTR info_id)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    pInventoryOwner->TransferInfo(info_id, false);
    return true;
}

void _AddIconedTalkMessage(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name);

void CScriptGameObject::AddIconedTalkMessage(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name)
{
    _AddIconedTalkMessage(caption, text, texture_name, templ_name);
}

void _AddIconedTalkMessage(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name)
{
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->AddIconedMessage(
            caption, text, texture_name, templ_name ? templ_name : "iconed_answer_item");
    }
}

void _give_news(LPCSTR caption, LPCSTR news, LPCSTR texture_name, int delay, int show_time, int type);

void CScriptGameObject::GiveGameNews(LPCSTR caption, LPCSTR news, LPCSTR texture_name, int delay, int show_time)
{
    GiveGameNews(caption, news, texture_name, delay, show_time, 0);
}

void CScriptGameObject::GiveGameNews(
    LPCSTR caption, LPCSTR news, LPCSTR texture_name, int delay, int show_time, int type)
{
    _give_news(caption, news, texture_name, delay, show_time, type);
}

void _give_news(LPCSTR caption, LPCSTR text, LPCSTR texture_name, int delay, int show_time, int type)
{
    GAME_NEWS_DATA news_data;
    news_data.m_type = (GAME_NEWS_DATA::eNewsType)type;
    news_data.news_caption = caption;
    news_data.news_text = text;
    if (show_time != 0)
        news_data.show_time = show_time; // override default

    VERIFY(xr_strlen(texture_name) > 0);

    news_data.texture_name = texture_name;

    if (delay == 0)
        Actor()->AddGameNews(news_data);
    else
        Actor()->AddGameNews_deffered(news_data, delay);
}

bool CScriptGameObject::HasInfo(LPCSTR info_id)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;

    return pInventoryOwner->HasInfo(info_id);
}
bool CScriptGameObject::DontHasInfo(LPCSTR info_id)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return true;

    return !pInventoryOwner->HasInfo(info_id);
}

bool CScriptGameObject::IsTalking()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    return pInventoryOwner->IsTalking();
}

void CScriptGameObject::StopTalk()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->StopTalk();
}

void CScriptGameObject::EnableTalk()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->EnableTalk();
}
void CScriptGameObject::DisableTalk()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->DisableTalk();
}

bool CScriptGameObject::IsTalkEnabled()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    return pInventoryOwner->IsTalkEnabled();
}

void CScriptGameObject::EnableTrade()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->EnableTrade();
}
void CScriptGameObject::DisableTrade()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->DisableTrade();
}
bool CScriptGameObject::IsTradeEnabled()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    return pInventoryOwner->IsTradeEnabled();
}

void CScriptGameObject::EnableInvUpgrade()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->EnableInvUpgrade();
}
void CScriptGameObject::DisableInvUpgrade()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return;
    pInventoryOwner->DisableInvUpgrade();
}
bool CScriptGameObject::IsInvUpgradeEnabled()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
        return false;
    return pInventoryOwner->IsInvUpgradeEnabled();
}

void CScriptGameObject::ForEachInventoryItems(const luabind::functor<void>& functor)
{
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(&object());
    if (!owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::ForEachInventoryItems non-CInventoryOwner object !!!");
        return;
    }

    CInventory* pInv = &owner->inventory();
    TIItemContainer item_list;
    pInv->AddAvailableItems(item_list, true);

    TIItemContainer::iterator it;
    for (it = item_list.begin(); item_list.end() != it; ++it)
    {
        CGameObject* inv_go = smart_cast<CGameObject*>(*it);
        if (inv_go)
        {
            functor(inv_go->lua_game_object(), this);
        }
    }
}

// 1
void CScriptGameObject::IterateInventory(luabind::functor<void> functor, luabind::object object)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&this->object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::IterateInventory non-CInventoryOwner object !!!");
        return;
    }

    TIItemContainer::iterator I = inventory_owner->inventory().m_all.begin();
    TIItemContainer::iterator E = inventory_owner->inventory().m_all.end();
    for (; I != E; ++I)
        functor(object, (*I)->object().lua_game_object());
}

#include "InventoryBox.h"
void CScriptGameObject::IterateInventoryBox(luabind::functor<void> functor, luabind::object object)
{
    CInventoryBox* inventory_box = smart_cast<CInventoryBox*>(&this->object());
    if (!inventory_box)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::IterateInventoryBox non-CInventoryBox object !!!");
        return;
    }

    xr_vector<u16>::const_iterator I = inventory_box->m_items.begin();
    xr_vector<u16>::const_iterator E = inventory_box->m_items.end();
    for (; I != E; ++I)
    {
        CGameObject* GO = smart_cast<CGameObject*>(Level().Objects.net_Find(*I));
        if (GO)
            functor(object, GO->lua_game_object());
    }
}

void CScriptGameObject::MarkItemDropped(CScriptGameObject* item)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::MarkItemDropped non-CInventoryOwner object !!!");
        return;
    }

    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&item->object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::MarkItemDropped non-CInventoryItem object !!!");
        return;
    }

    inventory_item->SetDropManual(TRUE);
}

bool CScriptGameObject::MarkedDropped(CScriptGameObject* item)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::MarkedDropped non-CInventoryOwner object !!!");
        return (false);
    }

    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&item->object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::MarkedDropped non-CInventoryItem object !!!");
        return (false);
    }

    return (!!inventory_item->GetDropManual());
}

void CScriptGameObject::UnloadMagazine()
{
    CWeaponMagazined* weapon_magazined = smart_cast<CWeaponMagazined*>(&object());
    if (!weapon_magazined)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::UnloadMagazine non-CWeaponMagazined object !!!");
        return;
    }

    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(weapon_magazined->H_Parent());
    if (stalker && stalker->hammer_is_clutched())
        return;

    weapon_magazined->UnloadMagazine(false);
}
//

void CScriptGameObject::DropItem(CScriptGameObject* pItem)
{
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(&object());
    CInventoryItem* item = smart_cast<CInventoryItem*>(&pItem->object());
    if (!owner || !item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject::DropItem non-CInventoryOwner object !!!");
        return;
    }

    NET_Packet P;
    CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, object().ID());
    P.w_u16(pItem->object().ID());
    CGameObject::u_EventSend(P);
}

void CScriptGameObject::DropItemAndTeleport(CScriptGameObject* pItem, Fvector position)
{
    DropItem(pItem);

    NET_Packet PP;
    CGameObject::u_EventGen(PP, GE_CHANGE_POS, pItem->object().ID());
    PP.w_vec3(position);
    CGameObject::u_EventSend(PP);
}

void CScriptGameObject::MakeItemActive(CScriptGameObject* pItem)
{
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(&object());
    CInventoryItem* item = smart_cast<CInventoryItem*>(&pItem->object());
    u16 slot = item->BaseSlot();

    CInventoryItem* item_in_slot = owner->inventory().ItemFromSlot(slot);

    NET_Packet P;
    if (item_in_slot)
    {
        CGameObject::u_EventGen(P, GEG_PLAYER_ITEM2RUCK, owner->object_id());
        P.w_u16(item_in_slot->object().ID());
        CGameObject::u_EventSend(P);
    }
    CGameObject::u_EventGen(P, GEG_PLAYER_ITEM2SLOT, owner->object_id());
    P.w_u16(item->object().ID());
    P.w_u16(slot);
    CGameObject::u_EventSend(P);

    CGameObject::u_EventGen(P, GEG_PLAYER_ACTIVATE_SLOT, owner->object_id());
    P.w_u16(slot);
    CGameObject::u_EventSend(P);
}

//передаче вещи из своего инвентаря в инвентарь партнера
void CScriptGameObject::TransferItem(CScriptGameObject* pItem, CScriptGameObject* pForWho)
{
    if (!pItem || !pForWho)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "cannot transfer NULL item");
        return;
    }

    CInventoryItem* pIItem = smart_cast<CInventoryItem*>(&pItem->object());

    if (!pIItem)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot transfer not CInventoryItem item");
        return;
    }

    // выбросить у себя
    NET_Packet P;
    CGameObject::u_EventGen(P, GE_TRADE_SELL, object().ID());
    P.w_u16(pIItem->object().ID());
    CGameObject::u_EventSend(P);

    // отдать партнеру
    CGameObject::u_EventGen(P, GE_TRADE_BUY, pForWho->object().ID());
    P.w_u16(pIItem->object().ID());
    CGameObject::u_EventSend(P);
}

u32 CScriptGameObject::Money()
{
    CInventoryOwner* pOurOwner = smart_cast<CInventoryOwner*>(&object());
    VERIFY(pOurOwner);
    return pOurOwner->get_money();
}

void CScriptGameObject::TransferMoney(int money, CScriptGameObject* pForWho)
{
    if (!pForWho)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "cannot transfer money for NULL object");
        return;
    }
    CInventoryOwner* pOurOwner = smart_cast<CInventoryOwner*>(&object());
    VERIFY(pOurOwner);
    CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&pForWho->object());
    VERIFY(pOtherOwner);

    if (pOurOwner->get_money() - money < 0)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Character does not have enought money");
        return;
    }

    pOurOwner->set_money(pOurOwner->get_money() - money, true);
    pOtherOwner->set_money(pOtherOwner->get_money() + money, true);
}

void CScriptGameObject::GiveMoney(int money)
{
    CInventoryOwner* pOurOwner = smart_cast<CInventoryOwner*>(&object());
    VERIFY(pOurOwner);

    pOurOwner->set_money(pOurOwner->get_money() + money, true);
}
//////////////////////////////////////////////////////////////////////////

int CScriptGameObject::GetGoodwill(CScriptGameObject* pToWho)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "GetGoodwill available only for InventoryOwner");
        return 0;
    }
    return RELATION_REGISTRY().GetGoodwill(pInventoryOwner->object_id(), pToWho->object().ID());
}

void CScriptGameObject::SetGoodwill(int goodwill, CScriptGameObject* pWhoToSet)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetGoodwill available only for InventoryOwner");
        return;
    }
    RELATION_REGISTRY().SetGoodwill(pInventoryOwner->object_id(), pWhoToSet->object().ID(), goodwill);
}

void CScriptGameObject::ForceSetGoodwill(int goodwill, CScriptGameObject* pWhoToSet)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "ForceSetGoodwill available only for InventoryOwner");
        return;
    }
    RELATION_REGISTRY().ForceSetGoodwill(pInventoryOwner->object_id(), pWhoToSet->object().ID(), goodwill);
}

void CScriptGameObject::ChangeGoodwill(int delta_goodwill, CScriptGameObject* pWhoToSet)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "ChangeGoodwill available only for InventoryOwner");
        return;
    }
    RELATION_REGISTRY().ChangeGoodwill(pInventoryOwner->object_id(), pWhoToSet->object().ID(), delta_goodwill);
}

//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::SetRelation(ALife::ERelationType relation, CScriptGameObject* pWhoToSet)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetRelation available only for InventoryOwner");
        return;
    }

    CInventoryOwner* pOthersInventoryOwner = smart_cast<CInventoryOwner*>(&pWhoToSet->object());
    VERIFY(pOthersInventoryOwner);
    if (!pOthersInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetRelation available only for InventoryOwner");
        return;
    }
    RELATION_REGISTRY().SetRelationType(pInventoryOwner, pOthersInventoryOwner, relation);
}

float CScriptGameObject::GetSympathy()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "GetSympathy available only for InventoryOwner");
        return 0.0f;
    }
    return pInventoryOwner->Sympathy();
}

void CScriptGameObject::SetSympathy(float sympathy)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetSympathy available only for InventoryOwner");
        return;
    }
    pInventoryOwner->CharacterInfo().SetSympathy(sympathy);
}

int CScriptGameObject::GetCommunityGoodwill_obj(LPCSTR community)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "GetCommunityGoodwill available only for InventoryOwner");
        return 0;
    }
    CHARACTER_COMMUNITY c;
    c.set(community);

    return RELATION_REGISTRY().GetCommunityGoodwill(c.index(), pInventoryOwner->object_id());
}

void CScriptGameObject::SetCommunityGoodwill_obj(LPCSTR community, int goodwill)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "SetCommunityGoodwill available only for InventoryOwner");
        return;
    }
    CHARACTER_COMMUNITY c;
    c.set(community);

    RELATION_REGISTRY().SetCommunityGoodwill(c.index(), pInventoryOwner->object_id(), goodwill);
}

//////////////////////////////////////////////////////////////////////////

int CScriptGameObject::GetAttitude(CScriptGameObject* pToWho)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    //VERIFY(pInventoryOwner);
	if (!pInventoryOwner)
	{
		GEnv.ScriptEngine->script_log(LuaMessageType::Error, "GetAttitude available only for InventoryOwner");
		return 0;
	}
    CInventoryOwner* pOthersInventoryOwner = smart_cast<CInventoryOwner*>(&pToWho->object());
    VERIFY(pOthersInventoryOwner);
    return RELATION_REGISTRY().GetAttitude(pInventoryOwner, pOthersInventoryOwner);
}

//////////////////////////////////////////////////////////////////////////

LPCSTR CScriptGameObject::ProfileName()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "ProfileName available only for InventoryOwner");
        return NULL;
    }

    shared_str profile_id = pInventoryOwner->CharacterInfo().Profile();
    if (!profile_id || !profile_id.size())
        return NULL;
    else
        return *profile_id;
}

LPCSTR CScriptGameObject::CharacterName()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CharacterName available only for InventoryOwner");
        return NULL;
    }
    return pInventoryOwner->Name();
}

LPCSTR CScriptGameObject::CharacterIcon()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CharacterIconName available only for InventoryOwner");
        return NULL;
    }
    return pInventoryOwner->IconName();
}

int CScriptGameObject::CharacterRank()
{
    // rank support for monster
    CBaseMonster* monster = smart_cast<CBaseMonster*>(&object());
    if (!monster)
    {
        CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
        if (!pInventoryOwner)
        {
            GEnv.ScriptEngine->script_log(
                LuaMessageType::Error, "CharacterRank available only for InventoryOwner and BaseMonster");
            return 0;
        }
        return pInventoryOwner->Rank();
    }
    return monster->Rank();
}
void CScriptGameObject::SetCharacterRank(int char_rank)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetCharacterRank available only for InventoryOwner");
        return;
    }
    return pInventoryOwner->SetRank(char_rank);
}

void CScriptGameObject::ChangeCharacterRank(int char_rank)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "ChangeCharacterRank available only for InventoryOwner");
        return;
    }
    return pInventoryOwner->ChangeRank(char_rank);
}

int CScriptGameObject::CharacterReputation()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CharacterReputation available only for InventoryOwner");
        return 0;
    }
    return pInventoryOwner->Reputation();
}

void CScriptGameObject::ChangeCharacterReputation(int char_rep)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "ChangeCharacterReputation available only for InventoryOwner");
        return;
    }
    pInventoryOwner->ChangeReputation(char_rep);
}

LPCSTR CScriptGameObject::CharacterCommunity()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CharacterCommunity available only for InventoryOwner");
        return NULL;
    }
    return *pInventoryOwner->CharacterInfo().Community().id();
}

void CScriptGameObject::SetCharacterCommunity(LPCSTR comm, int squad, int group)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    CEntity* entity = smart_cast<CEntity*>(&object());

    if (!pInventoryOwner || !entity)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "SetCharacterCommunity available only for InventoryOwner");
        return;
    }
    CHARACTER_COMMUNITY community;
    community.set(comm);
    pInventoryOwner->SetCommunity(community.index());
    entity->ChangeTeam(community.team(), squad, group);
}

LPCSTR CScriptGameObject::sound_voice_prefix() const
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "sound_voice_prefix available only for InventoryOwner");
        return NULL;
    }

    return pInventoryOwner->SpecificCharacter().sound_voice_prefix();
}

#include "GametaskManager.h"
ETaskState CScriptGameObject::GetGameTaskState(LPCSTR task_id)
{
    shared_str shared_name = task_id;
    CGameTask* t = Level().GameTaskManager().HasGameTask(shared_name, true);

    if (NULL == t)
        return eTaskStateDummy;

    return t->GetTaskState();
}

void CScriptGameObject::SetGameTaskState(ETaskState state, LPCSTR task_id)
{
    shared_str shared_name = task_id;
    Level().GameTaskManager().SetTaskState(shared_name, state);
}

//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::SwitchToTrade()
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
        return;

    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->SwitchToTrade();
    }
}

void CScriptGameObject::SwitchToUpgrade()
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
        return;

    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->SwitchToUpgrade();
    }
}

void CScriptGameObject::SwitchToTalk() { R_ASSERT("switch_to_talk called ;)"); }
void CScriptGameObject::AllowBreakTalkDialog(bool b)
{
    CInventoryOwner* inv_owner = smart_cast<CInventoryOwner*>(&object());
    VERIFY(inv_owner);
    inv_owner->bDisableBreakDialog = !b;
}

void CScriptGameObject::RunTalkDialog(CScriptGameObject* pToWho, bool disable_break)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    //	R_ASSERT2(pActor, "RunTalkDialog applicable only for actor");

    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "RunTalkDialog applicable only for actor");
        return;
    }

    CInventoryOwner* pPartner = smart_cast<CInventoryOwner*>(&pToWho->object());
    VERIFY(pPartner);
    pActor->RunTalkDialog(pPartner, disable_break);
}

void CScriptGameObject::ActorLookAtPoint(Fvector point)
{
    CCameraBase* c = Actor()->cam_FirstEye();
    CCameraFirstEye* cf = smart_cast<CCameraFirstEye*>(c);
    cf->LookAtPoint(point);
}

//////////////////////////////////////////////////////////////////////////

void construct_restriction_vector(shared_str restrictions, xr_vector<ALife::_OBJECT_ID>& result)
{
    result.clear();
    string64 temp;
    u32 n = _GetItemCount(*restrictions);
    for (u32 i = 0; i < n; ++i)
    {
        IGameObject* object = Level().Objects.FindObjectByName(_GetItem(*restrictions, i, temp));
        if (!object)
            continue;
        result.push_back(object->ID());
    }
}

void CScriptGameObject::add_restrictions(LPCSTR out, LPCSTR in)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member add_restrictions!");
        return;
    }

    //	Msg	( "object[%s] add_restrictions( \"%s\", \"%s\" )", monster->cName().c_str(), out, in );
    monster->movement().restrictions().add_restrictions(out, in);
}

void CScriptGameObject::remove_restrictions(LPCSTR out, LPCSTR in)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member remove_restrictions!");
        return;
    }

    //	Msg	( "object[%s] remove_restrictions( \"%s\", \"%s\" )", monster->cName().c_str(), out, in );
    monster->movement().restrictions().remove_restrictions(out, in);
}

void CScriptGameObject::remove_all_restrictions()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member remove_all_restrictions!");
        return;
    }

    //	Msg	( "object[%s] remove_all_restrictions( )", monster->cName().c_str() );
    monster->movement().restrictions().remove_all_restrictions();
}

LPCSTR CScriptGameObject::in_restrictions()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member in_restrictions!");
        return ("");
    }
    return (*monster->movement().restrictions().in_restrictions());
}

LPCSTR CScriptGameObject::out_restrictions()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member out_restrictions!");
        return ("");
    }
    return (*monster->movement().restrictions().out_restrictions());
}

LPCSTR CScriptGameObject::base_in_restrictions()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member base_in_restrictions!");
        return ("");
    }
    return (*monster->movement().restrictions().base_in_restrictions());
}

LPCSTR CScriptGameObject::base_out_restrictions()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member base_out_restrictions!");
        return ("");
    }
    return (*monster->movement().restrictions().base_out_restrictions());
}

bool CScriptGameObject::accessible_position(const Fvector& position)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member accessible!");
        return (false);
    }
    return (monster->movement().restrictions().accessible(position));
}

bool CScriptGameObject::accessible_vertex_id(u32 level_vertex_id)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member accessible!");
        return (false);
    }
    if (!ai().level_graph().valid_vertex_id(level_vertex_id))
        return false;
    THROW2(ai().level_graph().valid_vertex_id(level_vertex_id),
        "Cannot check if level vertex id is accessible, because it is invalid");
    return (monster->movement().restrictions().accessible(level_vertex_id));
}

u32 CScriptGameObject::accessible_nearest(const Fvector& position, Fvector& result)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CRestrictedObject : cannot access class member accessible!");
        return (u32(-1));
    }
    if (monster->movement().restrictions().accessible(position))
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "CRestrictedObject : you use accessible_nearest when position is already accessible!");
        return (u32(-1));
    }
    return (monster->movement().restrictions().accessible_nearest(position, result));
}

void CScriptGameObject::enable_attachable_item(bool value)
{
    CAttachableItem* attachable_item = smart_cast<CAttachableItem*>(&object());
    if (!attachable_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAttachableItem : cannot access class member enable_attachable_item!");
        return;
    }
    attachable_item->enable(value);
}

bool CScriptGameObject::attachable_item_enabled() const
{
    CAttachableItem* attachable_item = smart_cast<CAttachableItem*>(&object());
    if (!attachable_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAttachableItem : cannot access class member attachable_item_enabled!");
        return (false);
    }
    return (attachable_item->enabled());
}

void CScriptGameObject::enable_night_vision(bool value)
{
    CTorch* torch = smart_cast<CTorch*>(&object());
    if (!torch)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CTorch : cannot access class member enable_night_vision!");
        return;
    }
    torch->SwitchNightVision(value);
}

bool CScriptGameObject::night_vision_enabled() const
{
    CTorch* torch = smart_cast<CTorch*>(&object());
    if (!torch)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CTorch : cannot access class member enable_night_vision!");
        return (false);
    }
    return (torch->GetNightVisionStatus());
}

void CScriptGameObject::enable_torch(bool value)
{
    CTorch* torch = smart_cast<CTorch*>(&object());
    if (!torch)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CTorch : cannot access class member enable_torch!");
        return;
    }
    torch->Switch(value);
}

bool CScriptGameObject::torch_enabled() const
{
    CTorch* torch = smart_cast<CTorch*>(&object());
    if (!torch)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CTorch : cannot access class member torch_enabled!");
        return (false);
    }
    return (torch->torch_active());
}

void CScriptGameObject::attachable_item_load_attach(LPCSTR section)
{
    CAttachableItem* attachable_item = smart_cast<CAttachableItem*>(&object());
    if (!attachable_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAttachableItem : cannot access class member attachable_item_load_attach!");
        return;
    }
    attachable_item->load_attach_position(section);

    if (attachable_item->object().H_Parent())
    { // reattach
        CAttachmentOwner* AO = smart_cast<CAttachmentOwner*>(attachable_item->object().H_Parent());
        if (AO)
            AO->reattach_items();
    }
}

void CScriptGameObject::RestoreWeapon()
{
#ifdef DEBUG
    GEnv.ScriptEngine->script_log(LuaMessageType::Message, "CScriptGameObject::RestoreWeapon called!!!");
    GEnv.ScriptEngine->print_stack();
#endif //#ifdef DEBUG
    Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, false);
}

void CScriptGameObject::HideWeapon()
{
#ifdef DEBUG
    GEnv.ScriptEngine->script_log(LuaMessageType::Message, "CScriptGameObject::HideWeapon called!!!");
    GEnv.ScriptEngine->print_stack();
#endif //#ifdef DEBUG
    Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);
}

int CScriptGameObject::Weapon_GrenadeLauncher_Status()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_GrenadeLauncher_Status!");
        return (false);
    }
    return (int)weapon->get_GrenadeLauncherStatus();
}

int CScriptGameObject::Weapon_Scope_Status()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_Scope_Status!");
        return (false);
    }
    return (int)weapon->get_ScopeStatus();
}

int CScriptGameObject::Weapon_Silencer_Status()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_Silencer_Status!");
        return (false);
    }
    return (int)weapon->get_SilencerStatus();
}

bool CScriptGameObject::Weapon_IsGrenadeLauncherAttached()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_IsGrenadeLauncherAttached!");
        return (false);
    }
    return weapon->IsGrenadeLauncherAttached();
}

bool CScriptGameObject::Weapon_IsScopeAttached()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_IsScopeAttached!");
        return (false);
    }
    return weapon->IsScopeAttached();
}

bool CScriptGameObject::Weapon_IsSilencerAttached()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeapon : cannot access class member Weapon_IsSilencerAttached!");
        return (false);
    }
    return weapon->IsSilencerAttached();
}

void CScriptGameObject::AllowSprint(bool b) { Actor()->SetCantRunState(!b); }
int CScriptGameObject::animation_slot() const
{
    CHudItem* hud_item = smart_cast<CHudItem*>(&object());
    if (!hud_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CHudItem : cannot access class member animation_slot!");
        return (u32(-1));
    }
    return (hud_item->animation_slot());
}

CScriptGameObject* CScriptGameObject::active_detector() const
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member active_detector!");
        return (0);
    }

    CInventoryItem* result = inventory_owner->inventory().ItemFromSlot(DETECTOR_SLOT);
    if (result)
    {
        CCustomDetector* detector = smart_cast<CCustomDetector*>(result);
        VERIFY(detector);
        return (detector->IsWorking() ? result->object().lua_game_object() : 0);
    }
    return (0);
}

CScriptGameObject* CScriptGameObject::item_in_slot(u32 slot_id) const
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member item_in_slot!");
        return (0);
    }

    CInventoryItem* result = inventory_owner->inventory().ItemFromSlot((u16)slot_id);
    return (result ? result->object().lua_game_object() : 0);
}

void CScriptGameObject::GiveTaskToActor(CGameTask* t, u32 dt, bool bCheckExisting, u32 t_timer)
{
    Level().GameTaskManager().GiveGameTaskToActor(t, dt, bCheckExisting, t_timer);
}

CGameTask* CScriptGameObject::GetTask(LPCSTR id, bool only_inprocess)
{
    return Level().GameTaskManager().HasGameTask(id, only_inprocess);
}

void CScriptGameObject::SetActiveTask(CGameTask* t)
{
    VERIFY(t);
    Level().GameTaskManager().SetActiveTask(t);
}

bool CScriptGameObject::IsActiveTask(CGameTask* t)
{
    VERIFY(t);
    return Level().GameTaskManager().ActiveTask() == t;
}

u32 CScriptGameObject::active_slot()
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member active_slot!");
        return (0);
    }
    return inventory_owner->inventory().GetActiveSlot();
}

void CScriptGameObject::activate_slot(u32 slot_id)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member activate_slot!");
        return;
    }
    inventory_owner->inventory().Activate((u16)slot_id);
}

void CScriptGameObject::enable_movement(bool enable)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member movement_enabled!");
        return;
    }

    monster->movement().enable_movement(enable);
}

bool CScriptGameObject::movement_enabled()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member movement_enabled!");
        return (false);
    }

    return (monster->movement().enabled());
}

bool CScriptGameObject::can_throw_grenades() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member can_throw_grenades!");
        return (false);
    }

    return (stalker->can_throw_grenades());
}

void CScriptGameObject::can_throw_grenades(bool can_throw_grenades)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member can_throw_grenades!");
        return;
    }

    stalker->can_throw_grenades(can_throw_grenades);
}

u32 CScriptGameObject::throw_time_interval() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member throw_time_interval!");
        return (0);
    }

    return (stalker->throw_time_interval());
}

void CScriptGameObject::throw_time_interval(u32 throw_time_interval)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member throw_time_interval!");
        return;
    }

    stalker->throw_time_interval(throw_time_interval);
}

u32 CScriptGameObject::group_throw_time_interval() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member group_throw_time_interval!");
        return (0);
    }

    return (stalker->agent_manager().member().throw_time_interval());
}

void CScriptGameObject::group_throw_time_interval(u32 throw_time_interval)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member group_throw_time_interval!");
        return;
    }

    stalker->agent_manager().member().throw_time_interval(throw_time_interval);
}

void CScriptGameObject::aim_time(CScriptGameObject* weapon, u32 aim_time)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_time!");
        return;
    }

    CWeapon* weapon_ = smart_cast<CWeapon*>(&weapon->object());
    if (!weapon_)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_time (not a weapon passed)!");
        return;
    }

    stalker->aim_time(*weapon_, aim_time);
}

u32 CScriptGameObject::aim_time(CScriptGameObject* weapon)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_time!");
        return (u32(-1));
    }

    CWeapon* weapon_ = smart_cast<CWeapon*>(&weapon->object());
    if (!weapon_)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_time (not a weapon passed)!");
        return (u32(-1));
    }

    return (stalker->aim_time(*weapon_));
}

void CScriptGameObject::special_danger_move(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member special_danger_move!");
        return;
    }

    stalker->animation().special_danger_move(value);
}

bool CScriptGameObject::special_danger_move()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member special_danger_move!");
        return (false);
    }

    return (stalker->animation().special_danger_move());
}

void CScriptGameObject::sniper_update_rate(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member sniper_update_rate!");
        return;
    }

    stalker->sniper_update_rate(value);
}

bool CScriptGameObject::sniper_update_rate() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member sniper_update_rate!");
        return (false);
    }

    return (stalker->sniper_update_rate());
}

void CScriptGameObject::sniper_fire_mode(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member sniper_fire_mode!");
        return;
    }

    stalker->sniper_fire_mode(value);
}

bool CScriptGameObject::sniper_fire_mode() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member sniper_fire_mode!");
        return (false);
    }

    return (stalker->sniper_fire_mode());
}

void CScriptGameObject::aim_bone_id(LPCSTR bone_id)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_bone_id!");
        return;
    }

    stalker->aim_bone_id(bone_id);
}

LPCSTR CScriptGameObject::aim_bone_id() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member aim_bone_id!");
        return nullptr;
    }

    return (stalker->aim_bone_id().c_str());
}

void CScriptGameObject::register_in_combat()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member register_in_combat!");
        return;
    }

    stalker->agent_manager().member().register_in_combat(stalker);
}

void CScriptGameObject::unregister_in_combat()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member unregister_in_combat!");
        return;
    }

    stalker->agent_manager().member().unregister_in_combat(stalker);
}

CCoverPoint const* CScriptGameObject::find_best_cover(Fvector position_to_cover_from)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member find_best_cover!");
        return (0);
    }

    return (stalker->find_best_cover(position_to_cover_from));
}

bool CScriptGameObject::suitable_smart_cover(CScriptGameObject* object)
{
    if (!object)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker::suitable_smart_cover null smart cover specified!");
        return (false);
    }

    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member suitable_smart_cover!");
        return (false);
    }

    smart_cover::object const* const smart_object = smart_cast<smart_cover::object const*>(&object->object());
    if (!smart_object)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : suitable_smart_cover: passed non-smart_cover object!");
        return (false);
    }

    smart_cover::cover const& cover = smart_object->get_cover();
    if (!cover.can_fire())
        return (true);

    CInventoryItem const* inventory_item = stalker->inventory().ActiveItem();
    if (inventory_item)
        return (inventory_item->BaseSlot() == INV_SLOT_3);

    CInventoryItem const* best_weapon = stalker->best_weapon();
    if (!best_weapon)
        return (false);

    return (best_weapon->BaseSlot() == INV_SLOT_3);
}

void CScriptGameObject::take_items_enabled(bool const value)
{
    CAI_Stalker* const stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member take_items_enabled!");
        return;
    }

    stalker->take_items_enabled(value);
}

bool CScriptGameObject::take_items_enabled() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member take_items_enabled!");
        return (false);
    }

    return (stalker->take_items_enabled());
}

void CScriptGameObject::SetPlayShHdRldSounds(bool val)
{
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(&object());
    if (!owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member SetPlayShHdRldSounds!");
        return;
    }
    owner->SetPlayShHdRldSounds(val);
}

void CScriptGameObject::death_sound_enabled(bool const value)
{
    CAI_Stalker* const stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member death_sound_enabled!");
        return;
    }

    stalker->death_sound_enabled(value);
}

bool CScriptGameObject::death_sound_enabled() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member death_sound_enabled!");
        return (false);
    }

    return (stalker->death_sound_enabled());
}

void CScriptGameObject::register_door()
{
    VERIFY2(!m_door, make_string("object %s has been registered as a door already", m_game_object->cName().c_str()));
    m_door = ai().doors().register_door(*smart_cast<CPhysicObject*>(m_game_object));
    //	Msg									( "registering door 0x%-08x", m_door );
}

void CScriptGameObject::unregister_door()
{
    VERIFY2(m_door, make_string("object %s is not a door", m_game_object->cName().c_str()));
    //	Msg									( "UNregistering door 0x%-08x", m_door );
    ai().doors().unregister_door(m_door);
    m_door = 0;
}

void CScriptGameObject::on_door_is_open()
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    ai().doors().on_door_is_open(m_door);
}

void CScriptGameObject::on_door_is_closed()
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    ai().doors().on_door_is_closed(m_door);
}

bool CScriptGameObject::is_door_locked_for_npc() const
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    return ai().doors().is_door_locked(m_door);
}

void CScriptGameObject::lock_door_for_npc()
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    ai().doors().lock_door(m_door);
}

void CScriptGameObject::unlock_door_for_npc()
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    ai().doors().unlock_door(m_door);
}

bool CScriptGameObject::is_door_blocked_by_npc() const
{
    VERIFY2(m_door, make_string("object %s hasn't been registered as a door already", m_game_object->cName().c_str()));
    return ai().doors().is_door_blocked(m_door);
}


//Alundaio: Methods for exporting the ability to detach/attach addons for magazined weapons
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
void CScriptGameObject::Weapon_AddonAttach(CScriptGameObject* item)
{
    auto weapon = smart_cast<CWeaponMagazined*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CWeaponMagazined : cannot access class member Weapon_AddonAttach!");
        return;
    }

    CInventoryItem* pItm = item->object().cast_inventory_item();
    if (!pItm)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CWeaponMagazined : trying to attach non-CInventoryItem!");
        return;
    }

    if (weapon->CanAttach(pItm))
        weapon->Attach(pItm, true);
}

void CScriptGameObject::Weapon_AddonDetach(pcstr item_section)
{
    auto weapon = smart_cast<CWeaponMagazined*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CWeaponMagazined : cannot access class member Weapon_AddonDetach!");
        return;
    }

    if (weapon->CanDetach(item_section))
        weapon->Detach(item_section, true);
}

bool CScriptGameObject::InstallUpgrade(pcstr upgrade)
{
    CInventoryItem* item = smart_cast<CInventoryItem*>(&object());
    if (!item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CInventoryItem : cannot access class member InstallUpgrade!");
        return false;
    }

    if (!pSettings->section_exist(upgrade))
        return false;

    return ai().alife().inventory_upgrade_manager().upgrade_install(*item, upgrade, false);
}

bool CScriptGameObject::HasUpgrade(pcstr upgrade) const
{
    CInventoryItem* item = smart_cast<CInventoryItem*>(&object());
    if (!item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CInventoryItem : cannot access class member HasUpgrade!");
        return false;
    }

    if (!pSettings->section_exist(upgrade))
        return false;

    return item->has_upgrade(upgrade);
}

void CScriptGameObject::IterateInstalledUpgrades(luabind::functor<void> functor)
{
    CInventoryItem* Item = smart_cast<CInventoryItem*>(&object());
    if (!Item)
        return;

    for (auto upgrade : Item->get_upgrades())
        functor(upgrade.c_str(), object().lua_game_object());
}

CScriptGameObject* CScriptGameObject::ItemOnBelt(u32 item_id) const
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CInventoryOwner : cannot access class member item_on_belt!");
        return nullptr;
    }

    TIItemContainer* belt = &inventory_owner->inventory().m_belt;
    if (belt->size() < item_id)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "item_on_belt: item id outside belt!");
        return nullptr;
    }

    CInventoryItem* result = belt->at(item_id);
    return result ? result->object().lua_game_object() : nullptr;
}

bool CScriptGameObject::IsOnBelt(CScriptGameObject* obj) const
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&obj->object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CInventoryItem : cannot access class member is_on_belt!");
        return false;
    }

    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CInventoryOwner : cannot access class member is_on_belt!");
        return false;
    }

    return inventory_owner->inventory().InBelt(inventory_item);
}

u32 CScriptGameObject::BeltSize() const
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CInventoryOwner : cannot access class member move_to_belt!");
        return 0;
    }

    return inventory_owner->inventory().m_belt.size();
}

float CScriptGameObject::GetActorMaxWeight() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorMaxWeight!");
        return false;
    }
    return pActor->inventory().GetMaxWeight();
}

void CScriptGameObject::SetActorMaxWeight(float max_weight)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorMaxWeight!");
        return;
    }
    pActor->inventory().SetMaxWeight(max_weight);
}

// получить и задать максимальный вес при котором можно ходить
float CScriptGameObject::GetActorMaxWalkWeight() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorMaxWalkWeight!");
        return false;
    }
    return pActor->conditions().m_MaxWalkWeight;
}

void CScriptGameObject::SetActorMaxWalkWeight(float max_walk_weight)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorMaxWalkWeight!");
        return;
    }
    pActor->conditions().m_MaxWalkWeight = max_walk_weight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// получить и задать доп. вес для костюма
float CScriptGameObject::GetAdditionalMaxWeight() const
{
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(&object());
    if (!outfit)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CCustomOutfit : cannot access class member GetAdditionalMaxWeight!");
        return false;
    }
    return outfit->m_additional_weight2;
}

float CScriptGameObject::GetAdditionalMaxWalkWeight() const
{
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(&object());
    if (!outfit)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CCustomOutfit : cannot access class member GetAdditionalMaxWalkWeight!");
        return false;
    }
    return outfit->m_additional_weight;
}

void CScriptGameObject::SetAdditionalMaxWeight(float add_max_weight)
{
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(&object());
    if (!outfit)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CCustomOutfit : cannot access class member SetAdditionalMaxWeight!");
        return;
    }
    outfit->m_additional_weight2 = add_max_weight;
}

void CScriptGameObject::SetAdditionalMaxWalkWeight(float add_max_walk_weight)
{
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(&object());
    if (!outfit)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CCustomOutfit : cannot access class member SetAdditionalMaxWalkWeight!");
        return;
    }
    outfit->m_additional_weight = add_max_walk_weight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// получить суммарный вес инвентаря
float CScriptGameObject::GetTotalWeight() const
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CInventoryOwner : cannot access class member GetTotalWeight!");
        return false;
    }
    return inventory_owner->inventory().TotalWeight();
}

// получить вес предмета
float CScriptGameObject::Weight() const
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CSciptEntity : cannot access class member Weight!");
        return false;
    }
    return inventory_item->Weight();
}

void CScriptGameObject::SetWeight(float w)
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CSciptEntity : cannot access class member SetWeight!");
        return;
    }
    inventory_item->SetWeight(w);
}

float CScriptGameObject::GetActorJumpSpeed() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorJumpSpeed!");
        return false;
    }
    return pActor->m_fJumpSpeed;
}

void CScriptGameObject::SetActorJumpSpeed(float jump_speed)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorJumpSpeed!");
        return;
    }
    pActor->m_fJumpSpeed = jump_speed;
    //character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed);  
}

float CScriptGameObject::GetActorSprintKoef() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorJumpSpeed!");
        return false;
    }
    return pActor->m_fSprintFactor;
}

void CScriptGameObject::SetActorSprintKoef(float sprint_koef)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorJumpSpeed!");
        return;
    }
    pActor->m_fSprintFactor = sprint_koef;
}

float CScriptGameObject::GetActorRunCoef() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorJumpSpeed!");
        return false;
    }
    return pActor->m_fRunFactor;
}

void CScriptGameObject::SetActorRunCoef(float run_coef)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorJumpSpeed!");
        return;
    }
    pActor->m_fRunFactor = run_coef;
}

float CScriptGameObject::GetActorRunBackCoef() const
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member GetActorJumpSpeed!");
        return false;
    }
    return pActor->m_fRunBackFactor;
}

void CScriptGameObject::SetActorRunBackCoef(float run_back_coef)
{
    CActor* pActor = smart_cast<CActor*>(&object());
    if (!pActor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                                        "CActor : cannot access class member SetActorJumpSpeed!");
        return;
    }
    pActor->m_fRunBackFactor = run_back_coef;
}

void CScriptGameObject::SetCharacterIcon(pcstr iconName)
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(&object());

    if (!pInventoryOwner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "SetCharacterIcon available only for InventoryOwner");
        return;
    }
    return pInventoryOwner->SetIcon(iconName);
}
#endif
//-Alundaio
