#ifndef XR_SHA_INCLUDED
#define XR_SHA_INCLUDED

#include "crypto_api_def.h"
#include "../../xrCore/xrCore.h"

struct SHAstate_st;

namespace crypto
{

class CRYPTO_API xr_sha256
{
public:
	static u32 const digest_length	= 20; //SHA_DIGEST_LENGTH

				xr_sha256		();
				~xr_sha256		();

	void	start_calculate		(u8 const * data, u32 data_size);
	bool	continue_calculate	();
	
	u8 const *	pointer			()	const { return m_result; };
private:
	static u32 const calc_chunk_size = 512;

	u8 const *		m_data_src;
	u32				m_data_size;

	u8				m_result[digest_length];
	SHAstate_st*	m_sha_ctx;
}; //xr_sha256

} //namespace crypto

#endif //#ifndef XR_SHA_INCLUDED