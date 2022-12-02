#ifndef SCREENSHOTS_COMMON_INCLUDED
#define SCREENSHOTS_COMMON_INCLUDED

#include "xrCore/Crypto/xr_dsa.h"

namespace screenshots
{
extern char const* ss_info_secion;
extern char const* ss_player_name_key;
extern char const* ss_player_digest_key;
// extern char const * ss_admin_name_key;
extern char const* ss_digital_sign_key;
extern char const* ss_creation_date;

extern u8 const p_number[crypto::xr_dsa::public_key_length];
extern u8 const q_number[crypto::xr_dsa::private_key_length];
extern u8 const g_number[crypto::xr_dsa::public_key_length];
extern u8 const public_key[crypto::xr_dsa::public_key_length];
} // namespace screenshots

#endif //#ifndef SCREENSHOTS_COMMON_INCLUDED
