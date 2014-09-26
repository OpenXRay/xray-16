#ifndef FILETRANSFER_COMMON
#define FILETRANSFER_COMMON

namespace file_transfer
{

u32 const	data_max_chunk_size = 4096;	//4Kb on update ~	80		Kb/sec
u32 const	data_min_chunk_size = 128;	//					2.5		Kb/sec

enum sending_status_t			//state for callback
{
	sending_data				= 0x00,
	sending_aborted_by_user		= 0x01,
	sending_rejected_by_peer	= 0x02,
	sending_complete			= 0x03
};
enum receiving_status_t			//state for callback
{
	receiving_data				= 0x00,
	receiving_aborted_by_peer	= 0x01,	
	receiving_aborted_by_user	= 0x02,
	receiving_timeout			= 0x03,
	receiving_complete			= 0x04
};

enum ft_command_t		//command byte to M_FILE_TRANSFER message ...
{
	receive_data		=	0x00,	//means that packet contain new chunk of data
	abort_receive		=	0x01,	//this command send by source site, if he aborts file receiving ..
	receive_rejected	=	0x02	//this command send by dest site, if he doesn't want file..
};

typedef fastdelegate::FastDelegate3<sending_status_t, u32, u32> sending_state_callback_t;
typedef fastdelegate::FastDelegate3<receiving_status_t, u32, u32> receiving_state_callback_t;

typedef std::pair<u8*, u32 const>			mutable_buffer_t;
typedef std::pair<u8 const*, u32 const>		const_buffer_t;



void make_reject_packet(NET_Packet& packet, ClientID const & client);
void make_abort_packet(NET_Packet& packet, ClientID const & client);

} //namespace file_transfer

#endif //#ifndef FILETRANSFER_COMMON