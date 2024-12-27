#include "stdafx.h"
#include "xr_sha.h"

#ifdef USE_OPENSSL
#   include <openssl/sha.h>
#   include <openssl/evp.h>
#endif

namespace crypto
{
#ifdef USE_OPENSSL
static_assert(xr_sha1::DigestSize == SHA_DIGEST_LENGTH);
#endif

xr_sha1::hash_t xr_sha1::calculate_with_yielder(const u8* data, u32 data_size, const yielder_t& yielder)
{
#ifdef USE_OPENSSL
    R_ASSERT1_CURE(data && data_size,
    {
        return {};
    });

    EVP_MD_CTX* sha_ctx = EVP_MD_CTX_new();
    R_ASSERT1_CURE(sha_ctx,
    {
        return {};
    });
    R_ASSERT1_CURE(EVP_DigestInit_ex(sha_ctx, EVP_sha1(), nullptr) != 0,
    {
        EVP_MD_CTX_free(sha_ctx);
        return {};
    });

    const u8* data_pos = data;
    long chunks_processed{};
    while (0 != data_size)
    {
        const u32 chunk_size = data_size >= BlockSize ? BlockSize : data_size;
        EVP_DigestUpdate(sha_ctx, data_pos, chunk_size);
        data_pos += chunk_size;
        data_size -= chunk_size;
        yielder(chunks_processed);
        ++chunks_processed;
    }

    unsigned int hash_len{};
    hash_t result;

    [[maybe_unused]]
    const bool success = EVP_DigestFinal_ex(sha_ctx, result.data(), &hash_len) != 0;
    VERIFY(success);
    VERIFY(hash_len == DigestSize);

    EVP_MD_CTX_free(sha_ctx);
    return result;
#else
    return {};
#endif
}
} // namespace crypto

