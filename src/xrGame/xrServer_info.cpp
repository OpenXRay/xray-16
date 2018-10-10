#include "StdAfx.h"
#include "xrServer_info.h"
#include "Level.h"
#include "xrServer.h"
#include "xrNetServer/NET_Messages.h"
#include "xrCore/buffer_vector.h"

#define SERVER_LOGO_FN "server_logo.jpg"
#define SERVER_RULES_FN "server_rules.txt"

server_info_uploader& xrServer::GetServerInfoUploader()
{
    VERIFY(m_server_logo && m_server_rules);

    struct free_info_searcher
    {
        bool operator()(server_info_uploader const* uplinfo) { return !uplinfo->is_active(); };
    }; // struct free_info_searcher

    info_uploaders_t::iterator tmp_iter =
        std::find_if(m_info_uploaders.begin(), m_info_uploaders.end(), free_info_searcher());

    server_info_uploader* result = NULL;
    if (tmp_iter != m_info_uploaders.end())
    {
        result = *tmp_iter;
    }
    else
    {
        result = new server_info_uploader(m_file_transfers);
        m_info_uploaders.push_back(result);
    }
    return *result;
}

void xrServer::SendServerInfoToClient(ClientID const& new_client) // WARNING ! this function is thread unsafe !!!
{
    if (IsGameTypeSingle())
    {
        SendConfigFinished(new_client);
        return;
    }

    if (!m_server_logo || !m_server_rules)
    {
        SendConfigFinished(new_client);
        return;
    }

    NET_Packet sinfo_packet;

    sinfo_packet.w_begin(M_GAMEMESSAGE);
    sinfo_packet.w_u32(GAME_EVENT_RECEIVE_SERVER_LOGO);
    sinfo_packet.w_u32(GetServerClient()->ID.value());

    SendTo(new_client, sinfo_packet, net_flags(TRUE, TRUE));

    svinfo_upload_complete_cb upload_compl_cb(this, &xrServer::SendConfigFinished);
    server_info_uploader& tmp_uploader = GetServerInfoUploader();
    tmp_uploader.start_upload_info(m_server_logo, m_server_rules, new_client, upload_compl_cb);
}

void xrServer::LoadServerInfo()
{
    if (!FS.exist("$app_data_root$", SERVER_LOGO_FN) || !FS.exist("$app_data_root$", SERVER_RULES_FN))
    {
        return;
    }
    m_server_logo = FS.r_open("$app_data_root$", SERVER_LOGO_FN);
    if (!m_server_logo)
    {
        Msg("! ERROR: failed to open server logo file %s", SERVER_LOGO_FN);
        return;
    }
    m_server_rules = FS.r_open("$app_data_root$", SERVER_RULES_FN);
    if (!m_server_rules)
    {
        Msg("! ERROR: failed to open server rules file %s", SERVER_RULES_FN);
        FS.r_close(m_server_logo);
        m_server_logo = NULL;
        return;
    }
}

server_info_uploader::server_info_uploader(file_transfer::server_site* file_transfers)
    : m_state(eUploadNotActive), m_logo_data(NULL), m_logo_size(0), m_rules_data(NULL), m_rules_size(0),
      m_file_transfers(file_transfers)
{
    R_ASSERT(Level().Server && Level().Server->GetServerClient());
    m_from_client = Level().Server->GetServerClient()->ID;
}

server_info_uploader::~server_info_uploader()
{
    R_ASSERT(m_file_transfers != NULL);
    if (is_active())
        terminate_upload();
}

void server_info_uploader::terminate_upload()
{
    R_ASSERT(is_active());
    m_file_transfers->stop_transfer_file(std::make_pair(m_to_client, m_from_client));
    m_state = eUploadNotActive;
    execute_complete_cb();
}

void server_info_uploader::start_upload_info(IReader const* svlogo, IReader const* svrules, ClientID const& toclient,
    svinfo_upload_complete_cb const& complete_cb)
{
    using namespace file_transfer;
    sending_state_callback_t sndcb;
    sndcb.bind(this, &server_info_uploader::upload_server_info_callback);

    buffer_vector<mutable_buffer_t> tmp_bufvec(_alloca(sizeof(mutable_buffer_t) * 2), 2);

    tmp_bufvec.push_back(std::make_pair(static_cast<u8*>(svlogo->pointer()), svlogo->length()));

    tmp_bufvec.push_back(std::make_pair(static_cast<u8*>(svrules->pointer()), svrules->length()));

    m_to_client = toclient;

    m_file_transfers->start_transfer_file(tmp_bufvec, m_to_client, m_from_client, sndcb, 0);
    m_state = eUploadingInfo;
    m_complete_cb = complete_cb;
}

void server_info_uploader::execute_complete_cb()
{
    R_ASSERT(m_complete_cb);
    m_complete_cb(m_to_client);
    m_complete_cb.clear();
}

void __stdcall server_info_uploader::upload_server_info_callback(
    file_transfer::sending_status_t status, u32 uploaded, u32 total)
{
    switch (status)
    {
    case file_transfer::sending_data: {
#ifdef DEBUG
        Msg("* uploaded %d from %d bytes of server logo to client [%d]", uploaded, total, m_to_client.value());
#endif
        return;
    }
    break;
    case file_transfer::sending_aborted_by_user: { FATAL("* upload server logo terminated by user ");
    }
    break;
    case file_transfer::sending_rejected_by_peer:
    {
        Msg("* upload server logo terminated by peer [%d]", m_to_client.value());
    }
    break;
    case file_transfer::sending_complete: { Msg("* upload server info to client [%d] complete !", m_to_client.value());
    }
    break;
    };
    m_state = eUploadNotActive;
    execute_complete_cb();
}
