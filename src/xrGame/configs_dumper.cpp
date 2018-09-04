#include "stdafx.h"
#include "configs_dumper.h"
#include "configs_common.h"
#include "xrCore/Compression/ppmd_compressor.h"
#include "xrCore/xr_ini.h"
#include "xrCore/buffer_vector.h"
#include "GameObject.h"
#include "Level.h"
#include "actor_mp_client.h"
#include "inventory.h"
#include "weapon.h"
#include "game_cl_mp.h"

namespace mp_anticheat
{
configs_dumper::configs_dumper()
#ifdef DEBUG
    : m_start_time(0)
#endif
{
    m_state = ds_not_active;
    m_buffer_for_compress = nullptr;
    m_buffer_for_compress_size = 0;
    m_buffer_for_compress_capacity = 0;

    m_make_start_event = nullptr;
    m_make_done_event = nullptr;

    static u8 const sign_random_init[4] = {42, 42, 42, 42};
    m_dump_signer.sign(sign_random_init, sizeof(sign_random_init));
}

configs_dumper::~configs_dumper()
{
    if (m_make_start_event)
    {
        SetEvent(m_make_start_event);
        WaitForSingleObject(m_make_done_event, INFINITE); // thread stoped
        CloseHandle(m_make_done_event);
        CloseHandle(m_make_start_event);
    }
    xr_free(m_buffer_for_compress);
}

void configs_dumper::shedule_Update(u32 dt)
{
    DWORD thread_result = WaitForSingleObject(m_make_done_event, 0);
    R_ASSERT((thread_result != WAIT_ABANDONED) && (thread_result != WAIT_FAILED));
    R_ASSERT(m_state == ds_active);
    if (thread_result == WAIT_OBJECT_0)
    {
        m_complete_cb(m_buffer_for_compress, m_buffer_for_compress_size, m_dump_result.size());
        m_state = ds_not_active;
        Engine.Sheduler.Unregister(this);
    }
}

struct ExistDumpPredicate
{
    shared_str section_name;
    bool operator()(IAnticheatDumpable const* dump_obj) const
    {
        if (!dump_obj)
            return false;
        if (dump_obj->GetAnticheatSectionName() == section_name)
            return true;
        return false;
    }
}; // struct ExistDumpPredicate

typedef buffer_vector<IAnticheatDumpable const*> active_objects_t;
static active_objects_t::size_type get_active_objects(active_objects_t& dest)
{
    CActorMP const* tmp_actor = smart_cast<CActorMP const*>(Level().CurrentControlEntity());

    if (!tmp_actor)
        return 0;

    dest.push_back(tmp_actor);

    for (u16 i = KNIFE_SLOT; i <= GRENADE_SLOT; ++i)
    {
        VERIFY(dest.capacity() != dest.size());
        if (dest.capacity() == dest.size())
            return dest.size();

        CInventoryItem const* tmp_inv_item = tmp_actor->inventory().ItemFromSlot(i);
        if (!tmp_inv_item)
            continue;

        CWeapon const* tmp_weapon = smart_cast<CWeapon const*>(tmp_inv_item);
        if (tmp_weapon)
        {
            dest.push_back(tmp_weapon);
            if (tmp_weapon->m_magazine.size())
            {
                VERIFY(dest.capacity() != dest.size());
                if (dest.capacity() == dest.size())
                    return dest.size();

                IAnticheatDumpable const* tmp_cartridge = &tmp_weapon->m_magazine[0];
                if (!tmp_cartridge)
                    continue;

                ExistDumpPredicate tmp_predicate;
                tmp_predicate.section_name = tmp_cartridge->GetAnticheatSectionName();
                if (std::find_if(dest.begin(), dest.end(), tmp_predicate) == dest.end())
                {
                    dest.push_back(tmp_cartridge);
                };
            }
        }
    }
    return dest.size();
}

static active_objects_t::size_type const max_active_objects = 16;

void configs_dumper::write_configs()
{
    long i = 0;
    m_dump_result.clear();
    m_ltx_configs.start_dump();
    if (m_yield_cb)
    {
        while (m_ltx_configs.dump_one(m_dump_result))
        {
            m_yield_cb(i);
            ++i;
        }
    }
    else
    {
        while (m_ltx_configs.dump_one(m_dump_result))
        {
        };
    }
    CInifile active_params_dumper(NULL, FALSE, FALSE, FALSE);
    active_objects_t active_objects(
        _alloca(sizeof(active_objects_t::value_type) * max_active_objects), max_active_objects);
    active_objects_t::size_type aobjs_count = get_active_objects(active_objects);
    string16 tmp_strbuff;
    for (active_objects_t::size_type i = 0; i < aobjs_count; ++i)
    {
        xr_sprintf(tmp_strbuff, "%d", i + 1);
        m_active_params.dump(active_objects[i], tmp_strbuff, active_params_dumper);
    }
    active_params_dumper.save_as(m_dump_result);
}

char const* cd_info_secion = "config_dump_info";
char const* cd_player_name_key = "player_name";
char const* cd_player_digest_key = "player_digest";
char const* cd_digital_sign_key = "digital_sign";
char const* cd_creation_date = "creation_date";

void configs_dumper::sign_configs()
{
    string64 creation_date;
    LPSTR tmp_player_name = NULL;
    CInifile tmp_ini(NULL, FALSE, FALSE, FALSE);
    game_cl_mp* tmp_cl_game = smart_cast<game_cl_mp*>(&Game());
    R_ASSERT(tmp_cl_game);
    STRCONCAT(tmp_player_name, "\"",
        tmp_cl_game->local_player ? tmp_cl_game->local_player->getName() : "unknown_just_connected", "\"");
    LPCSTR tmp_cdkey_digest = Level().get_cdkey_digest().c_str();
    if (!tmp_cdkey_digest)
        tmp_cdkey_digest = "null";

    LPCSTR add_str = NULL;
    STRCONCAT(add_str, tmp_player_name, tmp_cdkey_digest, current_time(creation_date));

    u32 tmp_w_pos = m_dump_result.tell();
    m_dump_result.w_stringZ(add_str);

    tmp_ini.w_string(cd_info_secion, cd_player_name_key, tmp_player_name);
    tmp_ini.w_string(cd_info_secion, cd_player_digest_key, tmp_cdkey_digest);
    tmp_ini.w_string(cd_info_secion, cd_creation_date, creation_date);

    shared_str tmp_dsign;

    if (m_yield_cb)
    {
        tmp_dsign = m_dump_signer.sign_mt(m_dump_result.pointer(), m_dump_result.size(), m_yield_cb);
    }
    else
    {
        tmp_dsign = m_dump_signer.sign(m_dump_result.pointer(), m_dump_result.size());
    }

    m_dump_result.seek(tmp_w_pos);
    tmp_ini.w_string(cd_info_secion, cd_digital_sign_key, tmp_dsign.c_str());
    tmp_ini.save_as(m_dump_result);
}

void configs_dumper::dump_config(complete_callback_t complete_cb)
{
    if (is_active())
    {
#ifdef DEBUG
        Msg("! ERROR: CL: dump making already in progress...");
#endif
        return;
    }

    ULONG_PTR process_affinity_mask, tmp_dword;
    GetProcessAffinityMask(GetCurrentProcess(), &process_affinity_mask, &tmp_dword);
    bool single_core = (btwCount1(static_cast<u32>(process_affinity_mask)) == 1);
    if (single_core)
    {
        m_yield_cb.bind(this, &configs_dumper::yield_cb);
    }
    else
    {
        m_yield_cb.clear();
    }

    m_complete_cb = complete_cb;
    m_state = ds_active;
    if (m_make_start_event)
    {
        SetEvent(m_make_start_event);
        Engine.Sheduler.Register(this, TRUE);
        return;
    }
    m_make_start_event = CreateEvent(NULL, FALSE, TRUE, NULL);
    m_make_done_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    thread_spawn(&configs_dumper::dumper_thread, "configs_dumper", 0, this);
    Engine.Sheduler.Register(this, TRUE);
}

void configs_dumper::compress_configs()
{
    realloc_compress_buffer(m_dump_result.size());
    ppmd_yield_callback_t ts_cb;
    if (m_yield_cb)
    {
        ts_cb.bind(this, &configs_dumper::switch_thread);
    }
    m_buffer_for_compress_size = ppmd_compress_mt(
        m_buffer_for_compress, m_buffer_for_compress_capacity, m_dump_result.pointer(), m_dump_result.size(), ts_cb);
}

void configs_dumper::dumper_thread(void* my_ptr)
{
    configs_dumper* this_ptr = static_cast<configs_dumper*>(my_ptr);
    DWORD wait_result = WaitForSingleObject(this_ptr->m_make_start_event, INFINITE);
    while ((wait_result != WAIT_ABANDONED) || (wait_result != WAIT_FAILED))
    {
        if (!this_ptr->is_active())
            break; // error
        this_ptr->timer_begin("writing configs");
        this_ptr->write_configs();
        this_ptr->timer_end();
        this_ptr->timer_begin("signing configs");
        this_ptr->sign_configs();
        this_ptr->timer_end();
        this_ptr->timer_begin("compressing data");
        this_ptr->compress_configs();
        this_ptr->timer_end();
        SetEvent(this_ptr->m_make_done_event);
        wait_result = WaitForSingleObject(this_ptr->m_make_start_event, INFINITE);
    }
    SetEvent(this_ptr->m_make_done_event);
}

void __stdcall configs_dumper::yield_cb(long progress)
{
    if (progress % 5 == 0)
    {
        switch_thread();
    }
}

void __stdcall configs_dumper::switch_thread()
{
    if (!SwitchToThread())
        Sleep(10);
}

void configs_dumper::realloc_compress_buffer(u32 need_size)
{
    if (m_buffer_for_compress && (need_size <= m_buffer_for_compress_capacity))
        return;
#ifdef DEBUG
    Msg("* reallocing compression buffer.");
#endif
    m_buffer_for_compress_capacity = need_size * 2;
    void* new_buffer = xr_realloc(m_buffer_for_compress, m_buffer_for_compress_capacity);
    m_buffer_for_compress = static_cast<u8*>(new_buffer);
}

#ifdef DEBUG
void configs_dumper::timer_begin(LPCSTR comment)
{
    m_timer_comment = comment;
    m_debug_timer.Start();
}

void configs_dumper::timer_end() { Msg("* %s : %u ms", m_timer_comment.c_str(), m_debug_timer.GetElapsed_ms()); }
#endif //#ifdef DEBUG

// dump_signer

dump_signer::dump_signer() : xr_dsa_signer(p_number, q_number, g_number) { feel_private_dsa_key(); };
dump_signer::~dump_signer() {}
void dump_signer::feel_private_dsa_key()
{
    // Private key:
    m_private_key.m_value[0] = 0xdb;
    m_private_key.m_value[1] = 0xcb;
    m_private_key.m_value[2] = 0x99;
    m_private_key.m_value[3] = 0x19;
    m_private_key.m_value[4] = 0x2e;
    m_private_key.m_value[5] = 0x42;
    m_private_key.m_value[6] = 0xe7;
    m_private_key.m_value[7] = 0xf9;
    m_private_key.m_value[8] = 0xcb;
    m_private_key.m_value[9] = 0xf2;
    m_private_key.m_value[10] = 0xa3;
    m_private_key.m_value[11] = 0xde;
    m_private_key.m_value[12] = 0xa4;
    m_private_key.m_value[13] = 0xa0;
    m_private_key.m_value[14] = 0xef;
    m_private_key.m_value[15] = 0x14;
    m_private_key.m_value[16] = 0x88;
    m_private_key.m_value[17] = 0x32;
    m_private_key.m_value[18] = 0x91;
    m_private_key.m_value[19] = 0x17;
}

} // namespace mp_anticheat
