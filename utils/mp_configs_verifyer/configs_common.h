#ifndef CONFIGS_COMMON_INCLUDED
#define CONFIGS_COMMON_INCLUDED

#include "../../3rd party/crypto/crypto.h"

namespace mp_anticheat
{

extern u8 const p_number[crypto::xr_dsa::public_key_length];
extern u8 const q_number[crypto::xr_dsa::private_key_length];
extern u8 const g_number[crypto::xr_dsa::public_key_length];
extern u8 const public_key[crypto::xr_dsa::public_key_length];


} //namespace mp_anticheat

#endif //CONFIGS_COMMON_INCLUDED