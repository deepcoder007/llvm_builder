//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_DEFINES_H
#define LLVM_BUILDER_DEFINES_H

#include <ostream>
#include <cstdint>

#ifndef LLVM_BUILDER_UNLIKELY
#define LLVM_BUILDER_UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), 0)
#endif

#define LLVM_BUILDER_INLINE_ART    [[gnu::always_inline]] [[gnu::artificial]] inline

#define LLVM_BUILDER_XSTR2(x)                         #x
#define LLVM_BUILDER_XSTR(x)                          LLVM_BUILDER_XSTR2(x)

#define _LLVM_BUILDER_CONCAT2_AUX(x1, x2)             x1##x2
#define _LLVM_BUILDER_CONCAT5_AUX(x1, x2, x3, x4, x5) x1##x2##x3##x4##x5

#define LLVM_BUILDER_CONCAT2(x1, x2)                  _LLVM_BUILDER_CONCAT2_AUX(x1, x2)
#define LLVM_BUILDER_CONCAT5(x1, x2, x3, x4, x5)      _LLVM_BUILDER_CONCAT5_AUX(x1, x2, x3, x4, x5)

#ifdef LLVM_BUILDER_RELEASE
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

#define LLVM_BUILDER_DEBUG_SUFFIX() LLVM_BUILDER_CONCAT5(_DEBUG_SUFFIX1, _DEBUG_SUFFIX2, _DEBUG_SUFFIX3, _DEBUG_SUFFIX4, _DEBUG_SUFFIX5)

#define LLVM_BUILDER_NS()     LLVM_BUILDER_CONCAT2(llvm_builder, LLVM_BUILDER_DEBUG_SUFFIX())
#define LLVM_BUILDER_NS_BEGIN namespace LLVM_BUILDER_NS() {
#define LLVM_BUILDER_NS_END   }

#define OSTREAM_FRIEND(cls_t)                                                                                          \
    inline friend std::ostream& operator<<(std::ostream& os, const cls_t& x) {                                         \
        x.print(os);                                                                                                   \
        return os;                                                                                                     \
    }                                                                                                                  \
    /**/

namespace llvm_builder_types {
using bool_t = bool;
using float32_t = float;
using float64_t = double;
static_assert(sizeof(bool_t) == 1);
static_assert(sizeof(float32_t) == 4);
static_assert(sizeof(float64_t) == 8);
}

#if !__has_include(<unistd.h>)
static_assert(false, "system not posix compliant");
#endif

LLVM_BUILDER_NS_BEGIN

using namespace llvm_builder_types;

LLVM_BUILDER_NS_END

namespace llvm_builder = LLVM_BUILDER_NS();

#define FOR_EACH_LLVM_INT_TYPE(macro)                \
    macro(int8)                                      \
    macro(int16)                                     \
    macro(int32)                                     \
    macro(int64)                                     \
/**/                                                 \

#define FOR_EACH_LLVM_UINT_TYPE(macro)               \
    macro(uint8)                                     \
    macro(uint16)                                    \
    macro(uint32)                                    \
    macro(uint64)                                    \
/**/                                                 \

#define FOR_EACH_LLVM_FLOAT_TYPE(macro)              \
    macro(float32)                                   \
    macro(float64)                                   \
/**/                                                 \

#define FOR_EACH_LLVM_TYPE(macro)                    \
    macro(bool)                                      \
    FOR_EACH_LLVM_INT_TYPE(macro)                    \
    FOR_EACH_LLVM_UINT_TYPE(macro)                   \
    FOR_EACH_LLVM_FLOAT_TYPE(macro)                  \
/**/                                                 \

#define FOR_EACH_ARITHEMATIC_OP(macro)               \
    macro(add)                                       \
    macro(sub)                                       \
    macro(mult)                                      \
    macro(div)                                       \
    macro(remainder)                                 \
/**/

#define FOR_EACH_LOGICAL_OP(macro)                   \
    macro(less_than)                                 \
    macro(less_than_equal)                           \
    macro(greater_than)                              \
    macro(greater_than_equal)                        \
    macro(equal)                                     \
    macro(not_equal)                                 \
/**/

#define FOR_EACH_BINARY_OP(macro)                    \
    FOR_EACH_ARITHEMATIC_OP(macro)                   \
    FOR_EACH_LOGICAL_OP(macro)                       \
/**/

#endif // LLVM_BUILDER_DEFINES_H
