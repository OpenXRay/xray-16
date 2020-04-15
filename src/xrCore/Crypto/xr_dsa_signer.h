#pragma once
#ifndef XR_DSA_SIGNER_INCLUDED
#define XR_DSA_SIGNER_INCLUDED

#include "xr_dsa.h"
#include "xr_sha.h"

class XRCORE_API xr_dsa_signer
{
public:
    xr_dsa_signer(u8 const p_number[crypto::xr_dsa::public_key_length],
        u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length]);
    ~xr_dsa_signer();

    shared_str const sign(u8 const* data, u32 data_size);
    shared_str const sign_mt(u8 const* data, u32 data_size, crypto::yielder_t yielder);

protected:
    crypto::xr_dsa::private_key_t m_private_key;

private:
    xr_dsa_signer() : m_private_key(), m_dsa(nullptr, nullptr, nullptr){}

    crypto::xr_dsa m_dsa;

}; // xr_dsa_signer

XRCORE_API char* current_time(string64& dest_time);

#endif //#ifndef XR_DSA_SIGNER_INCLUDED
