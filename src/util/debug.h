//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_UTIL_DEBUG_H
#define LLVM_BUILDER_UTIL_DEBUG_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <string>
#include <stdexcept>
#include <cstdint>
#include <sstream>
#include <string>
#pragma GCC diagnostic pop

#include "llvm_builder/defines.h"
#include "meta/simple.h"

#define LLVM_BUILDER_CONCAT                                                                                                      \
    ::llvm_builder::concat_t {                                                                                              \
    }

#define LLVM_BUILDER_ABORT(msg) ::llvm_builder::Debug::abort(msg, __FILE__ ":" LLVM_BUILDER_XSTR(__LINE__), __PRETTY_FUNCTION__);

#define LLVM_BUILDER_ALWAYS_ASSERT_CONTEXT(context, cond)                                                                        \
    {                                                                                                                  \
        if (LLVM_BUILDER_UNLIKELY(not(cond))) {                                                                                  \
            LLVM_BUILDER_ABORT(context "-ASSERT: " #cond "::" __FILE__ ":" LLVM_BUILDER_XSTR(__LINE__))                                    \
        }                                                                                                              \
    }                                                                                                                  \
    /**/


#define LLVM_BUILDER_ALWAYS_ASSERT(cond)            LLVM_BUILDER_ALWAYS_ASSERT_CONTEXT("ALWAYS", cond)

#define _LLVM_BUILDER_ASSERT_TYPE(lhs, rhs)                                                                                      \
    {                                                                                                                  \
        using lhs_t = remove_cvr_t<decltype(lhs)>;                                                               \
        using rhs_t = remove_cvr_t<decltype(rhs)>;                                                               \
        static_assert(std::is_same_v<lhs_t, rhs_t> or std::is_convertible_v<lhs_t, rhs_t>                              \
                      or std::is_convertible_v<rhs_t, lhs_t>);                                                         \
    }
/**/

#define _LLVM_BUILDER_ALWAYS_ASSERT_OP(lhs, rhs, op)                                                                             \
    {                                                                                                                            \
        _LLVM_BUILDER_ASSERT_TYPE(lhs, rhs)                                                                                      \
        LLVM_BUILDER_ALWAYS_ASSERT(lhs op rhs);                                                                                  \
    }

#define LLVM_BUILDER_ALWAYS_ASSERT_EQ(lhs, rhs)              _LLVM_BUILDER_ALWAYS_ASSERT_OP(lhs, rhs, ==)
#define LLVM_BUILDER_ALWAYS_ASSERT_NOT_EQ(lhs, rhs)          _LLVM_BUILDER_ALWAYS_ASSERT_OP(lhs, rhs, !=)

#ifndef LLVM_BUILDER_RELEASE
#define LLVM_BUILDER_ASSERT(cond)            LLVM_BUILDER_ALWAYS_ASSERT_CONTEXT("DEBUG", cond)
#define LLVM_BUILDER_DEBUG(x)                x
#else
#define LLVM_BUILDER_ASSERT(cond)
#define LLVM_BUILDER_DEBUG(x)
#endif

LLVM_BUILDER_NS_BEGIN

template <typename T>
struct remove_cvr {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cvr_t = typename remove_cvr<T>::type;

class concat_t {
  private:
    std::ostringstream m_msg;

  public:
    concat_t() = default;
    concat_t(concat_t&&) = default;

  private:
    concat_t(const concat_t&) = delete;

  public:
    template <typename T, typename StreamT>
    static void append_term(StreamT& msg, const T& x) {
        using T2 = remove_cvr_t<std::decay_t<T>>;
        if constexpr (std::is_same_v<T2, uint8_t> or std::is_same_v<T2, int8_t>) {
            static_assert(not std::is_same_v<T2, char>);
            msg << static_cast<int>(x);
        } else if constexpr (std::is_same_v<T2, std::byte>) {
            static_assert(not std::is_same_v<T2, char>);
            msg << "b:" << static_cast<int>(x);
        } else {
            msg << x;
        }
    }
    template <typename T, typename StreamT>
    static void append_term(StreamT& msg, const std::reference_wrapper<T>& a_x) {
        const T& x = a_x;
        append_term(msg, x);
    }
    template <typename T>
    concat_t& operator<<(const T& x) {
        append_term(m_msg, x);
        return *this;
    }
    std::string str() const {
        return m_msg.str();
    }
    operator std::string() const {
        return str();
    }
    void print(std::ostream& os) const {
        os << str();
    }
    OSTREAM_FRIEND(concat_t)
  private:
    void operator=(const concat_t&) = delete;
    void operator=(concat_t&&) = delete;
};

template <typename... Args>
std::string concat(const Args&... args) {
    concat_t ss;
    (ss << ... << args);
    return ss.str();
}

class Debug {
  public:
    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void
    abort(const char* msg, const char* trace, const char* func_name);

    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void
    abort(std::string&& msg, const char* trace, const char* func_name);

  public:
    LLVM_BUILDER_INLINE_ART
    static uint64_t rdtsc() {
        uint64_t rax, rdx, aux;
        __asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(aux) : :);
        return (rdx << 32u) + rax;
    }

  private:
    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void M_abort(const char* msg);

    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void M_abort(const std::string& msg);
};

LLVM_BUILDER_NS_END

#endif
