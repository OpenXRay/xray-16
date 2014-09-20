#ifndef SCREENSHOT_WRITER_INCLUDED
#define SCREENSHOT_WRITER_INCLUDED

#include "xr_dsa_signer.h"

namespace screenshots
{

class signer : public xr_dsa_signer
{
public:
			signer					();
			~signer					();
private:
	void	feel_private_dsa_key	();
};

extern char const * ss_info_secion;
extern char const * ss_player_name_key;
extern char const * ss_player_digest_key;
//extern char const * ss_admin_name_key;
extern char const * ss_digital_sign_key;
extern char const * ss_creation_date;


class writer
{
public:
			writer	(u8* jpeg_data, u32 jpeg_size, u32 jpeg_buffer_size);
			~writer	();
	
	static u32 const info_max_size	= 1024;
	void	set_player_name			(shared_str const & pname);
	void	set_player_cdkey_digest	(shared_str const & cdkeydigest);
	//void	set_admin_name			(shared_str const & admin_name);

	u32	const write_info			(sha_process_yielder* yielder = NULL);
private:
	writer() : m_info_data(NULL, FALSE, FALSE, FALSE) {};
	
	u8*			m_buffer;
	u32			m_buffer_size;
	u32			m_buffer_info_pos;
	
	CInifile	m_info_data;
	signer		m_signer;
};//class writer

} //namespace screenshots

#endif //#ifndef SCREENSHOT_WRITER_INCLUDED