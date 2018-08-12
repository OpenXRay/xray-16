#pragma once
#ifndef TYPES_H
#define TYPES_H
#include <cstdint>
#include <limits>

// Type defs
using s8 = std::int8_t;
using u8 = std::uint8_t;

using s16 = std::int16_t;
using u16 = std::uint16_t;

using s32 = std::int32_t;
using u32 = std::uint32_t;

using s64 = std::int64_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

using pstr = char*;
using pcstr = const char*;

// Type limits
template <typename T>
constexpr auto type_max = std::numeric_limits<T>::max();

template <typename T>
constexpr auto type_min = -std::numeric_limits<T>::max();

template <typename T>
constexpr auto type_zero = std::numeric_limits<T>::min();

template <typename T>
constexpr auto type_epsilon = std::numeric_limits<T>::epsilon();

constexpr int int_max = type_max<int>;
constexpr int int_min = type_min<int>;
constexpr int int_zero = type_zero<int>;

constexpr float flt_max = type_max<float>;
constexpr float flt_min = type_min<float>;
constexpr float flt_zero = type_zero<float>;
constexpr float flt_eps = type_epsilon<float>;

#undef FLT_MAX
#undef FLT_MIN
#define FLT_MAX flt_max
#define FLT_MIN flt_min

constexpr double dbl_max = type_max<double>;
constexpr double dbl_min = type_min<double>;
constexpr double dbl_zero = type_zero<double>;
constexpr double dbl_eps = type_epsilon<double>;

constexpr int max_path = 260;

using string16 = char[16];
using string32 = char[32];
using string64 = char[64];
using string128 = char[128];
using string256 = char[256];
using string512 = char[512];
using string1024 = char[1024];
using string2048 = char[2048];
using string4096 = char[4096];

using string_path = char[2 * max_path];

// XXX: Replace __interface with either struct or class. MS defines it as struct for COM, but this project is C++.
#if defined(WINDOWS)
#define xr_pure_interface __interface
#elif defined(LINUX)
#define xr_pure_interface struct
#endif
#endif
