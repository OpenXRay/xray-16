#include "stdafx.h"
#include "trivial_encryptor.h"

trivial_encryptor g_trivial_encryptor;

class random32
{
    u32 m_seed;

public:
    random32() = delete;

    random32(const u32& seed)
    {
        m_seed = seed;
    }

    u32 random(const u32& range)
    {
        m_seed = 0x08088405 * m_seed + 1;
        return (u32(u64(m_seed) * u64(range) >> 32));
    }
};

trivial_encryptor::trivial_encryptor()
    : m_key_russian({ 2048, 20091958, 20031955 }),
      m_key_worldwide({ 1024, 6011979, 24031979 })
{
    initialize(key_flag::worldwide);
}

void trivial_encryptor::initialize(key_flag what)
{
    if (what == key_flag::russian)
        m_key = m_key_russian;
    else if (what == key_flag::worldwide)
        m_key = m_key_worldwide;
    else
        R_ASSERT(!"Unknown encryption key!");

    m_current_key = what;

    for (u32 i = 0; i < alphabet_size; ++i)
        m_alphabet[i] = (type)i;

    random32 temp(m_key.m_table_seed);

    for (u32 i = 0; i < m_key.m_table_iterations; ++i)
    {
        u32 j = temp.random(alphabet_size);
        u32 k = temp.random(alphabet_size);
        while (j == k)
            k = temp.random(alphabet_size);

        std::swap(m_alphabet[j], m_alphabet[k]);
    }

    for (u32 i = 0; i < alphabet_size; ++i)
        m_alphabet_back[m_alphabet[i]] = (type)i;
}

void trivial_encryptor::encode(pcvoid source, const u32& source_size, pvoid destination, key_flag what /*= key_flag::worldwide*/)
{
    if (what != m_current_key)
    {
        initialize(what);
    }

    random32 temp(m_key.m_encrypt_seed);

    const u8* I = (const u8*)source;
    const u8* E = (const u8*)source + source_size;
    u8* J = (u8*)destination;
    for (; I != E; ++I, ++J)
        *J = m_alphabet[*I] ^ type(temp.random(alphabet_size) & 0xff);
}

void trivial_encryptor::decode(pcvoid source, const u32& source_size, pvoid destination, key_flag what /*= key_flag::worldwide*/)
{
    if (what != m_current_key)
    {
        initialize(what);
    }

    random32 temp(m_key.m_encrypt_seed);

    const u8* I = (const u8*)source;
    const u8* E = (const u8*)source + source_size;
    u8* J = (u8*)destination;
    for (; I != E; ++I, ++J)
        *J = m_alphabet_back[(*I) ^ type(temp.random(alphabet_size) & 0xff)];
}
