#include "stdafx.h"
#include "filereceiver_node.h"
#include "xrCore/buffer_vector.h"

namespace file_transfer
{
filereceiver_node::filereceiver_node(shared_str const& file_name, receiving_state_callback_t const& callback)
    : m_file_name(file_name), m_data_size_to_receive(0), m_user_param(0),
      m_is_writer_memory(false), m_process_callback(callback), m_last_read_time(0)
{
    m_writer = FS.w_open(file_name.c_str());
}

filereceiver_node::filereceiver_node(CMemoryWriter* mem_writer, receiving_state_callback_t const& callback)
    : m_data_size_to_receive(0), m_user_param(0), m_is_writer_memory(true),
      m_process_callback(callback), m_last_read_time(0)
{
    m_writer = static_cast<IWriter*>(mem_writer);
}

filereceiver_node::~filereceiver_node()
{
    if (m_writer && !m_is_writer_memory)
        FS.w_close(m_writer);
}

bool filereceiver_node::receive_packet(NET_Packet& packet)
{
    if (!m_writer->tell())
    {
        if (packet.r_elapsed() < (sizeof(u32) * 2))
        {
            m_data_size_to_receive = m_writer->tell();
            return false;
        }

        packet.r_u32(m_data_size_to_receive);
        packet.r_u32(m_user_param);
    }
    u32 size_to_write = packet.B.count - packet.r_tell();
    void* pointer = static_cast<void*>(packet.B.data + packet.r_tell());
    m_writer->w(pointer, size_to_write);
    m_last_read_time = Device.dwTimeGlobal;
    return (m_writer->tell() == m_data_size_to_receive);
}

void filereceiver_node::signal_callback(receiving_status_t status)
{
    m_process_callback(status, m_writer->tell(), m_data_size_to_receive);
}

bool filereceiver_node::is_complete()
{
    if (m_writer)
        return (m_writer->tell() == m_data_size_to_receive);
    return false;
}

void split_received_to_buffers(u8* data_ptr, u32 data_size, buffer_vector<const_buffer_t>& dst_buffers)
{
    VERIFY(data_ptr && data_size);
    IReader tmp_reader(data_ptr, data_size);
    while (!tmp_reader.eof())
    {
        u32 const tmp_buffer_size = tmp_reader.r_u32();
        int current_pos = tmp_reader.tell();
        u8 const* tmp_buffer_ptr = static_cast<u8 const*>(tmp_reader.pointer());
        dst_buffers.push_back(std::make_pair(tmp_buffer_ptr, tmp_buffer_size));
        tmp_reader.seek(current_pos + tmp_buffer_size);
    }
}

}; // namespace file_transfer
