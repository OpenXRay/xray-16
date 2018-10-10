#include "StdAfx.h"
#include "game_state_accumulator.h"
#include "Level.h"
#include "actor_mp_client.h"
#include "CustomOutfit.h"
#include "Weapon.h"
#include "game_cl_deathmatch.h"
#include "UIGameCTA.h"
#include "ui/UIMpTradeWnd.h"
#include "ui/UIBuyWndShared.h"

namespace award_system
{
game_state_accumulator::game_state_accumulator()
{
    m_last_player_spawn_time = 0;
    m_local_player = NULL;
    m_item_mngr = NULL;
}

game_state_accumulator::~game_state_accumulator() { delete_data(m_accumulative_values); }
void game_state_accumulator::update()
{
    // update_average_values		();
    update_accumulative_values();
}

void game_state_accumulator::init_bone_groups(CActor* first_spawned_actor) { m_bone_groups.init(first_spawned_actor); }
void game_state_accumulator::init()
{
    // init_average_values	();
    init_accumulative_values();
}

void game_state_accumulator::init_player(game_PlayerState* local_player)
{
    m_local_player = local_player;
    init_player_accum_values(local_player);

    CUIMpTradeWnd* tmp_trade_wnd = NULL;
    game_cl_Deathmatch* tmp_dm_game = smart_cast<game_cl_Deathmatch*>(&Game());
    if (tmp_dm_game)
    {
        tmp_trade_wnd = smart_cast<CUIMpTradeWnd*>(tmp_dm_game->GetBuyWnd());
    }
    else
    {
        R_ASSERT(Game().Type() == eGameIDCaptureTheArtefact);
        CUIGameCTA* tmp_cta_ui = smart_cast<CUIGameCTA*>(CurrentGameUI());
        VERIFY(tmp_cta_ui);
        tmp_trade_wnd = smart_cast<CUIMpTradeWnd*>(tmp_cta_ui->GetBuyWnd());
    }
    R_ASSERT(tmp_trade_wnd);
    m_item_mngr = tmp_trade_wnd->GetItemMngr();
    R_ASSERT(m_item_mngr);
    m_amm_groups.init(m_item_mngr);
}

void game_state_accumulator::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnWeapon_Fire(sender, sender_weapon_id);
    }
}
void game_state_accumulator::OnBullet_Fire(
    u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnBullet_Fire(sender, sender_weapon_id, position, direction);
    }
}

void game_state_accumulator::OnBullet_Hit(
    IGameObject const* hitter, IGameObject const* victim, IGameObject const* weapon, u16 const bone)
{
    if (!hitter || !victim || !weapon)
        return;

    float bullet_fly_dist = 0.0f;

    bullet_fly_dist = hitter->Position().distance_to(victim->Position());

    m_hits.add_hit(hitter->cName(), victim->cName(), get_object_id(weapon), bone, bullet_fly_dist);
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnBullet_Hit(hitter, victim, weapon, bone);
    }
}

void game_state_accumulator::OnArtefactSpawned()
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnArtefactSpawned();
    }
}

void game_state_accumulator::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerTakeArtefact(ps);
    }
}

void game_state_accumulator::OnPlayerDropArtefact(game_PlayerState const* ps)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerDropArtefact(ps);
    }
}

void game_state_accumulator::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerBringArtefact(ps);
    }
}

void game_state_accumulator::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (ps == m_local_player)
    {
        m_last_player_spawn_time = Device.dwTimeGlobal;
    }
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerSpawned(ps);
    }
}

void game_state_accumulator::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    if (!is_enemies(killer_id, target_id))
        return;

    IGameObject* killer_obj = Level().Objects.net_Find(killer_id);
    IGameObject* victim_obj = Level().Objects.net_Find(target_id);
    if (killer_obj && victim_obj)
    {
        m_kills.add_kill(
            killer_obj->cName(), victim_obj->cName(), get_object_id(weapon_id), kill_type.first, kill_type.second);
    }

    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerKilled(killer_id, target_id, weapon_id, kill_type);
    }
}

void game_state_accumulator::OnPlayerChangeTeam(s8 team)
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerChangeTeam(team);
    }
}
void game_state_accumulator::OnPlayerRankChanged()
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnPlayerRankChanged();
    }
}

void game_state_accumulator::OnRoundEnd()
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnRoundEnd();
    }
}
void game_state_accumulator::OnRoundStart()
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->OnRoundStart();
    }
}

u16 game_state_accumulator::get_object_id(IGameObject const* obj)
{
    if (!obj)
        return 0;

    R_ASSERT2(m_item_mngr, "item manager not initialized");
    u32 itm_index = m_item_mngr->GetItemIdx(obj->cNameSect());
    if (itm_index == u32(-1))
        return 0;

    VERIFY((itm_index & 0xffff0000) == 0);
    return static_cast<u16>(itm_index);
}

u16 game_state_accumulator::get_object_id(u16 item_object_id)
{
    IGameObject* tmp_object = Level().Objects.net_Find(item_object_id);
    if (!tmp_object)
        return 0;

    return get_object_id(tmp_object);
}

void game_state_accumulator::reset_player_game()
{
    m_kills.clear();
    m_hits.clear();
    m_last_player_spawn_time = 0;

    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->reset_game();
    }
}

void game_state_accumulator::init_player_accum_values(game_PlayerState* new_local_player)
{
    for (int i = 0; i < acpv_count; ++i)
    {
        accumulative_values_collection_t::iterator tmp_iter =
            m_accumulative_values.find(static_cast<enum_accumulative_player_values>(i));
        R_ASSERT(tmp_iter != m_accumulative_values.end());
        tmp_iter->second->init_player(new_local_player);
    }
}

void game_state_accumulator::update_accumulative_values()
{
    for (accumulative_values_collection_t::iterator i = m_accumulative_values.begin(), ie = m_accumulative_values.end();
         i != ie; ++i)
    {
        i->second->update();
    }
}

bool game_state_accumulator::check_hit_params(u32 count, ammunition_group::enum_group_id weapon_group_id,
    bone_group::enum_group_id bone_group_id, float_binary_function* func, float right_dist_arg)
{
    struct hit_fetcher
    {
        bool operator()(shared_str const& hitter, shared_str const& victim, hits_store::bullet_hit const& hit)
        {
            if (m_hitter_name != hitter)
                return false;

            if (!m_owner->is_item_in_group(hit.m_weapon_id, ammunition_group::gid_sniper_rifels))
                return false;

            if (!m_func->exec(hit.m_dist, m_right_arg))
                return false;

            if (!m_owner->is_bone_in_group(hit.m_bone_id, m_bone_gid))
                return false;

            return true;
        }
        shared_str m_hitter_name;
        game_state_accumulator* m_owner;
        ammunition_group::enum_group_id m_weapon_gid;
        bone_group::enum_group_id m_bone_gid;
        float_binary_function* m_func;
        float m_right_arg;
    }; // struct hit_fetcher

    if (!m_local_player)
        return false;

    hit_fetcher tmp_fetcher;
    tmp_fetcher.m_hitter_name = m_local_player->getName();
    tmp_fetcher.m_owner = this;
    tmp_fetcher.m_weapon_gid = weapon_group_id;
    tmp_fetcher.m_bone_gid = bone_group_id;
    tmp_fetcher.m_func = func;
    tmp_fetcher.m_right_arg = right_dist_arg;

    buffer_vector<hits_store::bullet_hit> tmp_fake_buffer(NULL, 0);
    if (m_hits.fetch_hits(tmp_fetcher, tmp_fake_buffer) >= count)
    {
        return true;
    }

    return false;
}

bool game_state_accumulator::check_kill_params(u32 count, ammunition_group::enum_group_id weapon_group_id,
    KILL_TYPE kill_type, SPECIAL_KILL_TYPE special_kill_type, u32 time_period)
{
    struct kills_fetcher
    {
        bool operator()(shared_str const& killer, shared_str const& victim, kills_store::kill const& kill)
        {
            if (killer != m_killer_name)
                return false;

            if (kill.m_kill_time < m_after_time)
                return false;

            if (!m_owner->is_item_in_group(kill.m_weapon_id, m_weapon_gid))
                return false;

            if (kill.m_kill_type != m_kill_type)
                return false;

            if ((m_spec_kill != SKT_NONE) && (kill.m_spec_kill_type != m_spec_kill))
            {
                return false;
            }

            return true;
        }
        shared_str m_killer_name;
        u32 m_after_time;
        game_state_accumulator* m_owner;
        KILL_TYPE m_kill_type;
        SPECIAL_KILL_TYPE m_spec_kill;
        ammunition_group::enum_group_id m_weapon_gid;
    }; // struct kills_fetcher

    if (!m_local_player)
        return false;

    kills_fetcher tmp_predicate;
    tmp_predicate.m_killer_name = m_local_player->getName();
    if (time_period == u32(-1))
    {
        tmp_predicate.m_after_time = m_last_player_spawn_time;
    }
    else
    {
        tmp_predicate.m_after_time = Device.dwTimeGlobal - time_period;
    }
    tmp_predicate.m_owner = this;
    tmp_predicate.m_kill_type = kill_type;
    tmp_predicate.m_spec_kill = special_kill_type;
    tmp_predicate.m_weapon_gid = weapon_group_id;

    buffer_vector<kills_store::kill> tmp_buffer(NULL, 0);
    if (m_kills.fetch_kills(tmp_predicate, tmp_buffer) >= count)
        return true;

    return false;
}

bool game_state_accumulator::check_accumulative_value(
    enum_accumulative_player_values param_id, float_binary_function* func, float right_arg)
{
    accumulative_values_collection_t::iterator tmp_iter = m_accumulative_values.find(param_id);
    VERIFY2(
        tmp_iter != m_accumulative_values.end(), make_string("accumulative parameter %d not found", param_id).c_str());

    return func->exec(tmp_iter->second->get_float_param(), right_arg);
}

bool game_state_accumulator::check_accumulative_value(
    enum_accumulative_player_values param_id, u32_binary_function* func, u32 right_arg)
{
    accumulative_values_collection_t::iterator tmp_iter = m_accumulative_values.find(param_id);
    VERIFY2(
        tmp_iter != m_accumulative_values.end(), make_string("accumulative parameter %d not found", param_id).c_str());
    bool ret_value = func->exec(tmp_iter->second->get_u32_param(), right_arg);
#ifdef DEBUG
    Msg("* checking accumulative value: %s, result = %s", player_values_strtable[param_id],
        ret_value ? "true" : "false");
#endif
    return ret_value;
}

bool game_state_accumulator::is_item_in_group(u16 item_id, ammunition_group::enum_group_id gid)
{
    return m_amm_groups.is_item_in_group(item_id, gid);
}

bool game_state_accumulator::is_bone_in_group(u16 bone_id, bone_group::enum_group_id gid)
{
    return m_bone_groups.is_bone_in_group(bone_id, gid);
}

u16 game_state_accumulator::get_armor_of_player(game_PlayerState* player)
{
    if (!player)
        return 0;

    CActorMP const* tmp_actor = smart_cast<CActorMP const*>(Level().Objects.net_Find(player->GameID));

    if (!tmp_actor)
        return 0;

    CCustomOutfit* tmp_outfit = tmp_actor->GetOutfit();
    if (!tmp_outfit)
        return 0;

    return get_object_id(tmp_outfit);
}

u16 game_state_accumulator::get_active_weapon_of_player(game_PlayerState* player)
{
    if (!player)
        return 0;

    CActorMP const* tmp_actor = smart_cast<CActorMP const*>(Level().Objects.net_Find(player->GameID));

    if (!tmp_actor)
        return 0;

    u16 tmp_active_slot = tmp_actor->inventory().GetActiveSlot();
    CInventoryItem const* tmp_inv_item =
        tmp_active_slot != NO_ACTIVE_SLOT ? tmp_actor->inventory().ItemFromSlot(tmp_active_slot) : NULL;

    if (!tmp_inv_item)
        return 0;

    return get_object_id(tmp_inv_item->object_id());
}

CWeapon* game_state_accumulator::get_active_weapon(game_PlayerState* player)
{
    if (!player)
        return NULL;

    IGameObject* tmp_obj = Level().Objects.net_Find(player->GameID);
    if (!tmp_obj)
        return NULL;

    CActorMP const* tmp_actor = smart_cast<CActorMP const*>(tmp_obj);

    if (!tmp_actor)
        return NULL;

    u16 tmp_active_slot = tmp_actor->inventory().GetActiveSlot();
    CInventoryItem* tmp_inv_item =
        tmp_active_slot != NO_ACTIVE_SLOT ? tmp_actor->inventory().ItemFromSlot(tmp_active_slot) : NULL;

    if (!tmp_inv_item)
        return NULL;

    return smart_cast<CWeapon*>(tmp_inv_item);
}

CActor* game_state_accumulator::get_players_actor(u16 game_id)
{
    IGameObject* tmp_obj = Level().Objects.net_Find(game_id);
    if (!tmp_obj)
        return NULL;

    return smart_cast<CActor*>(tmp_obj);
}

bool game_state_accumulator::is_enemies(u16 left_pid, u16 right_pid) const
{
    game_PlayerState const* tmp_lp = Game().GetPlayerByGameID(left_pid);
    game_PlayerState const* tmp_rp = Game().GetPlayerByGameID(right_pid);
    return is_enemies(tmp_lp, tmp_rp);
}

bool game_state_accumulator::is_enemies(game_PlayerState const* left_player, game_PlayerState const* right_player) const
{
    if (!left_player || !right_player)
        return false;

    if (left_player == right_player)
        return false;

    if (Game().Type() == eGameIDDeathmatch)
        return true;

    if (left_player->team != right_player->team)
        return true;

    return false;
}

} // namespace award_system
