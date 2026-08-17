#ifndef UTILS_HPP
#define UTILS_HPP

#include "config.hpp"

#include <cstdint>
#include <print>

#if defined(_MSC_VER)
    #include <__MSVC_Int128.hpp>
	using u128 = std::_Unsigned128;
    using i128 = std::_Signed128;
#elif defined(__GNUC__)
	using u128 = __uint128_t;
    using i128 = __int128_t;
#else
    #error "no 128-bit type found"
#endif

using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;

#ifdef ENABLE_LOGGING
#ifdef __GNUC__
#define debug_log(fmt, ...) std::println("{}(): " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)
#define error_log(fmt, ...) std::println(stderr, "{}(): " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define debug_log(fmt, ...) std::println("{}(): " fmt, __func__, __VA_ARGS__)
#define error_log(fmt, ...) std::println(stderr, "{}(): " fmt, __func__, __VA_ARGS__)
#endif
#else
#define debug_log(...)
#define error_log(...)
#endif

#endif