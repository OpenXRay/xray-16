#ifndef CRYPTO_INCLUDED
#define CRYPTO_INCLUDED

#include "xrCore/xrCore.h"
#include "xr_dsa.h"
#include "xr_dsa_signer.h"
#include "xr_dsa_verifyer.h"
#include "xr_sha.h"

namespace crypto
{

void xr_crypto_init();

} //namespace crypto

#endif