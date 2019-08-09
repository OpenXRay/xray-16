#include "stdafx.h"
#include "xr_sha.h"

using namespace crypto;

void xr_sha1::calculate(hash_t& result, const u8* data, u32 data_size, std::optional<yielder_t> yielder)
{
    VERIFY(data_size);
    const u8* data_pos = data;
    CryptoPP::SHA1 sha_ctx{};
    long chunks_processed{};
    while (0 != data_size)
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
    sha_ctx.Final(result.data());
}

void xr_sha1::calculate(hash_t& result, const u8* data, u32 data_size)
{
    return calculate(result, data, data_size, std::nullopt);
}
