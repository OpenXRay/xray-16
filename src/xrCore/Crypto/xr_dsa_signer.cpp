#include "stdafx.h"
#include "xr_dsa_signer.h"

xr_dsa_signer::xr_dsa_signer(u8 const p_number[crypto::xr_dsa::public_key_length],
							 u8 const q_number[crypto::xr_dsa::private_key_length],
							 u8 const g_number[crypto::xr_dsa::public_key_length]) :
	m_dsa(p_number, q_number, g_number)
{
	STATIC_CHECK(crypto::xr_dsa::private_key_length  == crypto::xr_sha256::digest_length, private_key_size_must_be_equal_to_digest_value_size);
}

xr_dsa_signer::~xr_dsa_signer()
{
}

shared_str const xr_dsa_signer::sign(u8 const * data,
									 u32 data_size)
{
	m_sha.start_calculate	(data, data_size);
	while					(!m_sha.continue_calculate()) {};
#ifdef DEBUG
	IWriter*	sign_data	= FS.w_open("$logs$", "sign");
	sign_data->w(data, data_size);
	sign_data->w_string("sha_checksum");
	sign_data->w(m_sha.pointer(), m_sha.digest_length);
	FS.w_close				(sign_data);
#endif
	return m_dsa.sign		(m_private_key, m_sha.pointer(), m_sha.digest_length);
}

shared_str const xr_dsa_signer::sign_mt(u8 const * data,
										u32 data_size,
										sha_process_yielder yielder)
{
	m_sha.start_calculate	(data, data_size);
	long sha_process_counter = 0;
	while					(!m_sha.continue_calculate())
	{
		yielder(sha_process_counter);
		++sha_process_counter;
	}
	return m_dsa.sign		(m_private_key, m_sha.pointer(), m_sha.digest_length);
}

char const * current_time(string64 & dest_time)
{
	time_t		tmp_curr_time;
	
	dest_time[0]	= 0;
	_time64			(&tmp_curr_time);
	tm* tmp_tm		= _localtime64(&tmp_curr_time);

	xr_sprintf(dest_time, sizeof(dest_time),
		"%02d.%02d.%d_%02d:%02d:%02d",
		tmp_tm->tm_mday, 
		tmp_tm->tm_mon+1, 
		tmp_tm->tm_year+1900, 
		tmp_tm->tm_hour, 
		tmp_tm->tm_min, 
		tmp_tm->tm_sec
	);
	return dest_time;
}

	



