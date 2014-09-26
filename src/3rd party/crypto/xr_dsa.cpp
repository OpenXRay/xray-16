#include "xr_dsa.h"
#include "crypto.h"
#include <openssl/dsa.h>

namespace crypto
{

xr_dsa::xr_dsa(u8 const p[public_key_length],
			   u8 const q[private_key_length],
			   u8 const g[public_key_length])
{
	m_dsa			= DSA_new();
	m_dsa->p		= BN_new();
	m_dsa->q		= BN_new();
	m_dsa->g		= BN_new();
	m_dsa->priv_key	= BN_new();
	m_dsa->pub_key	= BN_new();

	BN_bin2bn(p, public_key_length, m_dsa->p);
	BN_bin2bn(q, private_key_length, m_dsa->q);
	BN_bin2bn(g, public_key_length, m_dsa->g);
}

xr_dsa::~xr_dsa()
{
	DSA_free	(m_dsa);
}

shared_str const xr_dsa::sign		(private_key_t const & priv_key,
									 u8 const* data,
									 u32 const data_size)
{
	BN_bin2bn(priv_key.m_value, sizeof(priv_key.m_value), m_dsa->priv_key);

	unsigned int	sign_size = DSA_size(m_dsa);
	u8*				sign_dest = static_cast<u8*>(
		_alloca(sign_size));
	
	BIGNUM			tmp_sign_res_bn;
	BN_init			(&tmp_sign_res_bn);
	
	DSA_sign		(0, data, data_size, sign_dest, &sign_size, m_dsa);
	BN_bin2bn		(sign_dest, sign_size, &tmp_sign_res_bn);

	return			shared_str(BN_bn2hex(&tmp_sign_res_bn));
}

bool		xr_dsa::verify				(public_key_t const & pub_key,
										 u8 const * data,
										 u32 const data_size,
										 shared_str const & dsign)
{
	BN_bin2bn(pub_key.m_value, sizeof(pub_key.m_value), m_dsa->pub_key);

	BIGNUM*	tmp_bn			= NULL;
	BN_hex2bn				(&tmp_bn, dsign.c_str());
	int	sig_size			= tmp_bn->top * sizeof(unsigned long);
	u8* sig_buff			= static_cast<u8*>(_alloca(sig_size));
	VERIFY					(sig_size == DSA_size(m_dsa));
	BN_bn2bin				(tmp_bn, sig_buff);
		
	bool ret = DSA_verify	(0, data, data_size, sig_buff, sig_size, m_dsa) == 1 ? true : false;
	BN_free(tmp_bn);
	return ret;
}



#ifdef DEBUG

static void dsa_genparams_cb(int p, int n, void *arg)
{
	Msg("* dsa genparams cb(%d, %d)", p, n);
}

static unsigned char rnd_seed[] = "S.T.A.L.K.E.R. 4ever Rulezz !!!";
void print_big_number(BIGNUM* big_num, u32 max_columns = 8)
{
	u8			bin_buff[xr_dsa::public_key_length];//public_key_length is the max
	int			bin_size = 0;

	string4096	result_buffer;
	string16	tmp_buff;

	ZeroMemory	(bin_buff, sizeof(bin_buff));
	BN_bn2bin	(big_num, bin_buff);
	bin_size	= big_num->top * sizeof(unsigned long);

	result_buffer[0] = 0;
	xr_strcat(result_buffer, "\t");
	for (int i = 0; i < bin_size; ++i)
	{
		if (((i % max_columns) == 0) && (i > 0))
		{
			xr_strcat(result_buffer, "\n\t");
		}
		xr_sprintf(tmp_buff, "0x%02x, ", bin_buff[i]);
		xr_strcat(result_buffer, tmp_buff);
	}
	Msg(result_buffer);
};

void xr_dsa::generate_params()
{
	int counter;
	unsigned long	long_ret;
	string256		random_string;
	xr_sprintf					(random_string, "%I64d_%s", CPU::QPC(), rnd_seed);
	//sprintf_s					(random_string, "%s", rnd_seed);
	unsigned char*	rnd_seed	= static_cast<unsigned char*>((void*)random_string);
	unsigned int	rnd_ssize	= xr_strlen(random_string);
	DSA* tmp_dsa_params	= DSA_generate_parameters(
		key_bit_length,
		rnd_seed,
		rnd_ssize,
		&counter,
		&long_ret,
		dsa_genparams_cb,
		NULL
	);
	DSA_generate_key	(tmp_dsa_params);

	VERIFY				(tmp_dsa_params->p->top * sizeof(u32)		== public_key_length);
	VERIFY				(tmp_dsa_params->q->top * sizeof(u32)		== private_key_length);
	VERIFY				(tmp_dsa_params->g->top * sizeof(u32)		== public_key_length);
	VERIFY				(tmp_dsa_params->pub_key->top * sizeof(u32) == public_key_length);
	VERIFY				(tmp_dsa_params->priv_key->top * sizeof(u32)== private_key_length);
	
	Msg("// DSA params ");
	
	Msg("u8 const p_number[crypto::xr_dsa::public_key_length] = {");
	print_big_number	(tmp_dsa_params->p);
	Msg("};//p_number");

	
	Msg("u8 const q_number[crypto::xr_dsa::private_key_length] = {");
	print_big_number	(tmp_dsa_params->q);
	Msg("};//q_number");
	
	
	Msg("u8 const g_number[crypto::xr_dsa::public_key_length] = {");
	print_big_number	(tmp_dsa_params->g);
	Msg("};//g_number");

	Msg("u8 const public_key[crypto::xr_dsa::public_key_length] = {");
	print_big_number	(tmp_dsa_params->pub_key);
	Msg("};//public_key");

	
	u8	priv_bin[private_key_length];
	BN_bn2bin			(tmp_dsa_params->priv_key, priv_bin);
	Msg("// Private key:");
	for (int i = 0; i < private_key_length; ++i)
	{
		Msg("	m_private_key.m_value[%d]	= 0x%02x;", i, priv_bin[i]);
	}

	u8	debug_digest[]		= "this is a test";
	u8	debug_bad_digest[]	= "this as a test";

	u32		siglen			= DSA_size(tmp_dsa_params);
	u8*		sig				= static_cast<u8*>(_alloca(siglen));

	BIGNUM	bn_sign;
	BN_init					(&bn_sign);
	
	VERIFY	(DSA_sign(0, debug_digest, sizeof(debug_digest), sig, &siglen, tmp_dsa_params) == 1);

	BN_bin2bn				(sig, siglen, &bn_sign);
	shared_str sig_str		= BN_bn2hex(&bn_sign);
	
	BIGNUM*	bn_rsing		= NULL;
	ZeroMemory				(sig, siglen);
	BN_hex2bn				(&bn_rsing, sig_str.c_str());
	BN_bn2bin				(bn_rsing, sig);
	BN_free					(bn_rsing);

	VERIFY	(DSA_verify(0, debug_digest, sizeof(debug_digest), sig, siglen, tmp_dsa_params) == 1);

	VERIFY	(DSA_verify(0, debug_bad_digest, sizeof(debug_bad_digest), sig, siglen, tmp_dsa_params) == 0);

	DSA_free(tmp_dsa_params);
}

#endif //#ifdef DEBUG


} //namespace crypto