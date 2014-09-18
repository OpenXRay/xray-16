#ifndef FILETRANSFER_NODE
#define FILETRANSFER_NODE

#include "filetransfer_common.h"
#include <boost/noncopyable.hpp>

namespace file_transfer
{

class file_reader : private boost::noncopyable
{
public:
					file_reader			() {};
	virtual			~file_reader		() {};
	
	virtual	bool	make_data_packet	(NET_Packet & packet, u32 chunk_size) = 0;
	virtual bool	is_first_packet		() = 0;
	virtual u32		size				() = 0;
	virtual u32		tell				() = 0;
	virtual	bool	opened				() const = 0;
}; //class file_reader

class disk_file_reader : public file_reader
{
	IReader*		m_reader;
public:
	explicit		disk_file_reader	(shared_str const & file_name);
	virtual			~disk_file_reader	();
	virtual	bool	make_data_packet	(NET_Packet & packet, u32 chunk_size);
	virtual bool	is_first_packet		();
	virtual u32		size				();
	virtual u32		tell				();
	virtual	bool	opened				() const;
}; //class disk_file_reader

class memory_reader : public file_reader
{
	IReader*		m_reader;
public:
					memory_reader		(u8* data_ptr, u32 data_size);
	virtual			~memory_reader		();
	virtual	bool	make_data_packet	(NET_Packet & packet, u32 chunk_size);
	virtual bool	is_first_packet		();
	virtual u32		size				();
	virtual u32		tell				();
	virtual	bool	opened				() const;
}; //class memory_reader

class buffers_vector_reader : public file_reader
{
public:
	explicit		buffers_vector_reader	(buffer_vector<mutable_buffer_t>* buffers);
	virtual			~buffers_vector_reader	();
	virtual	bool	make_data_packet		(NET_Packet & packet, u32 chunk_size);
	virtual bool	is_first_packet			();
	virtual u32		size					();
	virtual u32		tell					();
	virtual	bool	opened					() const;
private:
			void	accumulate_size			();
			void	read_from_current_buf	(NET_Packet & dest, u32 read_size);
	
	typedef xr_deque<mutable_buffer_t>		buffers_vector_t;
	
	buffers_vector_t						m_buffers;
	u32										m_current_buf_offs;
	u32										m_complete_buffers_size;
	u32										m_sum_size;
}; // class buffer_vector_reader

class memory_writer_reader : public file_reader
{
	CMemoryWriter*	m_writer_as_src;
	u32				m_writer_pointer;	// to read ..
	u32		const	m_writer_max_size;
public:
					memory_writer_reader	(CMemoryWriter* src_writer, u32 const max_size);
	virtual			~memory_writer_reader	();
	virtual	bool	make_data_packet		(NET_Packet & packet, u32 chunk_size);
	virtual bool	is_first_packet			();
	virtual u32		size					();
	virtual u32		tell					();
	virtual	bool	opened					() const;
}; //class memory_writer_reader



class filetransfer_node
{
private:
	u32 				m_chunk_size;
	u32					m_last_peak_throughput;
	u32					m_last_chunksize_update_time;
	u32					m_user_param;
	
	file_reader*		m_reader;
	
	sending_state_callback_t	m_process_callback;
public:
	filetransfer_node	(shared_str const & file_name, u32 const chunk_size, sending_state_callback_t const & callback);
	filetransfer_node	(u8* data, u32 const data_size, u32 const chunk_size, sending_state_callback_t const & callback, u32 user_param = 0);
	filetransfer_node	(buffer_vector<mutable_buffer_t>* vector_of_buffers, u32 const chunk_size, sending_state_callback_t const & callback, u32 user_param = 0);
	filetransfer_node	(CMemoryWriter* src_writer, u32 const max_size, u32 const chunk_size, sending_state_callback_t const & callback, u32 user_param = 0);
	
	filetransfer_node& operator=(filetransfer_node const & copy) {NODEFAULT;};
	~filetransfer_node	();

	void calculate_chunk_size	(u32 peak_throughput, u32 current_throughput);
	bool make_data_packet		(NET_Packet & packet);	//returns true if this is a last packet ...
	void signal_callback		(sending_status_t status);
	bool is_complete			();
	bool is_ready_to_send		();
	
	//inline	shared_str const &	get_file_name	() { return m_file_name; };
	bool						opened			() const;
	//inline	IReader*			get_reader		() { return m_reader; };
	inline	u32 const			get_chunk_size	() const { return m_chunk_size; };
}; //class filetransfer_node

} //namespace file_transfer

#endif //#ifndef FILETRANSFER_NODE