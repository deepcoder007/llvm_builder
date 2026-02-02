//
// Created by vibhanshu on 2024-06-07
//

#include "string_util.h"
#include "llvm_builder/util/cstring.h"

LLVM_BUILDER_NS_BEGIN

const std::string StringUtil::s_empty;

void separator_t::print(std::ostream& os) const {
    if (not m_is_first) {
        os << m_sep;
    }
    m_is_first = false;
}

uint32_t StringUtil::print_multiline(std::ostream& os,
                                     const std::string& msg,
                                     const std::string& prefix,
                                     const std::string& indentation_str) {
    std::vector<CString> l_split_lines = CString{msg}.split('\n');
    uint32_t num_lines = 0;
    separator_t sep{indentation_str};
    os << prefix;
    for (CString chunk : l_split_lines) {
        if (num_lines != 0) {
            os << '\n';
        }
        os << sep << chunk;
        ++num_lines;
    }
    return num_lines;
}

LLVM_BUILDER_NS_END
