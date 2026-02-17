//
// Created by vibhanshu on 2024-06-07
//

#include "string_util.h"
#include "util/cstring.h"

LLVM_BUILDER_NS_BEGIN

const std::string StringUtil::s_empty;

void separator_t::print(std::ostream& os) const {
    if (not m_is_first) {
        os << m_sep;
    }
    m_is_first = false;
}


LLVM_BUILDER_NS_END
