#include "stdafx.h"
#include "xr_dsa_signer.h"
#include <ctime>

xr_dsa_signer::xr_dsa_signer(u8 const p_number[crypto::xr_dsa::public_key_length],
    u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length])
    : m_private_key(), m_dsa(p_number, q_number, g_number)
{
    static_assert(crypto::xr_dsa::private_key_length == crypto::xr_sha1::DigestSize,
        "Private key size must be equal to digest value size.");
}

xr_dsa_signer::~xr_dsa_signer() {}
shared_str const xr_dsa_signer::sign(u8 const* data, u32 data_size)
{
    auto hash = crypto::xr_sha1::calculate(data, data_size);
#ifdef DEBUG
    IWriter* sign_data = FS.w_open("$logs$", "sign");
    sign_data->w(data, data_size);
    sign_data->w_string("sha_checksum");
    sign_data->w(hash.data(), crypto::xr_sha1::DigestSize);
    FS.w_close(sign_data);
#endif
    return m_dsa.sign(m_private_key, hash.data(), crypto::xr_sha1::DigestSize);
}

shared_str const xr_dsa_signer::sign_mt(u8 const* data, u32 data_size, crypto::yielder_t yielder)
{
    auto hash = crypto::xr_sha1::calculate_with_yielder(data, data_size, yielder);
    return m_dsa.sign(m_private_key, hash.data(), crypto::xr_sha1::DigestSize);
}

char* current_time(string64& dest_time)
{
    const time_t tmp_curr_time = std::time(nullptr);
    tm* tmp_tm = std::localtime(&tmp_curr_time);

    xr_sprintf(dest_time, sizeof(dest_time), "%02d.%02d.%d_%02d:%02d:%02d", tmp_tm->tm_mday, tmp_tm->tm_mon + 1,
        tmp_tm->tm_year + 1900, tmp_tm->tm_hour, tmp_tm->tm_min, tmp_tm->tm_sec);
    return dest_time;
}
