/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#ifndef __CRC_32_H__
#define __CRC_32_H__

#include <string_view>

//////////////////////////////////////////////////////////////////////////
// Macros for pre-processor crc32 conversion
//
// When AZ_CRC("My string") is used by default it will map to hash::crc32("My string").
// We do have a pro-processor program which will precompute the hash for you and
// transform that macro to AZ_CRC("My string",0xabcdef00) this will expand to just 0xabcdef00.
// This will remove completely the "My string" from your executable, it will add it to a database and so on.
// WHen you want to update the string, just change the string.
// If you don't run the precompile step the code should still run fine, except it will be slower,
// the strings will be in the exe and converted on the fly and you can't use the result where you need
// a constant expression.
// For example
// switch(id) {
//  case AZ_CRC("My string",0xabcdef00): {} break; // this will compile fine
//  case AZ_CRC("My string"): {} break; // this will cause "error C2051: case expression not constant"
// }
// So it's you choice what you do, depending on your needs.
//
/// Implementation when we have only 1 param (by default it should be string.
#define AZ_CRC_1(_1) hash::crc32(_1)
/// Implementation when we have 2 params (we use the 2 only)
#define AZ_CRC_2(_1, _2) hash::crc32(hash::u32(_2))

#define AZ_CRC(...) AZ_MACRO_SPECIALIZE(AZ_CRC_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

//! AZ_CRC_CE macro is for enforcing a compile time evaluation of a crc32 value
//! "CE" by the way stands for consteval, which is the C++20 term used to mark immediate functions
//! It can be used for any paramater that is convertible to either the uint32_t or string_view Crc constructor
//! It works by converting using the crc32 constructor to create a temp crc32 and then use the operator uint32_t
//! To convert the parameter into uint32_t. Since this code isn't using  C++20 yet only integral types can be used
//! as non-type-template parameters
//! An example of the use is as below
//! hash::crc32 testValue1 = AZ_CRC_CE("Test1");
//! hash::crc32 testValue2 = AZ_CRC_CE(0x43b3afd1);
#define AZ_CRC_CE(literal_) hash::internal::CompileTimeCrc32<static_cast<hash::u32>(hash::crc32{literal_})>

//////////////////////////////////////////////////////////////////////////

namespace hash
{
class SerializeContext;

/**
 * Class for all of our crc32 types, better than just using ints everywhere.
 */
class crc32
{
public:
    /**
     * Initializes to 0.
     */
    constexpr crc32() : m_value(0) {}

    /**
     * Initializes from an int.
     */
    constexpr crc32(u32 value) : m_value{value} {}

    /**
     * Calculates the value from a string.
     */
    explicit constexpr crc32(std::string_view view);

    /**
     * Calculates the value from a block of raw data.
     */
    crc32(const void* data, size_t size, bool forceLowerCase = false)
        : crc32{reinterpret_cast<const uint8_t*>(data), size, forceLowerCase}
    {
    }
   
    constexpr crc32(const uint8_t* data, size_t size, bool forceLowerCase = false);
    constexpr crc32(const char* data, size_t size, bool forceLowerCase = false);

    constexpr void Add(std::string_view view);
    void Add(const void* data, size_t size, bool forceLowerCase = false);
    constexpr void Add(const uint8_t* data, size_t size, bool forceLowerCase = false);
    constexpr void Add(const char* data, size_t size, bool forceLowerCase = false);

    constexpr operator u32() const { return m_value; }

    constexpr bool operator==(crc32 rhs) const { return (m_value == rhs.m_value); }
    constexpr bool operator!=(crc32 rhs) const { return (m_value != rhs.m_value); }

    constexpr bool operator!() const { return (m_value == 0); }

protected:
    // A constant expression cannot contain a conversion from const-volatile void to any pointer to object type
    // nor can it contain a reinterpret_cast, therefore overloads for const char* and uint8_t are added
    void Set(const void* data, size_t size, bool forceLowerCase = false);
    constexpr void Set(const uint8_t* data, size_t size, bool forceLowerCase = false);
    constexpr void Set(const char* data, size_t size, bool forceLowerCase = false);
    constexpr void Combine(u32 hash, size_t len);

    u32 m_value;
};
} // namespace hash

#include "crc32.inl"

#endif
