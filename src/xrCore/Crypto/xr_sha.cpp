#include "stdafx.h"
#include "xr_sha.h"

namespace crypto
{

const xr_sha1::sha_checksum_t& xr_sha1::calculate(const u8* data, u32 data_size)
{
    VERIFY(data_size);
    const u8* data_pos = data;
    CryptoPP::SHA1 sha_ctx{};
    while (data_size != 0) 
    {
        const u32 chunk_size = data_size >= BLOCK_SIZE ? BLOCK_SIZE : data_size;
        sha_ctx.Update(data_pos, chunk_size);
        data_pos += chunk_size;
        data_size -= chunk_size;
    }
    m_buf.fill(0);
    sha_ctx.Final(m_buf.data());
    return { m_buf };
}

}
