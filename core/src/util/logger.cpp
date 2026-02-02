//
// Created by vibhanshu on 2024-06-07
//

#include "logger.h"
#include "string_util.h"
#include "meta/simple.h"

#include <iostream>
#include <vector>
#include <functional>
#include <atomic>

CORE_NS_BEGIN

bool print_decorator_t::empty() const {
    std::ostringstream& buf = meta::remove_const(*this);
    const std::streampos pos = buf.tellp();
    if (pos != 0) {
        return false;
    }
    buf.seekp(0, ios_base::end);
    const bool is_empty = buf.tellp() == 0;
    buf.seekp(pos);
    return is_empty;
}

void print_decorator_t::M_finish(Severity severity, std::ostringstream&& buf, const char* file_name, int32_t line_num) {
    std::ostream& os = std::cout;
    std::ostringstream stamp;
    stamp << "[]";
    CString start_color = ""_cs;
    CString end_color = ""_cs;
    CString tag;
    switch (severity) {
        case Severity::debug:
            tag = "DEBUG"_cs;
            break;
        case Severity::info:
            tag = "INFO"_cs;
            break;
        case Severity::notice:
            start_color = "\033[44;37;1m"_cs;
            tag = "NOTICE"_cs;
            break;
        case Severity::warn:
            start_color = "\033[40;31;1m"_cs;
            tag = "WARN"_cs;
            break;
        case Severity::error:
            start_color = "\033[41;37;1m"_cs;
            tag = "ERROR"_cs;
            break;
        case Severity::fatal:
            start_color = "\033[41;37;1m"_cs;
            tag = "FATAL"_cs;
            break;
        default:
            start_color = "\033[41;37;1m"_cs;
            tag = "UNKNOWN"_cs;
            break;
    }
    if (not start_color.empty()) {
        stamp << start_color << ' ';
        end_color = start_color = "\033[0m"_cs;
    }
    const std::vector<CString> file_name_split = CString::from_pointer(file_name).split('/');
    if (file_name_split.size() > 0) {
        stamp << "[" << file_name_split.back() << ":" << line_num << "][" << tag << "] ";
    } else {
        stamp << "[" << tag << "] ";
    }
    const std::string& stamp_str = stamp.str();
    const std::string msg = buf.str();
    std::string indent_str;
    indent_str.resize(stamp_str.size() - start_color.size(), ' ');
    {
        uint32_t num_lines = StringUtil::print_multiline(os, msg, stamp_str, indent_str);
        os << end_color << '\n';
        if (num_lines >= 2u) {
            os << std::endl;
        } else if (num_lines >= 1u) {
            os << std::flush;
        }
    }
}

cerr_t::~cerr_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::error, std::move(*this), m_file_name, m_line_num);
    }
}

debug_t::~debug_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::debug, std::move(*this), m_file_name, m_line_num);
    }
}

info_t::~info_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::info, std::move(*this), m_file_name, m_line_num);
    }
}

notice_t::~notice_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::notice, std::move(*this), m_file_name, m_line_num);
    }
}

warn_t::~warn_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::warn, std::move(*this), m_file_name, m_line_num);
    }
}

error_t::~error_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::error, std::move(*this), m_file_name, m_line_num);
    }
}

fatal_t::~fatal_t() {
    if (CORE_LIKELY(not BaseT::empty())) {
        M_finish(Severity::fatal, std::move(*this), m_file_name, m_line_num);
    }
}

CORE_NS_END
