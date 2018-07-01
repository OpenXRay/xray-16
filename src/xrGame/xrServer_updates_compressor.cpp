#include "stdafx.h"
#include "Level.h"
#include "xrServer_updates_compressor.h"
#include "xrCore/Compression/ppmd_compressor.h"
#include "Common/object_broker.h"
#include "xrMessages.h"

BOOL g_sv_write_updates_bin = FALSE;

last_updates_cache::last_updates_cache()
{
    for (u32 i = 0; i < cache_entities_size; ++i)
    {
        m_cache[i].first.m_eq_count = 0;
        m_cache[i].first.m_object_id = 0;
        m_cache[i].first.m_update_time = 0;
        m_cache[i].second.B.count = 0;
    }
}

u16 last_updates_cache::add_update(u16 const entity_id, NET_Packet const& update)
{
    last_update_t* tmp_entity = search_entity(entity_id);
    u32 current_time = Device.dwTimeGlobal;
    if (!tmp_entity)
    {
        tmp_entity = search_most_expired(current_time, update.B.count);
        if (!tmp_entity)
        {
            return 0;
        }
    }
    tmp_entity->first.m_object_id = entity_id;
    if ((tmp_entity->second.B.count == update.B.count) &&
        (!memcmp(tmp_entity->second.B.data, update.B.data, update.B.count)))
    {
        ++tmp_entity->first.m_eq_count;
    }
    else
    {
        tmp_entity->first.m_eq_count = 0;
    }
    tmp_entity->first.m_update_time = current_time;
    CopyMemory(tmp_entity->second.B.data, update.B.data, update.B.count);
    tmp_entity->second.B.count = update.B.count;
    return tmp_entity->first.m_eq_count;
}

u16 last_updates_cache::get_last_equpdates(u16 const entity_id, NET_Packet const& update)
{
    last_update_t* tmp_entity = search_entity(entity_id);
    if (!tmp_entity)
        return 0;
    return tmp_entity->first.m_eq_count;
}

last_updates_cache::last_update_t* last_updates_cache::search_entity(u16 const entity_id)
{
    for (u32 i = 0; i < cache_entities_size; ++i)
    {
        if (m_cache[i].first.m_object_id == entity_id)
        {
            return &m_cache[i];
        }
    }
    return NULL;
}

last_updates_cache::last_update_t* last_updates_cache::search_most_expired(
    u32 const current_time, u32 const update_size)
{
    static_assert(cache_entities_size > 1, "Cache entities size must be greater than one.");
    last_update_t* min_time = &m_cache[0];
    for (u32 i = 1; i < cache_entities_size; ++i)
    {
        last_update_t& tmp_entity = m_cache[i];
        u32 curr_time = tmp_entity.first.m_update_time;
        u32 mtime = min_time->first.m_update_time;
        if (curr_time < mtime)
        {
            min_time = &tmp_entity;
        }
        else if ((curr_time == mtime) && (tmp_entity.second.B.count < min_time->second.B.count))
        {
            min_time = &tmp_entity;
        }
    }
    if ((min_time->first.m_update_time == current_time) && (min_time->second.B.count >= update_size))
    {
        return NULL;
    }
    return min_time;
}

server_updates_compressor::server_updates_compressor()
{
    u32 const need_to_reserve = (start_compress_buffer_size / sizeof(m_acc_buff.B.data)) + 1;
    for (u32 i = 0; i < need_to_reserve; ++i)
    {
        m_ready_for_send.push_back(new NET_Packet());
    }

    m_trained_stream = NULL;
    m_lzo_working_memory = NULL;
    m_lzo_working_buffer = NULL;

    if (!IsGameTypeSingle())
        init_compression();

    dbg_update_bins_writer = NULL;
}

server_updates_compressor::~server_updates_compressor()
{
    delete_data(m_ready_for_send);

    if (g_sv_write_updates_bin && dbg_update_bins_writer)
    {
        FS.w_close(dbg_update_bins_writer);
    }
    deinit_compression();
}

void server_updates_compressor::init_compression()
{
    compression::init_ppmd_trained_stream(m_trained_stream);
    compression::init_lzo(m_lzo_working_memory, m_lzo_working_buffer, m_lzo_dictionary);
}

void server_updates_compressor::deinit_compression()
{
    if (m_trained_stream)
    {
        compression::deinit_ppmd_trained_stream(m_trained_stream);
    }
    if (m_lzo_working_buffer)
    {
        VERIFY(m_lzo_dictionary.data);
        compression::deinit_lzo(m_lzo_working_buffer, m_lzo_dictionary);
    }
}

void server_updates_compressor::begin_updates()
{
    m_current_update = 0;
    if ((g_sv_traffic_optimization_level & eto_ppmd_compression) ||
        (g_sv_traffic_optimization_level & eto_lzo_compression))
    {
        m_ready_for_send.front()->w_begin(M_COMPRESSED_UPDATE_OBJECTS);
        m_ready_for_send.front()->w_u8(static_cast<u8>(g_sv_traffic_optimization_level));
        m_acc_buff.write_start();
    }
    else
    {
        m_ready_for_send.front()->write_start();
        m_acc_buff.w_begin(M_UPDATE_OBJECTS);
    }
}

NET_Packet* server_updates_compressor::get_current_dest() { return m_ready_for_send[m_current_update]; }
NET_Packet* server_updates_compressor::goto_next_dest()
{
    ++m_current_update;
    NET_Packet* new_dest = NULL;
    VERIFY(m_ready_for_send.size() >= m_current_update);

    if (m_ready_for_send.size() == m_current_update)
    {
        m_ready_for_send.push_back(new NET_Packet());
        new_dest = m_ready_for_send.back();
    }
    else
    {
        new_dest = m_ready_for_send[m_current_update];
    }

    if (g_sv_traffic_optimization_level & eto_ppmd_compression)
    {
        new_dest->w_begin(M_COMPRESSED_UPDATE_OBJECTS);
        m_ready_for_send.front()->w_u8(static_cast<u8>(g_sv_traffic_optimization_level));
    }
    else
    {
        new_dest->write_start();
    }

    return new_dest;
}

void server_updates_compressor::flush_accumulative_buffer()
{
    NET_Packet* dst_packet = get_current_dest();
    if ((g_sv_traffic_optimization_level & eto_ppmd_compression) ||
        (g_sv_traffic_optimization_level & eto_lzo_compression))
    {
        CompressStats.Begin();
        R_ASSERT(m_trained_stream);
        if (g_sv_traffic_optimization_level & eto_ppmd_compression)
        {
            m_compress_buf.B.count = ppmd_trained_compress(m_compress_buf.B.data, sizeof(m_compress_buf.B.data),
                m_acc_buff.B.data, m_acc_buff.B.count, m_trained_stream);
        }
        else
        {
            m_compress_buf.B.count = sizeof(m_compress_buf.B.data);
            lzo_compress_dict(m_acc_buff.B.data, m_acc_buff.B.count, m_compress_buf.B.data, m_compress_buf.B.count,
                m_lzo_working_memory, m_lzo_dictionary.data, m_lzo_dictionary.size);
        }
        CompressStats.End();
        //(sizeof(u16)*2 + 1) ::= w_begin(2) + compress_type(1) + zero_end(2)
        if (dst_packet->w_tell() + m_compress_buf.B.count + (sizeof(u16) * 2 + 1) < sizeof(dst_packet->B.data))
        {
            dst_packet->w_u16(static_cast<u16>(m_compress_buf.B.count));
            dst_packet->w(m_compress_buf.B.data, m_compress_buf.B.count);
            m_acc_buff.write_start();
            return;
        }
        dst_packet->w_u16(0);
        dst_packet = goto_next_dest();
        dst_packet->w_u16(static_cast<u16>(m_compress_buf.B.count));
        dst_packet->w(m_compress_buf.B.data, m_compress_buf.B.count);
        m_acc_buff.write_start();
        return;
    }
    dst_packet->w(m_acc_buff.B.data, m_acc_buff.B.count);
    goto_next_dest();
    m_acc_buff.w_begin(M_UPDATE_OBJECTS);
}

void server_updates_compressor::write_update_for(u16 const enity, NET_Packet& update)
{
    if (g_sv_traffic_optimization_level & eto_last_change)
    {
        // if (m_updates_cache.get_last_equpdates(enity, update) >= max_eq_packets)
        if (m_updates_cache.add_update(enity, update) >= max_eq_packets)
        {
            return;
        }
    }
    //(sizeof(u16)*2 + 1) ::= w_begin(2) + compress_type(1) + zero_end(2)
    if (m_acc_buff.w_tell() + update.w_tell() + (sizeof(u16) * 2 + 1) >= sizeof(m_acc_buff.B.data))
    {
        flush_accumulative_buffer();
    }
    m_acc_buff.w(update.B.data, update.B.count);
}

void server_updates_compressor::end_updates(
    send_ready_updates_t::const_iterator& b, send_ready_updates_t::const_iterator& e)
{
    if (m_acc_buff.w_tell() > 2)
        flush_accumulative_buffer();

    if ((g_sv_traffic_optimization_level & eto_ppmd_compression) ||
        (g_sv_traffic_optimization_level & eto_lzo_compression))
    {
        get_current_dest()->w_u16(0);
    }

    b = m_ready_for_send.begin();
    e = m_ready_for_send.begin() + m_current_update + 1;

    if (g_sv_write_updates_bin)
    {
        if (!dbg_update_bins_writer)
            create_update_bin_writer();

        VERIFY(dbg_update_bins_writer);
        for (send_ready_updates_t::const_iterator i = b; i != e; ++i)
        {
            dbg_update_bins_writer->w_u16(static_cast<u16>((*i)->B.count));
            dbg_update_bins_writer->w((*i)->B.data, (*i)->B.count);
        }
    }
}

void server_updates_compressor::create_update_bin_writer()
{
    string_path bin_name;
    FS.update_path(bin_name, "$logs$", "updates.bins");
    dbg_update_bins_writer = FS.w_open(bin_name);
    VERIFY(dbg_update_bins_writer);

    static u8 const header[] = {'B', 'I', 'N', 'S'};
    dbg_update_bins_writer->w(header, sizeof(header));
}
