#pragma once
#include "xrCore/xrCore.h"
#include <cryptopp/sha.h>

#include <optional>
#include <array>

namespace crypto
{

// TODO: Move this to common crypto header file
using yielder_t = fastdelegate::FastDelegate1<long>;

class XRCORE_API xr_sha1
{
public:
    static const size_t DIGEST_SIZE = CryptoPP::SHA1::DIGESTSIZE;
    static const size_t BLOCK_SIZE = CryptoPP::SHA1::BLOCKSIZE;
    using hash_t = std::array<u8, DIGEST_SIZE>;

public:
    xr_sha1() = delete;
    static void calculate(hash_t& result, const u8* data, u32 data_size, std::optional<yielder_t> yielder);
    static void calculate(hash_t& result, const u8* data, u32 data_size);
};

}

