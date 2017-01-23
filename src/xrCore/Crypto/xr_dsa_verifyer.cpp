#include "stdafx.h"
#include "xr_dsa_verifyer.h"

xr_dsa_verifyer::xr_dsa_verifyer(u8 const p_number[crypto::xr_dsa::public_key_length],
								 u8 const q_number[crypto::xr_dsa::private_key_length],
								 u8 const g_number[crypto::xr_dsa::public_key_length],
								 u8 const public_key[crypto::xr_dsa::public_key_length]) :
	m_dsa(p_number, q_number, g_number)
{
	STATIC_CHECK(
		sizeof(m_public_key.m_value) == crypto::xr_dsa::public_key_length,
		public_key_sizes_not_equal);
	CopyMemory(m_public_key.m_value, public_key, sizeof(m_public_key.m_value));
}

xr_dsa_verifyer::~xr_dsa_verifyer()
{
}

bool xr_dsa_verifyer::verify(u8 const * data,
							 u32 data_size,
							 shared_str const & dsign)
{
	m_sha.start_calculate	(data, data_size);
	while					(!m_sha.continue_calculate());
#ifdef DEBUG
	IWriter*	verify_data	= FS.w_open("$logs$", "verify");
	verify_data->w(data, data_size);
	verify_data->w_string("sha_checksum");
	verify_data->w(m_sha.pointer(), m_sha.digest_length);
	FS.w_close				(verify_data);
#endif
	return m_dsa.verify		(m_public_key, m_sha.pointer(), m_sha.digest_length, dsign);
}


