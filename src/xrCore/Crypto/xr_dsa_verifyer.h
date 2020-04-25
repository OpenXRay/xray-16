#pragma once
#ifndef XR_DSA_VERIFYER_INCLUDED
#define XR_DSA_VERIFYER_INCLUDED

#include <optional>

#include "xr_dsa.h"
#include "xr_sha.h"

class XRCORE_API xr_dsa_verifyer
{
public:
    xr_dsa_verifyer(u8 const p_number[crypto::xr_dsa::public_key_length],
        u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length],
        u8 const public_key[crypto::xr_dsa::public_key_length]);

    ~xr_dsa_verifyer();

    std::optional<crypto::xr_sha1::hash_t> verify(u8 const* data, u32 data_size, shared_str const& dsign);
protected:
    crypto::xr_dsa::public_key_t m_public_key;

private:
    crypto::xr_dsa m_dsa;
}; // class xr_dsa_verifyer

#endif //#ifndef XR_DSA_VERIFYER_INCLUDED
