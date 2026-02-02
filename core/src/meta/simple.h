//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_META_SIMPLE_H_
#define CORE_META_SIMPLE_H_

#include "basic_meta.h"
#include <array>

CORE_META_NS_BEGIN

//
// array info
//
template <typename T>
class array_info;

template <typename T, uint32_t N>
class array_info<T[N]> {
  public:
    using element_type = T;
    enum : uint32_t {
        length = N
    };

  public:
    static element_type& back(T (&arr)[N]) {
        return arr[length - 1u];
    }
};

template <typename T, std::size_t N>
class array_info<std::array<T, N>> {
  public:
    using element_type = T;
    enum : uint32_t {
        length = N
    };

  public:
    static element_type& back(T (&arr)[N]) {
        return arr[length - 1u];
    }
};

template <typename T>
constexpr uint32_t array_length = array_info<T>::length;

template <typename T>
decltype(auto) array_back(T& arr) {
    return array_info<T>::back(arr);
}

template <typename T>
constexpr std::make_unsigned_t<T> make_unsigned(const T& x) {
    static_assert(std::is_integral<T>::value, "sanity check");
    using T2 = std::make_unsigned_t<T>;
    return static_cast<T2>(x);
}

template <typename T>
constexpr std::make_signed_t<T> make_signed(const T& x) {
    static_assert(std::is_integral<T>::value, "sanity check");
    using T2 = std::make_signed_t<T>;
    return static_cast<T2>(x);
}

template <typename T>
struct remove_cvr {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cvr_t = typename remove_cvr<T>::type;

inline constexpr bool bool_if(bool a, bool b) {
    return (not a) or b;
}

template <typename T>
inline decltype(auto) remove_const(T&& x) {
    using T2 = std::remove_reference_t<T>;
    if constexpr (std::is_pointer_v<T2>) {
        using T3 = std::remove_pointer_t<T2>;
        static_assert(std::is_const_v<T3>);
        using T4 = std::remove_const_t<T3>;
        return const_cast<T4*>(x);
    } else {
        static_assert(std::is_const_v<T2>);
        using T3 = std::remove_const_t<T2>;
        return const_cast<T3&>(x);
    }
}

template <typename T>
inline auto remove_xvalue(T&& x) -> std::remove_reference_t<T>& {
    static_assert(not std::is_reference_v<T>);
    return x;
}

template <typename T>
inline const T* add_const(T* x) {
    return x;
}

template <typename T>
inline const T& add_const(T& x) {
    static_assert(not std::is_pointer_v<T>);
    return x;
}

CORE_META_NS_END

#endif
