///////////////////////////////////////////////////////////////////////////
/// @file   fast_lc16.hpp
/// @author Sultan Uramaev (Xottab_DUTY)
/// @brief  Based on FastRandom class extracted from Intel TBB
///         which is authored by Intel Corporation
///         and licenced by Apache licence
///////////////////////////////////////////////////////////////////////////
#pragma once

namespace random
{
//! A fast random number generator.
/** Uses linear congruential method. */
class fast_lc16 final
{
    u32 x, c;

public:
    using result_type   =   u16;

    static
    constexpr u32 a     =   0x9e3779b1; // a big prime number

public:
    fast_lc16               ( );

    explicit fast_lc16      ( u32 seed_v )
    {
        seed                ( seed_v );
    }

    explicit fast_lc16      ( u64 seed_v )
    {
        seed                ( seed_v );
    }

    fast_lc16               ( void* unique_ptr )
    {
        seed                ( uintptr_t(unique_ptr) );
    }

private:
    template                < int >
    struct int_to_type      { };

public:
    void seed               ( );

    template                < typename T >
    void seed               ( T value )
    {
        seed                ( value, int_to_type<sizeof(value)>() );
    }

    void seed               ( uint64_t value, int_to_type<8> )
    {
        seed                ( uint32_t((value >> 32) + value), int_to_type<4>() );
    }

    void seed               ( uint32_t value, int_to_type<4> )
    {
        // threads use different seeds for unique sequences
        c               =   (value | 1) * 0xba5703f5; // c must be odd, shuffle by a prime number
        x               =   c ^ (value >> 1); // also shuffle x for the first operator() invocation
    }

public:
    result_type operator()  ( )
    {
        result_type const r =   static_cast<result_type>( x >> 16 );
        VERIFY2             ( c & 1, "c must be odd for big rng period" );

        x               =   x * a + c;
        return r;
    }

public:
    [[nodiscard]]
    static
    constexpr result_type   min ( ) noexcept
    {
        return std::numeric_limits< result_type >::min ( );
    }

    [[nodiscard]]
    static
    constexpr result_type   max ( ) noexcept
    {
        return std::numeric_limits< result_type >::max ( );
    }
};
} // namespace random
