#ifndef SERVER_INFO_UPLOADER
#define SERVER_INFO_UPLOADER

#include <boost/noncopyable.hpp>
#include "../xrCore/fastdelegate.h"
#include "file_transfer.h"

typedef fastdelegate::FastDelegate<void (ClientID const &)>	svinfo_upload_complete_cb;

class server_info_uploader : boost::noncopyable
{
	enum ESvInfoUploadState
	{
		eUploadNotActive	= 0x00,
		eUploadingInfo
	};//enum ESvInfoUploadState
	ESvInfoUploadState			m_state;
	u8*							m_logo_data;
	u32							m_logo_size;
	u8*							m_rules_data;
	u32							m_rules_size;
	ClientID					m_to_client;
	ClientID					m_from_client;
	svinfo_upload_complete_cb	m_complete_cb;
	
	file_transfer::server_site*	m_file_transfers;

	void			terminate_upload			();
	void			execute_complete_cb			();
	
public:
	explicit		server_info_uploader		(file_transfer::server_site* file_transfers);
					~server_info_uploader		();

	inline	bool	is_active					()	const { return m_state != eUploadNotActive; };
	void			start_upload_info			(IReader const * svlogo,
												 IReader const * svrules,
												 ClientID const & toclient,
												 svinfo_upload_complete_cb const & complete_cb);
	void __stdcall upload_server_info_callback	(file_transfer::sending_status_t status,
												 u32 uploaded, u32 total);
}; //class server_info_uploader

#endif //#ifndef SERVER_INFO_UPLOADER