/**
 * endian.h - functions to deal with little-endian and big-endian
 *
 * @author Pavel Kryukov <pavel.kryukov@phystech.edu>
 * Copyright 2018 MIPT-MIPS project
 */
 
// protection from multi-include
#ifndef ENDIAN_H
#define ENDIAN_H

#include "macro.h"
#include <array>
#include <climits>

// Replace it with std::endian if C++20 is shipped
enum class Endian
{
#ifdef _WIN32
    little = 0,
    big    = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

template<typename T>
static inline constexpr T pack_array_le( std::array<Byte, bytewidth<T>> array) noexcept
{
    T value = 0;
    for ( size_t i = 0; i < array.size(); ++i) // NOLINTNEXTLINE
        value |= unsign_t<T>( array[i]) << (i * CHAR_BIT);

    return value;
}

template<typename T>
static inline constexpr T pack_array_be( std::array<Byte, bytewidth<T>> array) noexcept
{
    T value = 0;
    for ( size_t i = 0; i < array.size(); ++i) // NOLINTNEXTLINE
        value |= unsign_t<T>( array[i]) << ((array.size() - i - 1) * CHAR_BIT);

    return value;
}

template<typename T>
static inline auto unpack_array_le( T value) noexcept
{
    std::array<Byte, bytewidth<T>> array; // NOLINT(hicpp-member-init) We init it later
    for ( size_t i = 0; i < array.size(); ++i) // NOLINTNEXTLINE
        array[i] = Byte( value >> ( i * CHAR_BIT));

    return array;
}

template<typename T>
static inline auto unpack_array_be( T value) noexcept
{
    std::array<Byte, bytewidth<T>> array; // NOLINT(hicpp-member-init) We init it later
    for ( size_t i = 0; i < array.size(); ++i) // NOLINTNEXTLINE
        array[i] = Byte( value >> ((array.size() - i - 1) * CHAR_BIT));

    return array;
}

template<typename T, Endian e>
static inline auto unpack_array( T value)
{
    if constexpr (e == Endian::little)
        return unpack_array_le<T>( value);

    return unpack_array_be<T>( value);
}

template<typename T, Endian e>
static inline constexpr auto pack_array( std::array<Byte, bytewidth<T>> array)
{
    if constexpr (e == Endian::little)
        return pack_array_le<T>( array);

    return pack_array_be<T>( array);
}    
    
#endif // ENDIAN_H
