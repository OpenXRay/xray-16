#pragma once

struct evp_pkey_st;
struct evp_pkey_ctx_st;

namespace crypto
{
class XRCORE_API xr_dsa
{
public:
    static int const key_bit_length = 1024;
    static int const public_key_length = key_bit_length / 8;
    static int const private_key_length = 20;

    xr_dsa(u8 const p[public_key_length], u8 const q[private_key_length], u8 const g[public_key_length]);
    ~xr_dsa();

    struct private_key_t
    {
        u8 m_value[private_key_length];
    }; // struct private_key_t

    struct public_key_t
    {
        u8 m_value[public_key_length];
    }; // struct public_key_t

    shared_str sign(private_key_t const& priv_key, u8 const* data, u32 const data_size) const;
    bool verify(public_key_t const& pub_key, u8 const* data, u32 const data_size, shared_str const& dsign) const;

private:
    evp_pkey_st* m_key{};
    evp_pkey_ctx_st* m_context{};
}; // class xr_dsa
} // namespace crypto
