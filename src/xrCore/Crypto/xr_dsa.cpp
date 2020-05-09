#include "stdafx.h"
#include "xr_dsa.h"

#ifdef USE_CRYPTOPP
#include <cryptopp/dsa.h>
#include <cryptopp/integer.h>
#endif

namespace crypto
{
xr_dsa::xr_dsa(u8 const p[public_key_length], u8 const q[private_key_length], u8 const g[public_key_length])
{
#ifdef USE_CRYPTOPP
    CryptoPP::Integer p_number(p, public_key_length);
    CryptoPP::Integer q_number(q, private_key_length);
    CryptoPP::Integer g_number(g, public_key_length);

    m_dsa.Initialize(p_number, q_number, g_number);
#endif // USE_CRYPTOPP
}

xr_dsa::~xr_dsa() {}
shared_str xr_dsa::sign(private_key_t const& priv_key, u8 const* data, u32 const data_size)
{
#ifdef USE_CRYPTOPP
    CryptoPP::Integer exp(priv_key.m_value, sizeof(priv_key.m_value));
    CryptoPP::DSA::PrivateKey private_key;
    private_key.Initialize(m_dsa, exp);

    std::string signature;
    CryptoPP::DSA::Signer signer(private_key);
    CryptoPP::StringSource(data, data_size, true,
        xr_new<CryptoPP::SignerFilter>(m_rng, signer, xr_new<CryptoPP::StringSink>(signature)) // SignerFilter
    ); // StringSource

    return shared_str(signature.c_str());
#else // USE_CRYPTOPP
    UNUSED(priv_key);
    UNUSED(data);
    UNUSED(data_size);
    return shared_str("(null signature)");
#endif // USE_CRYPTOPP
}

bool xr_dsa::verify(public_key_t const& pub_key, u8 const* data, u32 const data_size, shared_str const& dsign)
{
#ifdef USE_CRYPTOPP
    CryptoPP::Integer exp(pub_key.m_value, sizeof(pub_key.m_value));
    CryptoPP::DSA::PublicKey public_key;
    public_key.Initialize(m_dsa, exp);

    std::string signature(dsign.c_str());
    std::string message((const char*)data, data_size);
    CryptoPP::DSA::Verifier verifier(public_key);
    CryptoPP::SignatureVerificationFilter svf(verifier);
    CryptoPP::StringSource(signature + message, true, xr_new<CryptoPP::Redirector>(svf));

    return svf.GetLastResult();
#else
    UNUSED(pub_key);
    UNUSED(data);
    UNUSED(data_size);
    UNUSED(dsign);
    return true;
#endif // USE_CRYPTOPP
}

#ifdef DEBUG
#ifdef USE_CRYPTOPP
void print_big_number(CryptoPP::Integer big_num, u32 max_columns = 8)
{
    u8 bin_buff[xr_dsa::public_key_length]; // public_key_length is the max
    int bin_size = 0;

    string4096 result_buffer;
    string16 tmp_buff;

    ZeroMemory(bin_buff, sizeof(bin_buff));
    big_num.Encode(bin_buff, xr_dsa::public_key_length);
    bin_size = big_num.ByteCount();

    result_buffer[0] = 0;
    xr_strcat(result_buffer, "\t");
    for (int i = 0; i < bin_size; ++i)
    {
        if (((i % max_columns) == 0) && (i > 0))
        {
            xr_strcat(result_buffer, "\n\t");
        }
        xr_sprintf(tmp_buff, "0x%02x, ", bin_buff[i]);
        xr_strcat(result_buffer, tmp_buff);
    }
    Msg(result_buffer);
};

void xr_dsa::generate_params()
{
    CryptoPP::AutoSeededRandomPool rng;
    CryptoPP::DSA::PrivateKey priv_key;
    priv_key.GenerateRandomWithKeySize(rng, key_bit_length);
    CryptoPP::DSA::PublicKey pub_key;
    pub_key.AssignFrom(priv_key);

    const CryptoPP::DL_GroupParameters_DSA& tmp_dsa_params = priv_key.GetGroupParameters();
    VERIFY(tmp_dsa_params.GetModulus().ByteCount() == public_key_length);
    VERIFY(tmp_dsa_params.GetSubgroupOrder().ByteCount() == private_key_length);
    VERIFY(tmp_dsa_params.GetSubgroupGenerator().ByteCount() == public_key_length);
    VERIFY(pub_key.GetPublicElement().ByteCount() == public_key_length);
    VERIFY(priv_key.GetPrivateExponent().ByteCount() == private_key_length);

    Msg("// DSA params ");

    Msg("u8 const p_number[crypto::xr_dsa::public_key_length] = {");
    print_big_number(tmp_dsa_params.GetModulus());
    Msg("};//p_number");

    Msg("u8 const q_number[crypto::xr_dsa::private_key_length] = {");
    print_big_number(tmp_dsa_params.GetSubgroupOrder());
    Msg("};//q_number");

    Msg("u8 const g_number[crypto::xr_dsa::public_key_length] = {");
    print_big_number(tmp_dsa_params.GetSubgroupGenerator());
    Msg("};//g_number");

    Msg("u8 const public_key[crypto::xr_dsa::public_key_length] = {");
    print_big_number(pub_key.GetPublicElement());
    Msg("};//public_key");

    u8 priv_bin[private_key_length];
    priv_key.GetPrivateExponent().Encode(priv_bin, private_key_length);
    Msg("// Private key:");
    for (int i = 0; i < private_key_length; ++i)
    {
        Msg("	m_private_key.m_value[%d]	= 0x%02x;", i, priv_bin[i]);
    }

    std::string debug_digest = "this is a test";
    std::string debug_bad_digest = "this as a test";

    std::string signature;
    CryptoPP::DSA::Signer signer(priv_key);
    CryptoPP::StringSource(debug_digest, true, xr_new<CryptoPP::SignerFilter>(rng, signer,
                                                   xr_new<CryptoPP::StringSink>(signature)) // SignerFilter
        ); // StringSource

    CryptoPP::DSA::Verifier verifier(pub_key);
    CryptoPP::SignatureVerificationFilter svf(verifier);

    CryptoPP::StringSource(signature + debug_digest, true, xr_new<CryptoPP::Redirector>(svf));
    VERIFY(svf.GetLastResult() == true);

    CryptoPP::StringSource(signature + debug_bad_digest, true, xr_new<CryptoPP::Redirector>(svf));
    VERIFY(svf.GetLastResult() == false);
}
#else // USE_CRYPTOPP
void xr_dsa::generate_params() {}
#endif // USE_CRYPTOPP
#endif //#ifdef DEBUG

} // namespace crypto
