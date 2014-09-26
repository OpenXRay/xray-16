#ifndef SCREENSHOT_SERVER
#define SCREENSHOT_SERVER
#include "file_transfer.h"
#include <boost/noncopyable.hpp>

enum clientdata_event_t
{
	e_screenshot_request			=	0x00,
	e_configs_request,
	e_screenshot_response,
	e_configs_response,
	e_screenshot_error_notif,
	e_configs_error_notif
};

class clientdata_proxy : boost::noncopyable
{
private:
	ClientID						m_admin_id;		//for file transfer
	ClientID						m_chearer_id;	//for file receiving
	shared_str						m_cheater_name;
	shared_str						m_cheater_digest;
	CMemoryWriter					my_proxy_mem_file;
	void							save_proxy_screenshot();
	void							save_proxy_config();//compressed
	bool							m_first_receive;
	file_transfer::filereceiver_node*	m_receiver;

	file_transfer::server_site*		m_ft_server;
	//memory file
	void notify_admin(clientdata_event_t event_for_admin, char const * reason);
	clientdata_proxy() {};
public:
	clientdata_proxy(file_transfer::server_site* ft_server);
	~clientdata_proxy();

	void make_screenshot	(ClientID const & admin_id, ClientID const & cheater_id);
	void make_config_dump	(ClientID const & admin_id, ClientID const & cheater_id);

	bool is_active();

	void __stdcall download_screenshot_callback(file_transfer::receiving_status_t status,
										u32 downloaded, u32 total);
	void __stdcall download_config_callback(file_transfer::receiving_status_t status,
										u32 downloaded, u32 total);
	void __stdcall upload_file_callback(file_transfer::sending_status_t status,
										u32 uploaded, u32 total);
}; //class clientdata_proxy

#endif //#ifndef SCREENSHOT_SERVER