#include "stdafx.h"
#include "xr_sha.h"

using namespace crypto;

const xr_sha1::sha_checksum_t& xr_sha1::calculate(const u8* data, u32 data_size, std::optional<sha_process_yielder> yielder)
{
    VERIFY(data_size);
    const u8* data_pos = data;
    CryptoPP::SHA1 sha_ctx{};
    long chunks_processed{};
    while (data_size != 0)
    {
        const u32 chunk_size = data_size >= BLOCK_SIZE ? BLOCK_SIZE : data_size;
        sha_ctx.Update(data_pos, chunk_size);
        data_pos += chunk_size;
        data_size -= chunk_size;
        if (yielder.has_value())
        {
            yielder.value()(chunks_processed);
            ++chunks_processed;
        }
    }
    m_buf.fill(0);
    sha_ctx.Final(m_buf.data());
    return { m_buf };
}

const inline xr_sha1::sha_checksum_t& xr_sha1::calculate(const u8* data, u32 data_size)
{
    return calculate(data, data_size, std::nullopt);
}
