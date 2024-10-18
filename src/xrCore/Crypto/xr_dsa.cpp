#include "stdafx.h"
#include "xr_dsa.h"

#ifdef USE_OPENSSL
#   include <openssl/dsa.h>
#   include <openssl/evp.h>
#   include <openssl/core_names.h>
#endif

namespace crypto
{
#ifdef USE_OPENSSL
static_assert(xr_dsa::key_bit_length >= OPENSSL_DSA_FIPS_MIN_MODULUS_BITS);
#endif

xr_dsa::xr_dsa(u8 const p[public_key_length], u8 const q[private_key_length], u8 const g[public_key_length])
{
#ifdef USE_OPENSSL
    m_context = EVP_PKEY_CTX_new_from_name(nullptr, "DSA", nullptr);
    EVP_PKEY_fromdata_init(m_context);

    // Transform p, q, g into OpenSSL acceptable form
    auto* bn_p = BN_bin2bn(p, public_key_length,  nullptr);
    auto* bn_q = BN_bin2bn(q, private_key_length, nullptr);
    auto* bn_g = BN_bin2bn(g, public_key_length,  nullptr);

    OSSL_PARAM params[] =
    {
        OSSL_PARAM_BN(OSSL_PKEY_PARAM_FFC_P, bn_p, public_key_length),
        OSSL_PARAM_BN(OSSL_PKEY_PARAM_FFC_Q, bn_q, private_key_length),
        OSSL_PARAM_BN(OSSL_PKEY_PARAM_FFC_G, bn_g, public_key_length),
        OSSL_PARAM_END
    };

    // Fill the key with p, q, g data
    EVP_PKEY_fromdata(m_context, &m_key, EVP_PKEY_KEY_PARAMETERS, params);

    // Everything is in pkey now, clean the rest
    BN_free(bn_p);
    BN_free(bn_q);
    BN_free(bn_g);
#endif
}

xr_dsa::~xr_dsa()
{
#ifdef USE_OPENSSL
    EVP_PKEY_CTX_free(m_context);
    EVP_PKEY_free(m_key);
#endif
}

shared_str xr_dsa::sign(private_key_t const& priv_key, u8 const* data, u32 const data_size) const
{
#ifdef USE_OPENSSL
    BIGNUM* temp{};

    // Fill in the private key into existing m_key
    EVP_PKEY_fromdata_init(m_context);
    {
        temp = BN_bin2bn(priv_key.m_value, sizeof(priv_key.m_value), nullptr);
        OSSL_PARAM params[] =
        {
            OSSL_PARAM_BN(OSSL_PKEY_PARAM_PRIV_KEY, temp, sizeof(priv_key.m_value)),
            OSSL_PARAM_END
        };
        EVP_PKEY_fromdata(m_context, nullptr, EVP_PKEY_PRIVATE_KEY, params);
    }

    // Initialize the signing context
    EVP_PKEY_sign_init(m_context);

    // Finalize the signature and get the required buffer size
    size_t sign_size{};
    EVP_PKEY_sign(m_context, nullptr, &sign_size, data, data_size);
    u8* sign_dest = static_cast<u8*>(xr_alloca(sign_size));

    // Perform the actual signing
    EVP_PKEY_sign(m_context, sign_dest, &sign_size, data, data_size);

    // Convert the signature to a BIGNUM
    BN_bin2bn(sign_dest, sign_size, temp);
    shared_str ret = BN_bn2hex(temp);
    BN_free(temp);
    return ret;
#else
    Msg("! [%s] Engine was built without OpenSSL, verifying will always fail.", __FUNCTION__);
    return "";
#endif
}

bool xr_dsa::verify(public_key_t const& pub_key, u8 const* data, u32 const data_size, shared_str const& dsign) const
{
#ifdef USE_OPENSSL
    BIGNUM* temp{};

    // Convert the hex signature (dsign) to binary
    BN_hex2bn(&temp, dsign.c_str());
    const size_t sig_size = BN_num_bytes(temp);
    u8* sig_buff = static_cast<u8*>(xr_alloca(sig_size));
    BN_bn2bin(temp, sig_buff);

    EVP_PKEY_fromdata_init(m_context);
    {
        BN_bin2bn(pub_key.m_value, sizeof(pub_key.m_value), temp);
        OSSL_PARAM params[] =
        {
            OSSL_PARAM_BN(OSSL_PKEY_PARAM_PUB_KEY, temp, sizeof(pub_key.m_value)),
            OSSL_PARAM_END
        };
        EVP_PKEY_fromdata(m_context, nullptr, EVP_PKEY_PUBLIC_KEY, params);
    }
    BN_free(temp);

    // Initialize the context for verification
    EVP_PKEY_verify_init(m_context);

    // Perform the verification
    const bool ret = EVP_PKEY_verify(m_context, sig_buff, sig_size, data, data_size) == 1;
    return ret;
#else
    Msg("! [%s] Engine was built without OpenSSL, verifying will always fail.", __FUNCTION__);
    return false;
#endif
}
} // namespace crypto
