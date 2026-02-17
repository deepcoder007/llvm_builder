//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_UTIL_STRING_UTIL_H
#define LLVM_BUILDER_UTIL_STRING_UTIL_H

#include "llvm_builder/defines.h"
#include "util/cstring.h"

LLVM_BUILDER_NS_BEGIN

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
    static const std::string s_empty;
};

LLVM_BUILDER_NS_END

#endif
