#pragma once
#include "xrCore/xrCore.h"
#include <cryptopp/sha.h>
#include <array>

namespace crypto
{
class XRCORE_API xr_sha1
{
public:
    static const size_t DIGEST_SIZE = CryptoPP::SHA1::DIGESTSIZE;
    static const size_t BLOCK_SIZE = CryptoPP::SHA1::BLOCKSIZE;
    using sha_checksum_t = std::array<u8, DIGEST_SIZE>;

    const xr_sha1::sha_checksum_t& calculate(const u8* data, u32 data_size);
private:
    // m_buf is added as a field, because in this case it will be 
    // allocated on the stack where xr_sha1 instance was created
    sha_checksum_t m_buf;
};
}

