//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include "util/debug.h"
#include "core/meta/noncopyable.h"

#include <sstream>
#include <functional>
#include <optional>

#define CORE_CERR                                                                                                        \
    ::core::cerr_t {                                                                                                \
        __FILE__, __LINE__                                                                                             \
    }
#define CORE_INFO                                                                                                        \
    ::core::info_t {                                                                                                \
        __FILE__, __LINE__                                                                                             \
    }
#define CORE_NOTICE                                                                                                      \
    ::core::notice_t {                                                                                              \
        __FILE__, __LINE__                                                                                             \
    }
#define CORE_WARN                                                                                                        \
    ::core::warn_t {                                                                                                \
        __FILE__, __LINE__                                                                                             \
    }
#define CORE_ERROR                                                                                                       \
    ::core::error_t {                                                                                               \
        __FILE__, __LINE__                                                                                             \
    }
#define CORE_FATAL                                                                                                       \
    ::core::fatal_t {                                                                                               \
        __FILE__, __LINE__                                                                                             \
    }

CORE_NS_BEGIN

class print_decorator_t : public std::ostringstream {
    using BaseT = std::ostringstream;

  public:
    enum class Severity : uint8_t {
        debug,
        info,
        notice,
        warn,
        error,
        fatal
    };

  protected:
    const char* m_file_name = nullptr;
    const int32_t m_line_num = 0;

  protected:
    print_decorator_t() = default;
    print_decorator_t(const char* file_name, int32_t line_num) : m_file_name{file_name}, m_line_num{line_num} {
    }
    print_decorator_t(print_decorator_t&&) = delete;
    print_decorator_t(const print_decorator_t&) = delete;

  public:
    virtual ~print_decorator_t() = default;

  public:
    std::ostringstream& buf() {
        return *this;
    }
    bool empty() const;
    print_decorator_t& to_base() & {
        return *this;
    }
    print_decorator_t&& to_base() && {
        return std::move(*this);
    }

  protected:
    static void M_finish(Severity severity, std::ostringstream&& buf, const char* file_name, int32_t line_num);
};

class cerr_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    cerr_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~cerr_t();
};

class debug_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    debug_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~debug_t();
};

class info_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    info_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~info_t();
};

class notice_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    notice_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~notice_t();
};

class warn_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    warn_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~warn_t();
};

class error_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    error_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~error_t();
};

class fatal_t final : public print_decorator_t {
    using BaseT = print_decorator_t;

  public:
    fatal_t(const char* file_path, int32_t line_num) : BaseT{file_path, line_num} {
    }
    ~fatal_t();
};

CORE_NS_END

#endif
