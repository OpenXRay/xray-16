#include "StdAfx.h"
#include "xrMessages.h"
#include "file_transfer.h"
#include "Level.h"
#include "xrServer.h"
#include "xrUICore/ui_base.h"
#include "xrNetServer/NET_Messages.h"
#include "xrCore/buffer_vector.h"
#ifdef DEBUG
#include "xrEngine/GameFont.h"
#endif

#define MAX_FT_WAIT_TIME (2000 * 3) /*3 max pings*/
#define MAX_START_WAIT_TIME (2000 * 14) /*10 max pings*/

namespace file_transfer
{
void make_reject_packet(NET_Packet& packet, ClientID const& client)
{
    packet.w_begin(M_FILE_TRANSFER);
    packet.w_u8(static_cast<u8>(receive_rejected));
    packet.w_u32(client.value());
}
void make_abort_packet(NET_Packet& packet, ClientID const& client)
{
    packet.w_begin(M_FILE_TRANSFER);
    packet.w_u8(static_cast<u8>(abort_receive));
    packet.w_u32(client.value());
}

server_site::server_site() {}
server_site::~server_site()
{
    transfer_sessions_t::iterator ti = m_transfers.begin();
    while (ti != m_transfers.end())
    {
        stop_transfer_file(ti->first);
        ti = m_transfers.begin();
    }
    receiving_sessions_t::iterator ri = m_receivers.begin();
    while (ri != m_receivers.end())
    {
        stop_receive_file(ri->first);
        ri = m_receivers.begin();
    }
}

void server_site::stop_transfer_sessions(
    buffer_vector<dst_src_pair_t> const& tsessions) // notifies sending_rejected_by_peer
{
    for (buffer_vector<dst_src_pair_t>::const_iterator i = tsessions.begin(), ie = tsessions.end(); i != ie; ++i)
    {
        stop_transfer_file(*i);
    }
}

void server_site::stop_receiving_sessions(buffer_vector<ClientID> const& rsessions) // notifies receiving_timeout
{
    for (buffer_vector<ClientID>::const_iterator i = rsessions.begin(), ie = rsessions.end(); i != ie; ++i)
    {
        stop_receive_file(*i);
    }
}

void server_site::update_transfer()
{
    if (m_transfers.empty())
        return;

    buffer_vector<dst_src_pair_t> to_stop_transfers(
        _alloca(m_transfers.size() * sizeof(dst_src_pair_t)), m_transfers.size());

    for (transfer_sessions_t::iterator ti = m_transfers.begin(), tie = m_transfers.end(); ti != tie; ++ti)
    {
        IClient* tmp_client = Level().Server->GetClientByID(ti->first.first); // dst
        if (!tmp_client)
        {
            Msg("! ERROR: SV: client [%u] not found for transfering file", ti->first);
            to_stop_transfers.push_back(ti->first);
            ti->second->signal_callback(sending_rejected_by_peer);
            continue;
        }
        filetransfer_node* tmp_ftnode = ti->second;
        if (!tmp_ftnode->is_ready_to_send())
        {
            continue;
        }

        tmp_ftnode->calculate_chunk_size(tmp_client->stats.getPeakBPS(), tmp_client->stats.getBPS());
        NET_Packet tmp_packet;
        tmp_packet.w_begin(M_FILE_TRANSFER);
        tmp_packet.w_u8(receive_data);
        tmp_packet.w_u32(ti->first.second.value()); // src
        bool complete = tmp_ftnode->make_data_packet(tmp_packet);
        Level().Server->SendTo(tmp_client->ID, tmp_packet, net_flags(TRUE, TRUE, TRUE));
        if (complete)
        {
            tmp_ftnode->signal_callback(sending_complete);
            to_stop_transfers.push_back(ti->first);
        }
        else
        {
            tmp_ftnode->signal_callback(sending_data);
        }
    }
    stop_transfer_sessions(to_stop_transfers);
}

void server_site::on_message(NET_Packet* packet, ClientID const& sender)
{
    ft_command_t command = static_cast<ft_command_t>(packet->r_u8());
    switch (command)
    {
    case receive_data:
    {
        receiving_sessions_t::iterator temp_iter = m_receivers.find(sender);
        if (temp_iter == m_receivers.end())
        {
            NET_Packet reject_packet;
            make_reject_packet(reject_packet, ClientID(0));
            Level().Server->SendTo(sender, reject_packet, net_flags(TRUE, TRUE, TRUE));
            break;
        }
        if (temp_iter->second->receive_packet(*packet))
        {
            temp_iter->second->signal_callback(receiving_complete);
            stop_receive_file(temp_iter->first);
        }
        else
        {
            temp_iter->second->signal_callback(receiving_data);
        }
    }
    break;
    case abort_receive:
    {
        // ignoring ClientID(0) source client
        receiving_sessions_t::iterator temp_iter = m_receivers.find(sender);
        if (temp_iter != m_receivers.end())
        {
            temp_iter->second->signal_callback(receiving_aborted_by_peer);
            stop_receive_file(sender);
        }
    }
    break;
    case receive_rejected:
    {
        transfer_sessions_t::iterator temp_iter = m_transfers.find(std::make_pair(sender, ClientID(packet->r_u32())));
        if (temp_iter != m_transfers.end())
        {
            temp_iter->second->signal_callback(sending_rejected_by_peer);
            stop_transfer_file(temp_iter->first);
        }
    }
    break;
    };
}

void server_site::stop_obsolete_receivers()
{
    u32 current_time = Device.dwTimeGlobal;
    buffer_vector<ClientID> to_stop_receivers(_alloca(m_receivers.size() * sizeof(ClientID)), m_receivers.size());

    for (receiving_sessions_t::iterator i = m_receivers.begin(), ie = m_receivers.end(); i != ie; ++i)
    {
        if (!i->second->get_downloaded_size())
        {
            if (!i->second->get_last_read_time())
            {
                i->second->set_last_read_time(current_time);
            }
            else if ((current_time - i->second->get_last_read_time()) > MAX_START_WAIT_TIME)
            {
                i->second->signal_callback(receiving_timeout);
                to_stop_receivers.push_back(i->first);
            }
        }
        else
        {
            if ((current_time - i->second->get_last_read_time()) > MAX_FT_WAIT_TIME)
            {
                i->second->signal_callback(receiving_timeout);
                to_stop_receivers.push_back(i->first);
            }
        }
    }
    stop_receiving_sessions(to_stop_receivers);
}

void server_site::start_transfer_file(shared_str const& file_name, ClientID const& to_client,
    ClientID const& from_client, sending_state_callback_t& tstate_callback)
{
    if (is_transfer_active(to_client, from_client))
    {
        Msg("! ERROR: SV: transfering file to client [%d] already active.", to_client);
        return;
    }
    filetransfer_node* ftnode = new filetransfer_node(file_name, data_max_chunk_size, tstate_callback);
    dst_src_pair_t tkey = std::make_pair(to_client, from_client);
    m_transfers.insert(std::make_pair(tkey, ftnode));
    if (!ftnode->opened())
    {
        Msg("! ERROR: SV: failed to open file [%s]", file_name.c_str());
        stop_transfer_file(tkey);
    }
}

void server_site::start_transfer_file(CMemoryWriter& mem_writer, u32 mem_writer_max_size, ClientID const& to_client,
    ClientID const& from_client, sending_state_callback_t& tstate_callback, u32 const user_param)
{
    if (is_transfer_active(to_client, from_client))
    {
        Msg("! ERROR: SV: transfering file to client [%d] already active.", to_client);
        return;
    }
    filetransfer_node* ftnode =
        new filetransfer_node(&mem_writer, mem_writer_max_size, data_max_chunk_size, tstate_callback, user_param);
    m_transfers.insert(std::make_pair(std::make_pair(to_client, from_client), ftnode));
}

void server_site::start_transfer_file(u8* data_ptr, u32 const data_size, ClientID const& to_client,
    ClientID const& from_client, sending_state_callback_t& tstate_callback, u32 const user_param)
{
    if (is_transfer_active(to_client, from_client))
    {
        Msg("! ERROR: SV: transfering file to client [%d] already active.", to_client);
        return;
    }
    filetransfer_node* ftnode =
        new filetransfer_node(data_ptr, data_size, data_max_chunk_size, tstate_callback, user_param);
    m_transfers.insert(std::make_pair(std::make_pair(to_client, from_client), ftnode));
}
void server_site::start_transfer_file(buffer_vector<mutable_buffer_t>& vector_of_buffers, ClientID const& to_client,
    ClientID const& from_client, sending_state_callback_t& tstate_callback, u32 const user_param)
{
    if (is_transfer_active(to_client, from_client))
    {
        Msg("! ERROR: SV: transfering file to client [%d] already active.", to_client);
        return;
    }
    filetransfer_node* ftnode =
        new filetransfer_node(&vector_of_buffers, data_max_chunk_size, tstate_callback, user_param);
    m_transfers.insert(std::make_pair(std::make_pair(to_client, from_client), ftnode));
}

void server_site::stop_transfer_file(dst_src_pair_t const& tkey)
{
    transfer_sessions_t::iterator temp_iter = m_transfers.find(tkey);
    if (temp_iter == m_transfers.end())
    {
        Msg("! ERROR: SV: no file transfer for client [%d] found from client [%d].", tkey.first, tkey.second);
        return;
    }
    if (!temp_iter->second->is_complete())
    {
        NET_Packet abort_packet;
        make_abort_packet(abort_packet, tkey.second);
        if (Level().Server->GetClientByID(tkey.first))
        {
            Level().Server->SendTo(tkey.first, abort_packet, net_flags(TRUE, TRUE, TRUE));
        }
    }
    xr_delete(temp_iter->second);
    m_transfers.erase(temp_iter);
}

filereceiver_node* server_site::start_receive_file(
    shared_str const& file_name, ClientID const& from_client, receiving_state_callback_t& rstate_callback)
{
    receiving_sessions_t::iterator temp_iter = m_receivers.find(from_client);
    if (temp_iter != m_receivers.end())
    {
        Msg("! ERROR: SV: file already receiving from client [%d]", from_client);
        return NULL;
    }
    filereceiver_node* frnode = new filereceiver_node(file_name, rstate_callback);
    m_receivers.insert(std::make_pair(from_client, frnode));
    if (!frnode->get_writer())
    {
        Msg("! ERROR: SV: failed to create file [%s]", file_name.c_str());
        stop_receive_file(from_client);
        return NULL;
    }
    return frnode;
}

filereceiver_node* server_site::start_receive_file(
    CMemoryWriter& mem_writer, ClientID const& from_client, receiving_state_callback_t& rstate_callback)
{
    receiving_sessions_t::iterator temp_iter = m_receivers.find(from_client);
    if (temp_iter != m_receivers.end())
    {
        Msg("! ERROR: SV: file already receiving from client [%d]", from_client);
        return NULL;
    }
    filereceiver_node* frnode = new filereceiver_node(&mem_writer, rstate_callback);
    m_receivers.insert(std::make_pair(from_client, frnode));
    return frnode;
}

void server_site::stop_receive_file(ClientID const& from_client)
{
    receiving_sessions_t::iterator temp_iter = m_receivers.find(from_client);
    if (temp_iter == m_receivers.end())
    {
        Msg("! ERROR: SV: no file receiving from client [%u] found", from_client);
        return;
    }
    if (!temp_iter->second->is_complete())
    {
        NET_Packet reject_packet;
        make_reject_packet(reject_packet, ClientID(0));
        Level().Server->SendTo(from_client, reject_packet, net_flags(TRUE, TRUE, TRUE));
    }
    xr_delete(temp_iter->second);
    m_receivers.erase(temp_iter);
}

bool server_site::is_transfer_active(ClientID const& to_client, ClientID const& from_client) const
{
    transfer_sessions_t::const_iterator temp_iter = m_transfers.find(std::make_pair(to_client, from_client));
    if (temp_iter == m_transfers.end())
        return false;
    return true;
}
bool server_site::is_receiving_active(ClientID const& from_client) const
{
    receiving_sessions_t::const_iterator temp_iter = m_receivers.find(from_client);
    if (temp_iter == m_receivers.end())
        return false;
    return true;
}

client_site::client_site() : m_transfering(NULL)
{
#ifdef DEBUG
    m_stat_graph = NULL;
#endif
}

client_site::~client_site()
{
    stop_transfer_file();
    receiving_sessions_t::iterator ri = m_receivers.begin();
    while (ri != m_receivers.end())
    {
        stop_receive_file(ri->first);
        ri = m_receivers.begin();
    }
#ifdef DEBUG
    dbg_deinit_statgraph();
#endif
}

void client_site::update_transfer()
{
    if (is_transfer_active() && m_transfering->is_ready_to_send())
    {
        IClientStatistic& peer_stats = Level().GetStatistic();
        m_transfering->calculate_chunk_size(peer_stats.getPeakBPS(), peer_stats.getBPS());
#ifdef DEBUG
        if (psDeviceFlags.test(rsStatistic))
        {
            dbg_update_statgraph();
        }
        else
        {
            dbg_deinit_statgraph();
        }
#endif
        NET_Packet tmp_packet;
        tmp_packet.w_begin(M_FILE_TRANSFER);
        tmp_packet.w_u8(receive_data);
        bool complete = m_transfering->make_data_packet(tmp_packet);
        Level().Send(tmp_packet, net_flags(TRUE, TRUE, TRUE));
        if (complete)
        {
            m_transfering->signal_callback(sending_complete);
            stop_transfer_file();
        }
        else
        {
            m_transfering->signal_callback(sending_data);
        }
    }
}
void client_site::on_message(NET_Packet* packet)
{
    ft_command_t command = static_cast<ft_command_t>(packet->r_u8());
    ClientID from_client(packet->r_u32());
    switch (command)
    {
    case receive_data:
    {
        receiving_sessions_t::iterator tmp_iter = m_receivers.find(from_client);
        if (tmp_iter != m_receivers.end())
        {
            if (tmp_iter->second->receive_packet(*packet))
            {
                tmp_iter->second->signal_callback(receiving_complete);
                stop_receive_file(from_client);
            }
            else
                tmp_iter->second->signal_callback(receiving_data);
        }
        else
        {
            NET_Packet reject_packet;
            make_reject_packet(reject_packet, from_client);
            Level().Send(reject_packet, net_flags(TRUE, TRUE, TRUE));
        }
    }
    break;
    case abort_receive:
    {
        receiving_sessions_t::iterator tmp_iter = m_receivers.find(from_client);
        if (tmp_iter != m_receivers.end())
        {
            tmp_iter->second->signal_callback(receiving_aborted_by_peer);
            stop_receive_file(from_client);
        }
        else
        {
            Msg("! WARNING: CL: server sent unknown abort receive message");
        }
    }
    break;
    case receive_rejected:
    {
        // ignoring from_client u32
        if (is_transfer_active())
        {
            m_transfering->signal_callback(sending_rejected_by_peer);
            stop_transfer_file();
        }
        else
        {
            Msg("! WARNING: CL: server sent unknown receive reject message");
        }
    }
    break;
    };
}

void client_site::start_transfer_file(shared_str const& file_name, sending_state_callback_t& tstate_callback)
{
    if (is_transfer_active())
    {
        Msg("! ERROR: CL: transfering file already active.");
        return;
    }
    m_transfering = new filetransfer_node(file_name, data_min_chunk_size, tstate_callback);
    if (!m_transfering->opened())
    {
        Msg("! ERROR: CL: failed to open file [%s]", file_name.c_str());
        stop_transfer_file();
    }
}
void client_site::start_transfer_file(
    u8* data, u32 size, sending_state_callback_t& tstate_callback, u32 size_to_allocate)
{
    if (is_transfer_active())
    {
        Msg("! ERROR: CL: transfering file already active.");
        return;
    }
    if (!size || !data)
    {
        Msg("! ERROR: CL: no data to transfer ...");
        return;
    }
    m_transfering = new filetransfer_node(data, size, data_min_chunk_size, tstate_callback, size_to_allocate);
}

void client_site::stop_transfer_file()
{
    if (!is_transfer_active())
        return;

    if (!m_transfering->is_complete())
    {
        NET_Packet abort_packet;
        make_abort_packet(abort_packet, ClientID(0));
        Level().Send(abort_packet, net_flags(TRUE, TRUE, TRUE));
    }
    xr_delete(m_transfering);
}

filereceiver_node* client_site::start_receive_file(
    shared_str const& file_name, ClientID const& from_client, receiving_state_callback_t& rstate_callback)
{
    if (is_receiving_active(from_client))
    {
        Msg("! ERROR: CL: file already receiving from client [%d]", from_client);
        return NULL;
    }
    filereceiver_node* frnode = new filereceiver_node(file_name, rstate_callback);
    m_receivers.insert(std::make_pair(from_client, frnode));
    if (!frnode->get_writer())
    {
        Msg("! ERROR: CL: failed to create file [%s]", file_name.c_str());
        stop_receive_file(from_client);
        return NULL;
    }
    return frnode;
}

filereceiver_node* client_site::start_receive_file(
    CMemoryWriter& mem_writer, ClientID const& from_client, receiving_state_callback_t& rstate_callback)
{
    if (is_receiving_active(from_client))
    {
        Msg("! ERROR: CL: file already receiving from client [%d]", from_client);
        return NULL;
    }
    mem_writer.clear();
    filereceiver_node* frnode = new filereceiver_node(&mem_writer, rstate_callback);
    m_receivers.insert(std::make_pair(from_client, frnode));
    return frnode;
}

void client_site::stop_receive_file(ClientID const& from_client)
{
    receiving_sessions_t::iterator temp_iter = m_receivers.find(from_client);
    if (temp_iter == m_receivers.end())
    {
        Msg("! ERROR: CL: no file receiving from client [%u] found", from_client);
        return;
    }
    if (!temp_iter->second->is_complete())
    {
        NET_Packet reject_packet;
        make_reject_packet(reject_packet, from_client);
        Level().Send(reject_packet, net_flags(TRUE, TRUE, TRUE));
    }
    xr_delete(temp_iter->second);
    m_receivers.erase(temp_iter);
}

void client_site::stop_receiving_sessions(buffer_vector<ClientID> const& rsessions)
{
    for (buffer_vector<ClientID>::const_iterator i = rsessions.begin(), ie = rsessions.end(); i != ie; ++i)
    {
        stop_receive_file(*i);
    }
}

void client_site::stop_obsolete_receivers()
{
    u32 current_time = Device.dwTimeGlobal;
    buffer_vector<ClientID> to_stop_receivers(_alloca(m_receivers.size() * sizeof(ClientID)), m_receivers.size());

    for (receiving_sessions_t::iterator i = m_receivers.begin(), ie = m_receivers.end(); i != ie; ++i)
    {
        if (!i->second->get_downloaded_size())
        {
            if (!i->second->get_last_read_time())
            {
                i->second->set_last_read_time(current_time);
            }
            else if ((current_time - i->second->get_last_read_time()) > MAX_START_WAIT_TIME)
            {
                i->second->signal_callback(receiving_timeout);
                to_stop_receivers.push_back(i->first);
            }
        }
        else
        {
            if ((current_time - i->second->get_last_read_time()) > MAX_FT_WAIT_TIME)
            {
                i->second->signal_callback(receiving_timeout);
                to_stop_receivers.push_back(i->first);
            }
        }
    }
    stop_receiving_sessions(to_stop_receivers);
}

#ifdef DEBUG
void client_site::dbg_init_statgraph()
{
    CGameFont* F = UI().Font().pFontDI;
    F->SetHeightI(0.015f);
    F->OutSet(360.f, 700.f);
    F->SetColor(color_xrgb(0, 255, 0));
    F->OutNext("%d", (int)data_max_chunk_size);
    F->OutSet(360.f, 760.f);
    F->OutNext("%d", (int)data_min_chunk_size);
    m_stat_graph = new CStatGraph();
    m_stat_graph->SetRect(400, 700, 200, 68, 0xff000000, 0xff000000);
    m_stat_graph->SetMinMax(float(data_min_chunk_size), float(data_max_chunk_size), 1000);
    m_stat_graph->SetStyle(CStatGraph::stBarLine);
    m_stat_graph->AppendSubGraph(CStatGraph::stBarLine);
}
void client_site::dbg_deinit_statgraph()
{
    if (m_stat_graph)
    {
        xr_delete(m_stat_graph);
    }
}

void client_site::dbg_update_statgraph()
{
    if (!m_stat_graph)
    {
        dbg_init_statgraph();
    }
    if (m_transfering)
    {
        m_stat_graph->AppendItem(float(m_transfering->get_chunk_size()), 0xff00ff00, 0);
    }
}
#endif

} // namespace file_transfer
