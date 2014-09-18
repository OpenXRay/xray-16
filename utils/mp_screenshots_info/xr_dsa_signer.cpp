#include "pch.h"
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
	



