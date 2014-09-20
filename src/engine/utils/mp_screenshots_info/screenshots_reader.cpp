#include "pch.h"
#include "screenshots_common.h"
#include "screenshots_reader.h"
//#include "screenshots_writer.h" //for writer::info_max_size

char const * ss_info_secion			= "screenshot_info";
char const * ss_player_name_key		= "player_name";
char const * ss_player_digest_key	= "player_digest";
char const * ss_admin_name_key		= "admin_name";
char const * ss_digital_sign_key	= "digital_sign";
char const * ss_creation_date		= "creation_date";

namespace screenshots
{

sign_verifyer::sign_verifyer() :
	xr_dsa_verifyer(p_number, q_number, g_number, public_key)
{
}
sign_verifyer::~sign_verifyer()
{
}

static char* search_info_section(u8* buffer, u32 buffer_size)
{
	u32	sstr_size		= xr_strlen(ss_info_secion);
	VERIFY				(buffer_size >= sstr_size);
	u8* rbegin			= buffer + (buffer_size - sstr_size);
	int r_size			= static_cast<int>(buffer_size - sstr_size);
	do
	{
		if (!memcmp(rbegin, ss_info_secion, sstr_size))
		{
			return static_cast<char*>((void*)rbegin);
		}
		--rbegin;
		--r_size;
	}
	while (r_size > 0);
	return NULL;
}


reader::reader(IReader* freader)
{
	m_info_section				= NULL;
	m_jpeg_data					= NULL;
	VERIFY						(freader);
	
	u32 file_size				= freader->elapsed();
	VERIFY						(file_size);
	m_jpeg_data					= static_cast<u8*>(xr_malloc(file_size + 1));
	m_jpeg_data_size			= file_size;
	freader->r					(m_jpeg_data, m_jpeg_data_size);
	m_jpeg_data[file_size]		= 0;

	char* tmp_info_begin		= search_info_section(
		m_jpeg_data,
		m_jpeg_data_size);
	
	if (!tmp_info_begin)
	{
		Msg("Can't find info section");
		return;
	}
	--tmp_info_begin;//- '['
	u32 m_info_size				= xr_strlen(tmp_info_begin);
	m_info_pos					= static_cast<u32>((u8*)tmp_info_begin - m_jpeg_data);

	IReader	tmp_reader			(tmp_info_begin, m_info_size);
	m_info_section				= xr_new<CInifile>(&tmp_reader);
}

reader::~reader()
{
	xr_free		(m_jpeg_data);
	xr_delete	(m_info_section);
}

shared_str const reader::player_name()
{
	VERIFY(is_valid());
	return m_info_section->r_string(ss_info_secion, ss_player_name_key);
}

shared_str const reader::player_cdkey_digest	()
{
	VERIFY(is_valid());
	return m_info_section->r_string(ss_info_secion, ss_player_digest_key);
}

/*shared_str const reader::admin_name			()
{
	VERIFY(is_valid());
	return m_info_section->r_string(ss_info_secion, ss_admin_name_key);
}*/

shared_str const reader::creation_date		()
{
	VERIFY(is_valid());
	return m_info_section->r_string(ss_info_secion, ss_creation_date);
}

bool const reader::verify						()
{
	VERIFY(is_valid());
	
	shared_str const tmp_sign		= m_info_section->r_string(
		ss_info_secion,
		ss_digital_sign_key);

	char* jpeg_info_start			= static_cast<char*>((void*)(m_jpeg_data + m_info_pos));
	jpeg_info_start[0]				= 0;
		
	xr_strcat(jpeg_info_start, m_info_size, player_name().c_str());
	xr_strcat(jpeg_info_start, m_info_size, player_cdkey_digest().c_str());
	//xr_strcat(jpeg_info_start, m_info_size, admin_name().c_str());
	xr_strcat(jpeg_info_start, m_info_size, creation_date().c_str());
	
	u32		jpeg_info_size		= xr_strlen(jpeg_info_start) + 1; //ending zero
	u32		jpeg_full_size		= m_info_pos + jpeg_info_size;

	return	m_verifyer.verify	(m_jpeg_data, jpeg_full_size, tmp_sign);
}

}//namespace screenshots