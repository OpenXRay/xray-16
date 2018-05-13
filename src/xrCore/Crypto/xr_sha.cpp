#include "stdafx.h"
#include "xr_sha.h"

namespace crypto
{
xr_sha256::xr_sha256() {}
xr_sha256::~xr_sha256() {}
void xr_sha256::start_calculate(u8 const* data, u32 data_size)
{
    ZeroMemory(m_result, sizeof(m_result));
    VERIFY(data_size);
    m_data_src = data;
    m_data_size = data_size;
}

bool xr_sha256::continue_calculate()
{
    u32 const to_calc = m_data_size >= calc_chunk_size ? calc_chunk_size : m_data_size;

    m_sha_ctx.Update(m_data_src, to_calc);

    m_data_src += to_calc;
    m_data_size -= to_calc;

    if (!m_data_size)
    {
        static_assert(digest_length == CryptoPP::SHA1::DIGESTSIZE, "Digest length must be equal to digest size.");
        m_sha_ctx.Final(m_result);
        return true;
    }
    return false;
}

} // namespace crypto
