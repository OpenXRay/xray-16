#pragma once

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

class XRCORE_API trivial_encryptor
{
    using type = u8;
    using pvoid = void*;
    using pcvoid = const void*;

public:
    static constexpr u32 alphabet_size = u32(1 << (8 * sizeof(type)));

    enum class key_flag
    {
        russian,
        worldwide
    };

private:
    struct key
    {
        u32 m_table_iterations;
        u32 m_table_seed;
        u32 m_encrypt_seed;
    };

    key m_key;

public:
    const key m_key_russian;
    const key m_key_worldwide;

private:
    key_flag m_current_key;

    type m_alphabet[alphabet_size];
    type m_alphabet_back[alphabet_size];

    void initialize(key_flag what);

public:
    trivial_encryptor();

    void encode(pcvoid source, const u32& source_size, pvoid destination, key_flag what = key_flag::worldwide);
    void decode(pcvoid source, const u32& source_size, pvoid destination, key_flag what = key_flag::worldwide);
};

extern XRCORE_API trivial_encryptor g_trivial_encryptor;
