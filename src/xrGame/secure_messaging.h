#ifndef SECRET_KEY_GENERATOR_INCLUDED
#define SECRET_KEY_GENERATOR_INCLUDED

#include "Common/Noncopyable.hpp"
#include "xrCore/_random.h"

namespace secure_messaging
{
class seed_generator : private Noncopyable
{
public:
    seed_generator();
    ~seed_generator();

    s32 genrate();

private:
    CRandom m_random;
}; // class seed_generator

struct key_t
{
    static u32 const max_key_length = 32; // in bytes
    static u32 const min_key_length = 16;

    u32 m_key_length;
    s32 m_key[max_key_length];
}; // struct key_t

u32 generate_key(s32 const seed, key_t& result_key);

// returns checksum of raw data
u32 encrypt(void* buffer, u32 buffer_size, key_t const& sec_key);
u32 decrypt(void* buffer, u32 buffer_size, key_t const& sec_key);

} // namespace secure_messaging

#endif //#ifndef SECRET_KEY_GENERATOR_INCLUDED
