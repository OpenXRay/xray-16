#include "StdAfx.h"
#include "filetransfer_node.h"
#include "Level.h"
#include "xrServer.h"
#include "xrCore/buffer_vector.h"

using namespace file_transfer;

// disk reader ..
disk_file_reader::disk_file_reader(shared_str const& file_name) { m_reader = FS.r_open(file_name.c_str()); };
disk_file_reader::~disk_file_reader() { FS.r_close(m_reader); }
bool disk_file_reader::make_data_packet(NET_Packet& packet, u32 chunk_size)
{
    u32 size_to_write = (static_cast<u32>(m_reader->elapsed()) >= chunk_size) ? chunk_size : m_reader->elapsed();

    void* pointer = _alloca(size_to_write);

    R_ASSERT(size_to_write < (NET_PacketSizeLimit - packet.w_tell()));

    m_reader->r(pointer, size_to_write);
    packet.w(pointer, size_to_write);

    return m_reader->eof() ? true : false;
};

bool disk_file_reader::is_first_packet() { return !m_reader->tell(); }
u32 disk_file_reader::size() { return m_reader->length(); }
u32 disk_file_reader::tell() { return m_reader->tell(); }
bool disk_file_reader::opened() const { return (m_reader != NULL); }
// memory reader
memory_reader::memory_reader(u8* data_ptr, u32 data_size)
{
    m_reader = new IReader(static_cast<void*>(data_ptr), static_cast<int>(data_size));
}
memory_reader::~memory_reader() { xr_delete(m_reader); }
bool memory_reader::make_data_packet(NET_Packet& packet, u32 chunk_size)
{
    u32 size_to_write = (static_cast<u32>(m_reader->elapsed()) >= chunk_size) ? chunk_size : m_reader->elapsed();

    void* pointer = _alloca(size_to_write);

    R_ASSERT(size_to_write < (NET_PacketSizeLimit - packet.w_tell()));

    m_reader->r(pointer, size_to_write);
    packet.w(pointer, size_to_write);

    return m_reader->eof() ? true : false;
}

bool memory_reader::is_first_packet() { return !m_reader->tell(); }
u32 memory_reader::size() { return m_reader->length(); }
u32 memory_reader::tell() { return m_reader->tell(); }
bool memory_reader::opened() const { return (m_reader != NULL); }
// buffers_vector reader

buffers_vector_reader::buffers_vector_reader(buffer_vector<mutable_buffer_t>* buffers)
    : m_current_buf_offs(0), m_complete_buffers_size(0), m_sum_size(0)
{
    VERIFY(buffers);
    for (buffer_vector<mutable_buffer_t>::iterator i = buffers->begin(), ie = buffers->end(); i != ie; ++i)
    {
        m_buffers.push_back(*i);
    }
    accumulate_size();
};

buffers_vector_reader::~buffers_vector_reader() {}
void buffers_vector_reader::accumulate_size()
{
    for (buffers_vector_t::iterator i = m_buffers.begin(), ie = m_buffers.end(); i != ie; ++i)
    {
        m_sum_size += i->second;
        m_sum_size += sizeof(u32);
    }
    VERIFY(m_sum_size != 0);
}

void buffers_vector_reader::read_from_current_buf(NET_Packet& dest, u32 read_size)
{
    u32 buffer_size = m_buffers.front().second;

    // each buffer contains its size in header
    if (m_current_buf_offs == 0)
    {
        VERIFY(read_size > sizeof(u32));
        dest.w_u32(buffer_size);
        read_size -= sizeof(u32);
        m_complete_buffers_size += sizeof(u32);
    }

    VERIFY(read_size <= buffer_size);

    dest.w(static_cast<void*>(m_buffers.front().first + m_current_buf_offs), read_size);
    m_current_buf_offs += read_size;

    if (m_current_buf_offs == buffer_size)
    {
        m_buffers.pop_front();
        m_complete_buffers_size += buffer_size;
        m_current_buf_offs = 0;
    }
}

bool buffers_vector_reader::make_data_packet(NET_Packet& packet, u32 chunk_size)
{
    u32 buffer_size = m_buffers.front().second;
    VERIFY(buffer_size > m_current_buf_offs);
    do
    {
        u32 buff_size_header = 0;
        if (m_current_buf_offs == 0)
        {
            buff_size_header = sizeof(u32);
        }
        u32 rest_size = (buffer_size - m_current_buf_offs) + buff_size_header;
        if ((chunk_size <= rest_size) && (chunk_size > buff_size_header))
        {
            read_from_current_buf(packet, chunk_size);
            break;
        }
        else if (chunk_size <= buff_size_header)
        {
#ifdef DEBUG
            Msg("! Not enough chunk, writing is deferred ...");
#endif
            break;
        }
        read_from_current_buf(packet, rest_size);
        chunk_size -= rest_size;
    } while (chunk_size == 0);

    return (size() == tell());
}

bool buffers_vector_reader::is_first_packet()
{
    VERIFY(opened());
    return (m_current_buf_offs == 0) && (m_complete_buffers_size == 0);
}
u32 buffers_vector_reader::size() { return m_sum_size; }
u32 buffers_vector_reader::tell() { return m_complete_buffers_size + m_current_buf_offs; }
bool buffers_vector_reader::opened() const { return !m_buffers.empty(); }
// memory_writer reader

memory_writer_reader::memory_writer_reader(CMemoryWriter* src_writer, u32 const max_size)
    : m_writer_as_src(src_writer), m_writer_pointer(0), m_writer_max_size(max_size)
{
}

memory_writer_reader::~memory_writer_reader() {}
bool memory_writer_reader::make_data_packet(NET_Packet& packet, u32 chunk_size)
{
    u32 elapsed = m_writer_as_src->size() - m_writer_pointer;
    // in case of very fast client (can be server client),
    // server can send all data stored in memory writer, while new data is not received
    // ready_to_send method will return true because tell() < size().
    // and on transfer updates elapsed will be 0
    // to avoid this situation we make return ...
    if (!elapsed)
        return (m_writer_pointer == m_writer_max_size);

    u32 size_to_write = (elapsed >= chunk_size) ? chunk_size : elapsed;
    void* pointer = m_writer_as_src->pointer() + m_writer_pointer;

    R_ASSERT(size_to_write < (NET_PacketSizeLimit - packet.w_tell()));

    m_writer_pointer += size_to_write;
    packet.w(pointer, size_to_write);
    return (m_writer_pointer == m_writer_max_size);
}

bool memory_writer_reader::is_first_packet() { return !m_writer_pointer; }
u32 memory_writer_reader::size() { return m_writer_max_size; }
u32 memory_writer_reader::tell() { return m_writer_pointer; }
bool memory_writer_reader::opened() const { return (m_writer_as_src != NULL); }
filetransfer_node::filetransfer_node(
    shared_str const& file_name, u32 const chunk_size, sending_state_callback_t const& callback)
    : m_chunk_size(chunk_size), m_last_peak_throughput(0), m_last_chunksize_update_time(0), m_user_param(0),
      m_process_callback(callback)
{
    m_reader = new disk_file_reader(file_name);
}

filetransfer_node::filetransfer_node(
    u8* data, u32 const data_size, u32 const chunk_size, sending_state_callback_t const& callback, u32 user_param)
    : m_chunk_size(chunk_size), m_last_peak_throughput(0), m_last_chunksize_update_time(0), m_user_param(user_param),
      m_process_callback(callback)
{
    m_reader = new memory_reader(data, data_size);
}

filetransfer_node::filetransfer_node(CMemoryWriter* src_writer, u32 const max_size, u32 const chunk_size,
    sending_state_callback_t const& callback, u32 user_param)
    : m_chunk_size(chunk_size), m_last_peak_throughput(0), m_last_chunksize_update_time(0), m_user_param(user_param),
      m_process_callback(callback)
{
    m_reader = new memory_writer_reader(src_writer, max_size);
}

filetransfer_node::filetransfer_node(buffer_vector<mutable_buffer_t>* vector_of_buffers, u32 const chunk_size,
    sending_state_callback_t const& callback, u32 user_param)
    : m_chunk_size(chunk_size), m_last_peak_throughput(0), m_last_chunksize_update_time(0), m_user_param(user_param),
      m_process_callback(callback)
{
    VERIFY(vector_of_buffers);
    m_reader = new buffers_vector_reader(vector_of_buffers);
}

filetransfer_node::~filetransfer_node() { xr_delete(m_reader); }
void filetransfer_node::calculate_chunk_size(u32 peak_throughput, u32 current_throughput)
{
    if ((Device.dwTimeGlobal - m_last_chunksize_update_time) < 1000)
        return;

    if (m_last_peak_throughput < peak_throughput) // peak throughput is increasing, so we can increase upload size :)
    {
        m_chunk_size += data_min_chunk_size;
#ifdef MP_LOGGING
        Msg("* peak throughout is not reached - increasing upload rate : (m_chunk_size: %d)", m_chunk_size);
#endif
    }
    else // peak is reached
    {
        if (OnServer())
        {
            m_chunk_size = data_max_chunk_size;
            return;
        }
        if ((Device.dwTimeGlobal - m_last_chunksize_update_time) < 3000)
            return;

        m_chunk_size = static_cast<u32>(Random.randI(data_min_chunk_size, data_max_chunk_size));
#ifdef MP_LOGGING
        Msg("* peak throughout is reached, (current_throughput: %d), (peak_throughput: %d), (m_chunk_size: %d)",
            current_throughput, peak_throughput, m_chunk_size);
#endif
    }
    clamp(m_chunk_size, data_min_chunk_size, data_max_chunk_size);
    m_last_peak_throughput = peak_throughput;
    m_last_chunksize_update_time = Device.dwTimeGlobal;
}

bool filetransfer_node::make_data_packet(NET_Packet& packet)
{
    if (m_reader->is_first_packet())
    {
        packet.w_u32(m_reader->size());
        packet.w_u32(m_user_param);
    }
    return m_reader->make_data_packet(packet, m_chunk_size);
}

void filetransfer_node::signal_callback(sending_status_t status)
{
    m_process_callback(status, m_reader->tell(), m_reader->size());
}

bool filetransfer_node::is_complete()
{
    if (opened())
        return (m_reader->tell() == m_reader->size());

    return false;
}

bool filetransfer_node::is_ready_to_send()
{
    VERIFY(opened());
    return !is_complete();
}

bool filetransfer_node::opened() const
{
    VERIFY(m_reader);
    return m_reader->opened();
}
