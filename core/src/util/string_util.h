//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_UTIL_STRING_UTIL_H
#define CORE_UTIL_STRING_UTIL_H

#include "core/util/defines.h"
#include "core/util/cstring.h"

CORE_NS_BEGIN

// TODO{vibhanshu}: remove usage of this class, instead return a list of strings
//      in a hierarchy which front-end cna print accordingly
class separator_t {
    const std::string m_sep;
    mutable bool m_is_first = true;
  public:
    explicit separator_t(CString s) : m_sep{ s.str() } {
    }
    explicit separator_t(const std::string& s) : m_sep{ s } {
    }
  public:
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(separator_t)
};

class StringUtil {
  public:
    static uint32_t print_multiline(std::ostream& os, const std::string& msg, const std::string& prefix,
                                    const std::string& indentation_str);

  public:
    static const std::string s_empty;
};

CORE_NS_END

#endif
