#ifndef XRSERVER_UPDATES_COMPRESSOR_INCLUDED
#define XRSERVER_UPDATES_COMPRESSOR_INCLUDED

#include "traffic_optimization.h"

extern Flags8 g_sv_traffic_optimization_level;
extern Flags8 g_sv_available_traffic_optimization_level;

class last_updates_cache : private Noncopyable
{
public:
    static u32 const cache_entities_size = 32;
    struct entity_update_key
    {
        u16 m_object_id;
        u16 m_eq_count;
        u32 m_update_time;
    };
    typedef std::pair<entity_update_key, NET_Packet> last_update_t;
    typedef last_update_t last_updates_cache_t[cache_entities_size];

    last_updates_cache();
    ~last_updates_cache(){};

    u16 add_update(u16 const entity_id, NET_Packet const& update);
    u16 get_last_equpdates(u16 const entity_id, NET_Packet const& update);

private:
    last_update_t* search_entity(u16 const entity_id);
    last_update_t* search_most_expired(u32 const current_time, u32 const update_size);

    last_updates_cache_t m_cache;

}; // class last_updates_cache

class server_updates_compressor
{
public:
    CStatTimer CompressStats;

    server_updates_compressor();
    ~server_updates_compressor();

    typedef xr_vector<NET_Packet*> send_ready_updates_t;

    void begin_updates();
    void write_update_for(u16 const enity, NET_Packet& update);
    void end_updates(send_ready_updates_t::const_iterator& b, send_ready_updates_t::const_iterator& e);

private:
    // actor update size ~ 150 bytes..
    static u16 const max_eq_packets = 3;
    static u32 const entities_count = 32;
    static u32 const start_compress_buffer_size = 1024 * 150 * entities_count;

    NET_Packet m_acc_buff;
    NET_Packet m_compress_buf;

    last_updates_cache m_updates_cache;

    send_ready_updates_t m_ready_for_send;
    send_ready_updates_t::size_type m_current_update;

    compression::ppmd_trained_stream* m_trained_stream;
    compression::lzo_dictionary_buffer m_lzo_dictionary;
    // alligned to 16 bytes m_lzo_working_buffer
    u8* m_lzo_working_memory;
    u8* m_lzo_working_buffer;

    void init_compression();
    void deinit_compression();

    void flush_accumulative_buffer();
    NET_Packet* get_current_dest();
    NET_Packet* goto_next_dest();

    IWriter* dbg_update_bins_writer;
    void create_update_bin_writer();
}; // class server_updates_compressor

#endif //#ifndef XRSERVER_UPDATES_COMPRESSOR_INCLUDED
