#include "StdAfx.h"
#include "secure_messaging.h"

namespace secure_messaging
{
seed_generator::seed_generator() : m_random(static_cast<s32>(CPU::QPC() & u32(-1))){};
seed_generator::~seed_generator() {}
s32 const seed_generator::genrate() { return m_random.randI(); }
u32 const generate_key(s32 const seed, key_t& result_key)
{
    CRandom tmp_random(seed);
    result_key.m_key_length = static_cast<u32>(tmp_random.randI(key_t::min_key_length, key_t::max_key_length));

#ifdef DEBUG
    char tmp_str[key_t::max_key_length * 8];
    tmp_str[0] = 0;
#endif

    for (u32 i = 0; i < result_key.m_key_length; ++i)
    {
        result_key.m_key[i] = (tmp_random.randI() << 17) | (tmp_random.randI() << 2) | (tmp_random.randI() & 3);
#ifdef DEBUG
        string16 dst_num;
        xr_sprintf(dst_num, "%08x", result_key.m_key[i]);
        xr_strcat(tmp_str, dst_num);
#endif
    }
#ifdef DEBUG
    Msg("* next key generated (seed=0x%08x):%s", seed, tmp_str);
#endif
    return result_key.m_key_length;
}

enum enum_xray_crypt_action // do not add any, identifier !
{
    xr_encrypt = 0x00,
    xr_decrypt
};

inline u32 const xray_crypt(void* buffer, u32 buffer_size, key_t const& sec_key, enum_xray_crypt_action crypt_action)
{
    s32 last_word = -1;
    s32* current_word = static_cast<s32*>(buffer);
    u32 words_count = buffer_size / sizeof(s32);
    u32 key_word_index = 0;
    u32 ret_checksum = 0;
    s32 raw_word = 0;

    for (u32 cwi = 0; cwi < words_count; ++cwi)
    {
        raw_word = *current_word;
        *current_word = (raw_word ^ sec_key.m_key[key_word_index]) ^ last_word;

        if (crypt_action == xr_encrypt)
        {
            last_word = raw_word;
            ret_checksum += raw_word;
        }
        else
        {
            VERIFY(crypt_action == xr_decrypt);
            last_word = *current_word;
            ret_checksum += *current_word;
        }

        ++key_word_index;
        ++current_word;
        if (key_word_index >= sec_key.m_key_length)
        {
            key_word_index = 0;
        }
    }
    u32 rest_bytes = buffer_size - (words_count * sizeof(s32));
    if (!rest_bytes)
        return ret_checksum;

    s32 next_raw_word = 0;
    CopyMemory(&next_raw_word, current_word, rest_bytes);
    raw_word = next_raw_word;
    next_raw_word = (raw_word ^ sec_key.m_key[key_word_index]) ^ last_word;
    CopyMemory(current_word, &next_raw_word, rest_bytes);

    if (crypt_action == xr_encrypt)
    {
        ret_checksum += raw_word;
    }
    else
    {
        VERIFY(crypt_action == xr_decrypt);
        u32 shift_value = (sizeof(s32) - rest_bytes) * 8;
        ret_checksum += u32(next_raw_word) & (u32(-1) >> shift_value);
    }
    return ret_checksum;
}

u32 const encrypt(void* buffer, u32 buffer_size, key_t const& sec_key)
{
    return xray_crypt(buffer, buffer_size, sec_key, xr_encrypt);
}

u32 const decrypt(void* buffer, u32 buffer_size, key_t const& sec_key)
{
    return xray_crypt(buffer, buffer_size, sec_key, xr_decrypt);
}

} // namespace secure_messaging
