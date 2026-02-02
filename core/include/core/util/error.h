//
// Created by vibhanshu on 2025-11-13
// 

#ifndef CORE_UTIL_ERROR_H_
#define CORE_UTIL_ERROR_H_

#include "defines.h"
#include "core/meta/noncopyable.h"
#include "core/ds/fixed_vector.h"

#include <vector>
#include <functional>

#define CODEGEN_LINE(...)                                   \
    ::core::SourceContext::update_top(__FILE__, __LINE__);   \
    __VA_ARGS__;                                                    \
/**/                                                         \

#define CODEGEN_MARK_NEXT_LINE                                   \
    ::core::SourceContext::update_top(__FILE__, __LINE__ + 1);   \
/**/                                                             \

#define CODEGEN_FN_STACK_PUSH                   \
    [[maybe_unused]] uint32_t l_fn_stack_size = SourceContext::push(__FILE__, __LINE__);    \
/**/

#define CODEGEN_FN_STACK_POP                    \
    SourceContext::pop(l_fn_stack_size);        \
/**/                                            \

#define CODEGEN_FN                                                   \
    SourceContext __func__##_source_ctx_val{__FILE__, __LINE__};     \
/**/                                                                 \

#define CODEGEN_PUSH_ERROR(CODE, str) ErrorContext::push_error(ErrorCode::CODE, CORE_CONCAT << str, __FILE__, __LINE__);

#define ERROR_LOG_5   ErrorContext::print(std::cout, 5);

CORE_NS_BEGIN

class Error;
class ErrorContext;

extern const uint32_t l_fn_stack_size;

enum class ErrorCode {
    NO_ERROR,
    UNKNOWN,
    TYPE_ERROR,
    VALUE_ERROR,
    BRANCH_ERROR,
    LINK_SYMBOL,
    CONTEXT,
    MODULE,
    FUNCTION,
    CODE_SECTION,
    JIT,
};

class SourceLoc {
    std::string m_file_name;
    uint32_t m_line_no = std::numeric_limits<uint32_t>::max();
    std::string m_section_name;
public:
    explicit SourceLoc();
    explicit SourceLoc(const std::string& file_name, uint32_t line_no, const std::string& section_name);
    explicit SourceLoc(const std::string& file_name, uint32_t line_no);
    SourceLoc(SourceLoc&&) = default;
    SourceLoc(const SourceLoc&) = default;
    SourceLoc& operator = (const SourceLoc&) = default;
    SourceLoc& operator = (SourceLoc&&) = default;
    ~SourceLoc() = default;
public:
    bool is_valid() const;
    const std::string& file_name() const {
        return m_file_name;
    }
    uint32_t line_num() const {
        return m_line_no;
    }
    const std::string& section_name() const {
        return m_section_name;
    }
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(SourceLoc)
    bool operator == (const SourceLoc& rhs) const;
};

class SourceContext {
    using vec_t = std::vector<SourceLoc>;
    friend class Error;
    friend class ErrorContext;
private:
    uint32_t m_line_num = std::numeric_limits<uint32_t>::max();
    static vec_t s_stack;
public:
    [[gnu::always_inline]]
    SourceContext(const std::string& file_name, uint32_t line_no) {
        m_line_num = push(file_name, line_no);
    }
    [[gnu::always_inline]]
    ~SourceContext() {
        SourceContext::pop(m_line_num);
    }
public:
    [[gnu::noinline]]
    static uint32_t push(const SourceLoc& loc);
    static uint32_t push(const std::string& file_name, uint32_t line_no) {
        return SourceContext::push(SourceLoc{file_name, line_no});
    }
    static void update_top(const std::string& file_name, uint32_t line_no);
    static bool peek(SourceLoc& val);
    static SourceLoc pop();
    static std::vector<SourceLoc> pop(uint32_t pos);
    static void clear(); 
    static void iterate(std::function<void (const SourceLoc&)>&& fn);
};

class Error : meta::noncopyable {
    const ErrorCode m_code = ErrorCode::NO_ERROR;
    const std::string m_msg;
    const std::vector<SourceLoc> m_loc;
public:
    explicit Error(ErrorCode code, const std::string& msg, const std::vector<SourceLoc>& loc);
    explicit Error() = default;
    ~Error() = default;
public:
    bool is_valid() const {
        return m_code != ErrorCode::NO_ERROR;
    }
    ErrorCode code() const {
        return m_code;
    }
    const std::string& msg() const {
        return m_msg;
    }
    void print(std::ostream &os) const;
    OSTREAM_FRIEND(Error);
    static std::string error_code(ErrorCode e);
};

class ErrorContext {
    enum : uint32_t {
        c_max_error_log = 1024
    };
    static inline bool m_has_error = false;
public:
    ErrorContext() = delete;
    ~ErrorContext() = delete;
public:
    static bool has_error() {
        return m_has_error;
    }
    static const Error& last_error();
    static void clear_error();
    [[gnu::cold]] [[gnu::noinline]]
    static const Error& push_error(ErrorCode code, const std::string& msg, const std::string& file_name, uint32_t line_no);
    static void print(std::ostream& os, uint32_t num_max_entries = std::numeric_limits<uint32_t>::max());
private:
    static fixed_vector<Error>& M_error_stack();
};

CORE_NS_END

#endif // CORE_UTIL_ERROR_H_
