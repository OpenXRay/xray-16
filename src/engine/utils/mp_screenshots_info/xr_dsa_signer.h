#ifndef XR_DSA_SIGNER_INCLUDED
#define XR_DSA_SIGNER_INCLUDED

#include "../../3rd party/crypto/crypto.h"

typedef fastdelegate::FastDelegate1< long >	sha_process_yielder;

class xr_dsa_signer
{
public:
						xr_dsa_signer			(u8 const p_number[crypto::xr_dsa::public_key_length],
												 u8 const q_number[crypto::xr_dsa::private_key_length],
												 u8 const g_number[crypto::xr_dsa::public_key_length]);
						~xr_dsa_signer			();

	shared_str const	sign					(u8 const * data,
												 u32 data_size);
	shared_str const	sign_mt					(u8 const * data,
												 u32 data_size,
												 sha_process_yielder yielder);
protected:
	crypto::xr_dsa::private_key_t	m_private_key;
private:
	xr_dsa_signer() : m_dsa(NULL, NULL, NULL) {};
	
	crypto::xr_dsa					m_dsa;
	crypto::xr_sha256				m_sha;
	
}; //xr_dsa_signer

#endif //#ifndef XR_DSA_SIGNER_INCLUDED