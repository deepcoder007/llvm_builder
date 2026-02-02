//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_META_BASIC_H
#define CORE_META_BASIC_H

#include "core/util/defines.h"

#include <type_traits>
#include <cstdint>
#include <variant>
#include <new>
#include <version>
#include <atomic>

#define CORE_META_NS()     CORE_NS()::meta
#define CORE_META_NS_BEGIN namespace CORE_META_NS() {
#define CORE_META_NS_END   }

CORE_META_NS_BEGIN

template <typename T>
struct type_c {
    using type = T;
};

struct invalid_type_c {};

template <bool cond, typename A, typename B>
using if_c = std::conditional_t<cond, A, B>;

template <typename T, bool add_const>
using maybe_add_const_t = if_c<add_const, const T, T>;

struct void_t final {};
struct implicit_t final {};
struct empty_t final {};

class hashable_t final {
  private:
    hashable_t() = delete;
};

class address_comparable_t final {
  private:
    address_comparable_t() = delete;
};

class istreamable_t final {
  private:
    istreamable_t() = delete;
};

class Reflectable final {
  private:
    Reflectable() = delete;
};

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename First, typename... Rest>
using first = First;

template <typename T, typename class_t>
struct tuple_has_type;

template <typename T, typename... Us>
struct tuple_has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

template <typename T, typename tuple_t>
inline constexpr bool tuple_has_type_v = tuple_has_type<T, tuple_t>::value;

namespace impl {
template <typename, typename = void>
struct has_type_type : std::false_type {};

template <typename T>
struct has_type_type<T, type_c<typename T::type>> : std::true_type {};
}

template <typename T>
inline constexpr bool is_type_holder_v = impl::has_type_type<T>::value;

template <typename T>
inline constexpr bool is_reflectable_v = std::is_convertible_v<T, const Reflectable&>;

template <typename T>
struct is_atomic : std::false_type {};

template <typename T>
struct is_atomic<std::atomic<T>> : std::true_type {};

#define CORE_SINGLETON_VAR_DEC(cls_t)                                                                                    \
  private:                                                                                                             \
    static cls_t s_singleton;                                                                                          \
/**/
#define CORE_SINGLETON_VAR_DEF(cls_t)                                                                                    \
    cls_t cls_t::s_singleton{};                                                                                        \
/**/
#define CORE_SINGLETON_FUNC(cls_t)                                                                                       \
  public:                                                                                                              \
    CORE_INLINE_ART                                                                                                      \
    static cls_t& singleton() {                                                                                        \
        return s_singleton;                                                                                            \
    }                                                                                                                  \
/**/
#define CORE_SINGLETON_SLOW_VAR_FUNC(cls_t)                                                                              \
  private:                                                                                                             \
    explicit cls_t();                                                                                                  \
                                                                                                                       \
  public:                                                                                                              \
    static cls_t& singleton();                                                                                         \
/**/
#define CORE_SINGLETON_SLOW_VAR_FUNC_IMPL(cls_t)                                                                         \
    cls_t& cls_t::singleton() {                                                                                        \
        static cls_t s_singleton;                                                                                      \
        return s_singleton;                                                                                            \
    }                                                                                                                  \
    /**/

#define CORE_COMPARABLE(cls_t, field)                                                                                    \
  public:                                                                                                              \
    CORE_INLINE_ART                                                                                                      \
    bool operator==(const cls_t& rhs) const {                                                                          \
        return this->field == rhs.field;                                                                               \
    }                                                                                                                  \
    CORE_INLINE_ART                                                                                                      \
    bool operator!=(const cls_t& rhs) const {                                                                          \
        return this->field != rhs.field;                                                                               \
    }                                                                                                                  \
    /**/

#define CORE_ARITHEMATIC(cls_t, field)                                                                                   \
  public:                                                                                                              \
    CORE_INLINE_ART                                                                                                      \
    bool operator<(const cls_t& rhs) const {                                                                           \
        return this->field < rhs.field;                                                                                \
    }                                                                                                                  \
    CORE_INLINE_ART                                                                                                      \
    bool operator>(const cls_t& rhs) const {                                                                           \
        return this->field > rhs.field;                                                                                \
    }                                                                                                                  \
    CORE_INLINE_ART                                                                                                      \
    bool operator<=(const cls_t& rhs) const {                                                                          \
        return this->field <= rhs.field;                                                                               \
    }                                                                                                                  \
    CORE_INLINE_ART                                                                                                      \
    bool operator>=(const cls_t& rhs) const {                                                                          \
        return this->field >= rhs.field;                                                                               \
    }                                                                                                                  \
    /**/

#ifdef __cpp_lib_hardware_interference_size
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
enum : size_t {
    hardware_constructive_interference_size = std::hardware_constructive_interference_size,
    hardware_destructive_interference_size = std::hardware_destructive_interference_size
};
#pragma GCC diagnostic pop
#else
constexpr size_t hardware_constructive_interference_size = 64;
constexpr size_t hardware_destructive_interference_size = 64;
#endif

CORE_META_NS_END

#endif
