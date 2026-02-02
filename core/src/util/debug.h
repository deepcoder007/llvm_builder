//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_UTIL_DEBUG_H
#define CORE_UTIL_DEBUG_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <string>
#include <stdexcept>
#include <cstdint>
#include <sstream>
#include <string>
#pragma GCC diagnostic pop

#include "core/util/defines.h"
#include "meta/simple.h"

#define CORE_CONCAT                                                                                                      \
    ::core::concat_t {                                                                                              \
    }

#define CORE_ABORT(msg) ::core::Debug::abort(msg, __FILE__ ":" CORE_XSTR(__LINE__), __PRETTY_FUNCTION__);

#define CORE_ALWAYS_ASSERT_CONTEXT(context, cond)                                                                        \
    {                                                                                                                  \
        if (CORE_UNLIKELY(not(cond))) {                                                                                  \
            CORE_ABORT(context "-ASSERT: " #cond "::" __FILE__ ":" CORE_XSTR(__LINE__))                                    \
        }                                                                                                              \
    }                                                                                                                  \
    /**/

#define CORE_ALWAYS_ASSERT_CONTEXT_INFO(context, cond, info)                                                             \
    {                                                                                                                  \
        if (CORE_UNLIKELY(not(cond))) {                                                                                  \
            CORE_ABORT(CORE_CONCAT << context "-ASSERT: " << #cond "::" << __FILE__ << ":" << CORE_XSTR(__LINE__)            \
                               << "\n              log:" << info);                                                     \
        }                                                                                                              \
    }                                                                                                                  \
    /**/

#define CORE_ALWAYS_ASSERT(cond)            CORE_ALWAYS_ASSERT_CONTEXT("ALWAYS", cond)
#define CORE_ALWAYS_ASSERT_INFO(cond, info) CORE_ALWAYS_ASSERT_CONTEXT_INFO("ALWAYS", cond, info)

#define _CORE_ASSERT_TYPE(lhs, rhs)                                                                                      \
    {                                                                                                                  \
        using lhs_t = remove_cvr_t<decltype(lhs)>;                                                               \
        using rhs_t = remove_cvr_t<decltype(rhs)>;                                                               \
        static_assert(std::is_same_v<lhs_t, rhs_t> or std::is_convertible_v<lhs_t, rhs_t>                              \
                      or std::is_convertible_v<rhs_t, lhs_t>);                                                         \
    }
/**/

#define _CORE_ALWAYS_ASSERT_OP(lhs, rhs, op)                                                                             \
    {                                                                                                                  \
        _CORE_ASSERT_TYPE(lhs, rhs)                                                                                      \
        CORE_ALWAYS_ASSERT_INFO(lhs op rhs,                                                                              \
                              CORE_CONCAT << "Found value:" << #lhs << "=" << lhs << ", " << #rhs << "=" << rhs);        \
    }

#define CORE_ALWAYS_ASSERT_EQ(lhs, rhs)              _CORE_ALWAYS_ASSERT_OP(lhs, rhs, ==)
#define CORE_ALWAYS_ASSERT_NOT_EQ(lhs, rhs)          _CORE_ALWAYS_ASSERT_OP(lhs, rhs, !=)
#define CORE_ALWAYS_ASSERT_LESS_THAN(lhs, rhs)       _CORE_ALWAYS_ASSERT_OP(lhs, rhs, <)
#define CORE_ALWAYS_ASSERT_LESS_THAN_EQ(lhs, rhs)    _CORE_ALWAYS_ASSERT_OP(lhs, rhs, <=)
#define CORE_ALWAYS_ASSERT_GREATER_THAN(lhs, rhs)    _CORE_ALWAYS_ASSERT_OP(lhs, rhs, >)
#define CORE_ALWAYS_ASSERT_GREATER_THAN_EQ(lhs, rhs) _CORE_ALWAYS_ASSERT_OP(lhs, rhs, >=)

#ifndef CORE_RELEASE
#define CORE_ASSERT(cond)            CORE_ALWAYS_ASSERT_CONTEXT("DEBUG", cond)
#define CORE_ASSERT_INFO(cond, info) CORE_ALWAYS_ASSERT_CONTEXT_INFO("DEBUG", cond, info)
#define CORE_DEBUG(x)                x
#else
#define CORE_ASSERT(cond)
#define CORE_ASSERT_INFO(cond, info)
#define CORE_DEBUG(x)
#endif

CORE_NS_BEGIN

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
    CORE_INLINE_ART
    static uint64_t rdtsc() {
        uint64_t rax, rdx, aux;
        __asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx), "=c"(aux) : :);
        return (rdx << 32u) + rax;
    }

  private:
    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void M_abort(const char* msg);

    [[noreturn]] [[gnu::noinline]] [[gnu::cold]] static void M_abort(const std::string& msg);
};

CORE_NS_END

CORE_META_NS_BEGIN

template <typename src_t>
struct shrink_t {
  private:
    static_assert(std::is_arithmetic_v<src_t>);
    static_assert(not std::is_reference_v<src_t>);
    const src_t& m_x;

  public:
    explicit shrink_t(const src_t& x) : m_x{x} {
    }
    template <typename result_t,
              typename = std::enable_if_t<std::is_same_v<result_t, src_t> or std::is_arithmetic_v<result_t>>>
    operator result_t() const {
        if constexpr (std::is_same_v<result_t, src_t>) {
            return m_x;
        } else {
            static_assert(std::is_arithmetic_v<result_t>);
            static_assert(not std::is_reference_v<src_t>);
            const result_t r = static_cast<result_t>(m_x);
            if constexpr (std::is_integral_v<src_t> and std::is_integral_v<result_t>) {
                if constexpr (std::is_signed_v<src_t> and std::is_unsigned_v<result_t>) {
                    CORE_ASSERT(M_check(m_x >= 0, r, "cannot shrink to unsigned when src_value is negative"));
                }
                if constexpr (std::is_unsigned_v<src_t> and std::is_signed_v<result_t>) {
                    CORE_ASSERT(M_check(r >= 0, r, "shrink led to sign-flip"));
                }
            }
            CORE_ASSERT(M_check(static_cast<src_t>(r) == m_x, r, "conversion was not lossless"));
            return r;
        }
    }

  private:
#ifndef CORE_RELEASE
    template <typename result_t>
    bool M_check(bool cond, const result_t& r, const char* errmsg) const {
        if (CORE_LIKELY(cond)) {
            return true;
        } else {
            M_abort(errmsg, r);
        }
    }
    template <typename result_t>
    [[noreturn]]
    void M_abort(const char* errmsg, const result_t& r) const;
#endif
};

#ifndef CORE_RELEASE
template <typename src_t>
template <typename result_t>
[[noreturn]]
void shrink_t<src_t>::M_abort(const char* errmsg, const result_t& r) const {
    if constexpr (std::is_integral_v<src_t>) {
        using signed_src_t = std::make_signed_t<src_t>;
        const auto diff = static_cast<signed_src_t>(static_cast<src_t>(static_cast<src_t>(r) - m_x));
        CORE_ABORT(CORE_CONCAT << (errmsg ? errmsg : "<errmsg is corrupt>") << "; src_value=" << m_x << "; dst_value=" << r
                           << "; diff=" << diff);
    } else {
        const auto diff = static_cast<src_t>(r) - m_x;
        CORE_ABORT(CORE_CONCAT << (errmsg ? errmsg : "<errmsg is corrupt>") << "; src_value=" << m_x << "; dst_value=" << r
                           << "; diff=" << diff);
    }
}
#endif

template <typename R = implicit_t, typename U>
auto shrink(const U& x) {
    if constexpr (std::is_same_v<R, implicit_t>) {
        return shrink_t<U>{x};
    } else {
        return static_cast<R>(shrink_t<U>{x});
    }
}

CORE_META_NS_END

#endif
