#ifndef SCREENSHOT_READER_INCLUDED
#define SCREENSHOT_READER_INCLUDED

#include "xr_dsa_verifyer.h"

namespace screenshots
{

class sign_verifyer : public xr_dsa_verifyer
{
public:
	sign_verifyer	();
	~sign_verifyer	();
};


class reader
{
public:
							reader				(IReader* freader);
							~reader				();

		shared_str const player_name			();
		shared_str const player_cdkey_digest	();
		//shared_str const admin_name				();
		shared_str const creation_date			();

		bool const			verify				();
		bool const			is_valid			() const { return m_info_section != NULL; };
		

private:
	reader() {};
	u8*				m_jpeg_data;
	u32				m_jpeg_data_size;

	u32				m_info_pos;
	u32				m_info_size;

	CInifile*		m_info_section;
	sign_verifyer	m_verifyer;

};//class reader



} //namespace screenshots


#endif //#ifndef SCREENSHOT_READER_INCLUDED