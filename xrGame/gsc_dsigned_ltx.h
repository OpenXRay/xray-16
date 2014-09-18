#ifndef GSC_DSINGNED_LTX_INCLUDED
#define GSC_DSINGNED_LTX_INCLUDED

#include "../xrCore/fastdelegate.h"
#include "xr_dsa_signer.h"
#include "xr_dsa_verifyer.h"

class gsc_dsigned_ltx_writer : private xr_dsa_signer
{
public:
	typedef void (*priv_key_filler_function_t)(crypto::xr_dsa::private_key_t & priv_key_dest);

				gsc_dsigned_ltx_writer		(u8 const p_number[crypto::xr_dsa::public_key_length],
											 u8 const q_number[crypto::xr_dsa::private_key_length],
											 u8 const g_number[crypto::xr_dsa::public_key_length],
											 priv_key_filler_function_t pkf_func);
				~gsc_dsigned_ltx_writer		();
	void		sign_and_save				(IWriter& writer);
	CInifile &	get_ltx						() { return m_ltx; };
private:
	CInifile			m_ltx;
	CMemoryWriter		m_mem_writer;
}; //class gsc_dsigned_ltx_writer

class gsc_dsigned_ltx_reader : private xr_dsa_verifyer
{
public:
				gsc_dsigned_ltx_reader		(u8 const p_number[crypto::xr_dsa::public_key_length],
											 u8 const q_number[crypto::xr_dsa::private_key_length],
											 u8 const g_number[crypto::xr_dsa::public_key_length],
											 u8 const public_key[crypto::xr_dsa::public_key_length]);
				~gsc_dsigned_ltx_reader		();
	bool		load_and_verify				(u8* buffer, u32 const size);
	CInifile &	get_ltx						() { return *m_ltx; };
private:
	CInifile*	m_ltx;
}; //class gsc_dsigned_ltx_reader


#endif //#ifndef GSC_DSINGNED_LTX_INCLUDED