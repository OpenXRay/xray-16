#ifndef FILETRANSFER
#define FILETRANSFER

#include "associative_vector.h"
#include "..\xrEngine\StatGraph.h"
#include "filetransfer_node.h"
#include "filereceiver_node.h"


//this module is an implementation of file transfering ...
namespace file_transfer
{




class server_site
{
public:
	typedef std::pair<ClientID, ClientID> dst_src_pair_t;
private:
	typedef associative_vector<dst_src_pair_t, filetransfer_node*> transfer_sessions_t;
	typedef associative_vector<ClientID, filereceiver_node*> receiving_sessions_t;

	transfer_sessions_t		m_transfers;
	receiving_sessions_t	m_receivers;
	void stop_transfer_sessions		(buffer_vector<dst_src_pair_t> const & tsessions);
	void stop_receiving_sessions	(buffer_vector<ClientID> const & tsessions);
public:
	server_site						();
	~server_site					();

	void update_transfer			();
	void stop_obsolete_receivers	();
	void on_message					(NET_Packet* packet, ClientID const & sender);

	void start_transfer_file		(shared_str const & file_name, 
										ClientID const & to_client,
										ClientID const & from_client,
										sending_state_callback_t & tstate_callback);
	//this method used by proxies, to transmit file by portions through the memory writer ...
	void start_transfer_file		(CMemoryWriter& mem_writer,
										u32 mem_writer_max_size,
										ClientID const & to_client,
										ClientID const & from_client,
										sending_state_callback_t & tstate_callback,
										u32 const user_param);
	void start_transfer_file		(u8* data_ptr,
										u32 const data_size,
										ClientID const & to_client,
										ClientID const & from_client,
										sending_state_callback_t & tstate_callback,
										u32 const user_param);
	void start_transfer_file		(buffer_vector<mutable_buffer_t> & vector_of_buffers,
										ClientID const & to_client,
										ClientID const & from_client,
										sending_state_callback_t & tstate_callback,
										u32 const user_param);
	
	void stop_transfer_file			(dst_src_pair_t const & tofrom);

	
	filereceiver_node* start_receive_file	(shared_str const & file_name,
										ClientID const & from_client,
										receiving_state_callback_t & rstate_callback);
	filereceiver_node* start_receive_file	(CMemoryWriter& mem_writer,
										ClientID const & from_client,
										receiving_state_callback_t & rstate_callback);
	void stop_receive_file			(ClientID const & from_client);

	bool is_transfer_active			(ClientID const & to_client, ClientID const & from_client) const;
	bool is_receiving_active		(ClientID const & from_client) const;
};


class client_site
{
	filetransfer_node*		m_transfering;
	typedef associative_vector<ClientID, filereceiver_node*> receiving_sessions_t;
	receiving_sessions_t	m_receivers;
	void stop_receiving_sessions(buffer_vector<ClientID> const & tsessions);
#ifdef DEBUG
	CStatGraph*				m_stat_graph;
	void dbg_init_statgraph		();
	void dbg_update_statgraph	();
	void dbg_deinit_statgraph	();
#endif
public:
	client_site				();
	~client_site			();
	
	void update_transfer		();
	void stop_obsolete_receivers();
	void on_message				(NET_Packet* packet);
	
	void start_transfer_file	(shared_str const & file_name,
											sending_state_callback_t & tstate_callback);
	void start_transfer_file	(u8* data, u32 size,
											sending_state_callback_t & tstate_callback, u32 size_to_allocate = 0);
	void stop_transfer_file		();

	
	filereceiver_node* start_receive_file	(shared_str const & file_name,
											ClientID const & from_client,
											receiving_state_callback_t & rstate_callback);
	filereceiver_node* start_receive_file	(CMemoryWriter& mem_writer,
											ClientID const & from_client,
											receiving_state_callback_t & rstate_callback);
	void stop_receive_file		(ClientID const & from_client);

	inline bool is_transfer_active	() const { return m_transfering ? true : false; };
	inline bool is_receiving_active	(ClientID const & from_client) const { return m_receivers.find(from_client) != m_receivers.end(); };
};

}; //namespace file_transfer

#endif //#ifndef FILETRANSFER