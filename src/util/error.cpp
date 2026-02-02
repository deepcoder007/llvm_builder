//
// Created by vibhanshu on 2025-11-13
// 

#include "llvm_builder/util/error.h"

LLVM_BUILDER_NS_BEGIN

const uint32_t l_fn_stack_size = std::numeric_limits<uint32_t>::max();

//
// SourceLoc
//
SourceLoc::SourceLoc() = default;

SourceLoc::SourceLoc(const std::string& file_name, uint32_t line_no, const std::string& section_name)
  : m_file_name{file_name}, m_line_no{line_no}, m_section_name{section_name} {
}

SourceLoc::SourceLoc(const std::string& file_name, uint32_t line_no)
  : m_file_name{file_name}, m_line_no{line_no} {
}

bool SourceLoc::is_valid() const {
    return not m_file_name.empty();
}

void SourceLoc::print(std::ostream& os) const {
    os << "{File:" << m_file_name << ":" << m_line_no << "}";
}

bool SourceLoc::operator == (const SourceLoc& rhs) const {
    if (not is_valid() and not rhs.is_valid()) {
        return true;
    } else {
        return m_file_name == rhs.m_file_name
          and m_line_no == rhs.m_line_no;
    }
}

typename SourceContext::vec_t SourceContext::s_stack{};

//
// SourceContext
// 
[[gnu::noinline]]
uint32_t SourceContext::push(const SourceLoc& loc) {
    if (loc.is_valid()) {
        s_stack.emplace_back(loc);
    }
    return (uint32_t)s_stack.size();
}

void SourceContext::update_top(const std::string& file_name, uint32_t line_no) {
    vec_t& l_vec = s_stack;
    if (l_vec.empty()) {
        l_vec.emplace_back(file_name, line_no);
    } else {
        l_vec.back() = SourceLoc{file_name, line_no};
    }
}

bool SourceContext::peek(SourceLoc& val) {
    vec_t& l_vec = s_stack;
    if (not l_vec.empty()) {
        val = l_vec.back();
        return true;
    } else {
        return false;
    }
}

SourceLoc SourceContext::pop() {
    vec_t& l_vec = s_stack;
    SourceLoc l_ret{};
    if (not l_vec.empty()) {
        l_ret = l_vec.back();
        l_vec.pop_back();
    }
    return l_ret;
}

std::vector<SourceLoc> SourceContext::pop(uint32_t pos) {
    vec_t& l_vec = s_stack;
    std::vector<SourceLoc> l_ret{};
    while (l_vec.size() >= pos) {
        l_ret.emplace_back(l_vec.back());
        l_vec.pop_back();
    }
    return l_ret;
}

void SourceContext::clear() {
    s_stack.clear();
}

void SourceContext::iterate(std::function<void (const SourceLoc&)>&& fn) {
    vec_t& l_vec = s_stack;
    for (const SourceLoc& loc : l_vec) {
        fn(loc);
    }
}

//
// Error
//
Error::Error(ErrorCode code, const std::string &msg, const std::vector<SourceLoc>& loc)
    : m_code{code}, m_msg{msg}, m_loc{loc} {
}

void Error::print(std::ostream &os) const {
    os << "{Error:" << error_code(m_code) << ":" << m_msg << ":[" << std::endl;
    for (const SourceLoc& l : m_loc) {
        os << "            ," << l << std::endl;
    }
    os << "    ]}";
}

std::string Error::error_code(ErrorCode e) {
    switch (e) {
    case ErrorCode::NO_ERROR:
        return "NO_ERRROR";
        break;
    case ErrorCode::UNKNOWN:
        return "UNKNOWN";
        break;
    case ErrorCode::TYPE_ERROR:
        return "TYPE_ERROR";
        break;
    case ErrorCode::VALUE_ERROR:
        return "VALUE_ERROR";
        break;
    case ErrorCode::BRANCH_ERROR:
        return "BRANCH_ERROR";
        break;
    case ErrorCode::LINK_SYMBOL:
        return "LINK_SYMBOL";
        break;
    case ErrorCode::CONTEXT:
        return "CONTEXT";
        break;
    case ErrorCode::MODULE:
        return "MODULE";
        break;
    case ErrorCode::FUNCTION:
        return "FUNCTION";
        break;
    case ErrorCode::CODE_SECTION:
        return "CODE_SECTION";
        break;
    case ErrorCode::JIT:
        return "JIT";
        break;
    }
    return "<INVALID_ERROR_CODE>";
}

//
// ErrorContext
//
const Error& ErrorContext::last_error() {
    if (has_error()) {
        return M_error_stack().back();
    } else {
        static const Error s_err{ErrorCode::NO_ERROR, "", std::vector<SourceLoc>{}};
        return s_err;
    }
}

void ErrorContext::clear_error() {
    m_has_error = false;
    M_error_stack().clear();
}

const Error& ErrorContext::push_error(ErrorCode code, const std::string& msg, const std::string& file_name, uint32_t line_no) {
    SourceContext::push(file_name, line_no);
    m_has_error = true;
    return M_error_stack().emplace_back(code, msg, SourceContext::s_stack);
}

void ErrorContext::print(std::ostream &os, uint32_t num_max_entries) {
    uint32_t l_count = 0;
    os << "Error:" << std::endl;
    for (const Error &l_err : M_error_stack()) {
        os << "\t" << l_err << std::endl;
        ++l_count;
        if (l_count > num_max_entries) {
            break;
        }
    }
}

auto ErrorContext::M_error_stack() -> std::vector<Error> & {
    static std::vector<Error> s_error_stack;
    return s_error_stack;
}

LLVM_BUILDER_NS_END


