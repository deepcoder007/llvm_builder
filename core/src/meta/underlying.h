//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_META_UNDERLYING_H
#define CORE_META_UNDERLYING_H

#include "basic_meta.h"

CORE_META_NS_BEGIN

template <typename T, typename = void>
struct _has_explicit_underlying_type : std::false_type {};

template <typename T>
struct _has_explicit_underlying_type<T, std::void_t<typename T::underlying_type>> : std::true_type {};

template <typename T>
inline constexpr bool _has_explicit_underlying_type_v = _has_explicit_underlying_type<T>::value;

template <typename T>
auto _to_underlying_type(const T&) {
    if constexpr (std::is_enum_v<T>) {
        return type_c<std::underlying_type_t<T>>{};
    } else if constexpr (_has_explicit_underlying_type_v<T>) {
        return type_c<typename T::underlying_type>{};
    } else {
        return invalid_type_c{};
    }
}

template <typename T>
struct underlying : decltype(_to_underlying_type(std::declval<T>())) {};

template <typename T>
using underlying_t = typename underlying<T>::type;

template <typename T>
constexpr auto to_underlying(T&& x) {
    using R = underlying_t<std::remove_reference_t<T>>;
    return static_cast<R>(std::forward<T>(x));
}

template <typename T, typename = void>
struct has_to_underlying : std::false_type {};

template <typename T>
struct has_to_underlying<T, std::void_t<decltype(to_underlying(std::declval<T>()))>> : std::true_type {};

template <typename T>
inline constexpr bool has_to_underlying_v = has_to_underlying<T>::value;

template <typename T, typename = void>
struct has_underlying_type : std::false_type {};

template <typename T>
struct has_underlying_type<T, std::void_t<underlying_t<T>>> : std::true_type {};

template <typename T>
inline constexpr bool has_underlying_type_v = has_underlying_type<T>::value;

CORE_META_NS_END

#endif
