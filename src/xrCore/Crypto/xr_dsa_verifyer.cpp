#include "stdafx.h"
#include "xr_dsa_verifyer.h"

xr_dsa_verifyer::xr_dsa_verifyer(u8 const p_number[crypto::xr_dsa::public_key_length],
    u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length],
    u8 const public_key[crypto::xr_dsa::public_key_length])
    : m_dsa(p_number, q_number, g_number)
{
    static_assert(sizeof(m_public_key.m_value) == crypto::xr_dsa::public_key_length, "Public key sizes not equal.");
    CopyMemory(m_public_key.m_value, public_key, sizeof(m_public_key.m_value));
}

xr_dsa_verifyer::~xr_dsa_verifyer() {}
std::optional<crypto::xr_sha1::hash_t> xr_dsa_verifyer::verify(u8 const* data, u32 data_size, shared_str const& dsign)
{
    auto hash = crypto::xr_sha1::calculate(data, data_size);
#ifdef DEBUG
    IWriter* verify_data = FS.w_open("$logs$", "verify");
    verify_data->w(data, data_size);
    verify_data->w_string("sha_checksum");
    verify_data->w(hash.data(), crypto::xr_sha1::DigestSize);
    FS.w_close(verify_data);
#endif
    const bool success = m_dsa.verify(m_public_key, hash.data(), crypto::xr_sha1::DigestSize, dsign);
    return success ? std::optional<crypto::xr_sha1::hash_t>{hash} : std::nullopt;
}
