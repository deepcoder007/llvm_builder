//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_UTIL_DEFINES_H
#define CORE_UTIL_DEFINES_H

#include <utility>
#include <ostream>
#include <cstdint>

#ifndef CORE_ALIGNED
#define CORE_ALIGNED(align) __attribute__((aligned(align)))
#endif

#ifndef CORE_PACKED
#define CORE_PACKED(align) __attribute__((packed, aligned(align)))
#endif

#ifndef CORE_LIKELY
#define CORE_LIKELY(cond) __builtin_expect(static_cast<bool>(cond), 1)
#endif
#ifndef CORE_UNLIKELY
#define CORE_UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), 0)
#endif

#ifndef CORE_HOT
#define CORE_HOT __attribute__((hot))
#endif
#ifndef CORE_COLD
#define CORE_COLD __attribute__((cold))
#endif

#define CORE_INLINE_ART    [[gnu::always_inline]] [[gnu::artificial]] inline
#define CORE_STARTUP_ONLY  [[gnu::noinline]] [[gnu::cold]]

#define CORE_XSTR2(x)                         #x
#define CORE_XSTR(x)                          CORE_XSTR2(x)
#define CORE_LIST(...)                        __VA_ARGS__

#define _CORE_CONCAT2_AUX(x1, x2)             x1##x2
#define _CORE_CONCAT3_AUX(x1, x2, x3)         x1##x2##x3
#define _CORE_CONCAT4_AUX(x1, x2, x3, x4)     x1##x2##x3##x4
#define _CORE_CONCAT5_AUX(x1, x2, x3, x4, x5) x1##x2##x3##x4##x5

#define CORE_CONCAT2(x1, x2)                  _CORE_CONCAT2_AUX(x1, x2)
#define CORE_CONCAT3(x1, x2, x3)              _CORE_CONCAT3_AUX(x1, x2, x3)
#define CORE_CONCAT4(x1, x2, x3, x4)          _CORE_CONCAT4_AUX(x1, x2, x3, x4)
#define CORE_CONCAT5(x1, x2, x3, x4, x5)      _CORE_CONCAT5_AUX(x1, x2, x3, x4, x5)

#ifdef CORE_RELEASE
#define _DEBUG_SUFFIX1 0
#else
#define _DEBUG_SUFFIX1 1
#endif

#ifdef NDEBUG
#define _DEBUG_SUFFIX2 0
#else
#define _DEBUG_SUFFIX2 1
#endif

#ifdef _GLIBCXX_DEBUG
#define _DEBUG_SUFFIX3 1
#else
#define _DEBUG_SUFFIX3 0
#endif

#ifdef _GLIBCXX_DEBUG_PEDANTIC
#define _DEBUG_SUFFIX4 1
#else
#define _DEBUG_SUFFIX4 0
#endif

#ifdef _GLIBCXX_DEBUG_VECTOR
#define _DEBUG_SUFFIX5 1
#else
#define _DEBUG_SUFFIX5 0
#endif

#define CORE_DEBUG_SUFFIX() CORE_CONCAT5(_DEBUG_SUFFIX1, _DEBUG_SUFFIX2, _DEBUG_SUFFIX3, _DEBUG_SUFFIX4, _DEBUG_SUFFIX5)

#define CORE_NS()     CORE_CONCAT2(core, CORE_DEBUG_SUFFIX())
#define CORE_NS_BEGIN namespace CORE_NS() {
#define CORE_NS_END   }

#define OSTREAM_FRIEND(cls_t)                                                                                          \
    inline friend std::ostream& operator<<(std::ostream& os, const cls_t& x) {                                         \
        x.print(os);                                                                                                   \
        return os;                                                                                                     \
    }                                                                                                                  \
    /**/

#define ISTREAM_FRIEND(cls_t)                                                                                          \
    inline friend std::istream& operator>>(std::istream& is, const cls_t& x) {                                         \
        std::string l_str;                                                                                             \
        is >> l_str;                                                                                                   \
        return cls_t::from_string(l_str);                                                                              \
    }                                                                                                                  \
    operator ::core::meta::istreamable_t&() const;                                                                  \
    /**/

namespace core_types {
using bool_t = bool;
using float32_t = float;
using float64_t = double;
static_assert(sizeof(bool_t) == 1);
static_assert(sizeof(float32_t) == 4);
static_assert(sizeof(float64_t) == 8);

enum : bool {
#ifdef __x86_64__
    c_is_x86_64 = true,
#else
    c_is_x86_64 = false,
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    c_is_little_endian = true
#else
    c_is_little_endian = false
#endif
};
}

#if !__has_include(<unistd.h>)
static_assert(false, "system not posix compliant");
#endif

CORE_NS_BEGIN

using namespace core_types;

CORE_NS_END

namespace core = CORE_NS();

#endif
