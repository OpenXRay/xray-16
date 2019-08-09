#pragma once
#include "xrCore/xrCore.h"
#include <cryptopp/sha.h>

#include <optional>
#include <array>

namespace crypto
{

using sha_process_yielder = fastdelegate::FastDelegate1<long>;

class XRCORE_API xr_sha1
{
public:
    static const size_t DIGEST_SIZE = CryptoPP::SHA1::DIGESTSIZE;
    static const size_t BLOCK_SIZE = CryptoPP::SHA1::BLOCKSIZE;
    using sha_checksum_t = std::array<u8, DIGEST_SIZE>;

    const xr_sha1::sha_checksum_t& calculate(const u8* data, u32 data_size, std::optional<sha_process_yielder> yielder);
    const xr_sha1::sha_checksum_t& calculate(const u8* data, u32 data_size);
private:
    // m_buf is added as a field, because in this case it will be 
    // allocated on the stack where xr_sha1 instance was created
    sha_checksum_t m_buf;
};

}

