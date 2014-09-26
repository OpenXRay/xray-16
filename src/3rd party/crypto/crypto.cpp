#include "crypto.h"
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/dsa.h>

#pragma comment(lib, "xrCore.lib")

namespace crypto
{

static void unsafe_xr_free(void* ptr)
{
	xr_free(ptr);
};

static unsigned char rnd_seed[] = "S.T.A.L.K.E.R. 4ever Rulezz !!!";

CRYPTO_API	void		xr_crypto_init	()
{
	string256 random_string;
	xr_sprintf					(random_string, "%I64d_%s", CPU::QPC(), rnd_seed);
	//sprintf_s					(random_string, "%s", rnd_seed);
	CRYPTO_set_mem_functions	(xr_malloc, xr_realloc, unsafe_xr_free);
	RAND_seed					(random_string, xr_strlen(random_string));
	//unsigned int siglen;
	//tmp_dsa_params->flags |= DSA_FLAG_NO_EXP_CONSTTIME;
	//ZeroMemory(sig, sizeof(sig));
	//DSA_sign(0, digest, sizeof(digest), sig, &siglen, tmp_dsa_params);
	//DSA_verify(0, digest, sizeof(digest), sig, siglen, tmp_dsa_params);
/*#ifdef DEBUG
	CRYPTO_dbg_set_options		(V_CRYPTO_MDEBUG_ALL);
	CRYPTO_mem_ctrl				(CRYPTO_MEM_CHECK_ON);
#endif*/
}

} //namespace crypto