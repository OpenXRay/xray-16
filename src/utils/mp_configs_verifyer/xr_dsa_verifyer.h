#ifndef XR_DSA_VERIFYER_INCLUDED
#define XR_DSA_VERIFYER_INCLUDED

#include "../../3rd party/crypto/crypto.h"

class xr_dsa_verifyer
{
public:
	xr_dsa_verifyer				(u8 const p_number[crypto::xr_dsa::public_key_length],
								 u8 const q_number[crypto::xr_dsa::private_key_length],
								 u8 const g_number[crypto::xr_dsa::public_key_length],
								 u8 const public_key[crypto::xr_dsa::public_key_length]);

	~xr_dsa_verifyer			();

	bool			verify				(u8 const * data,
										 u32 data_size,
										 shared_str const & dsign);
	u8 const*		get_sha_checksum	() const { return m_sha.pointer(); };
protected:
	crypto::xr_dsa::public_key_t	m_public_key;
private:
	crypto::xr_dsa			m_dsa;
	crypto::xr_sha256		m_sha;
};//class xr_dsa_verifyer


#endif //#ifndef XR_DSA_VERIFYER_INCLUDED