//
// Created by vibhanshu on 2024-06-07
//

#include "util/debug.h"
#include "util/cstring.h"

#include <errno.h>
#include <string.h>

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <regex>
#include <atomic>

#ifdef __FAST_MATH__
#error "debug.cpp should never be compiled with -ffast-math or with -ffinite-math-only"
#endif

LLVM_BUILDER_NS_BEGIN

//
// DebugUtil
//
class DebugUtil {
  public:
    static void write_abort_msg(std::ostream& out, const std::string& msg) {
        CString short_msg;
        const char* abort_tag = "ABORT";
        out << "\n\n033[41;37;1m" << abort_tag << ": " << std::flush;
        CString cmsg{msg};
        if (auto end_short_msg = cmsg.find("\n    [func="_cs)) {
            short_msg = cmsg.substr_to(end_short_msg);
            out << short_msg << "\033[0m";
            out << cmsg.substr_from(end_short_msg);
        } else {
            out << msg << "\033[0m";
        }
        out << std::endl;
    }
};

//
// Debug
//
void Debug::M_abort(const std::string& msg) {
    DebugUtil::write_abort_msg(std::cerr, msg);
#ifndef LLVM_BUILDER_RELEASE
    ::abort();
#else
    ::exit(1);
#endif
    ::abort();
}

void Debug::M_abort(const char* msg) {
    const std::string l_msg{msg == nullptr ? "<no_msg>" : msg};
    M_abort(l_msg);
}

void Debug::abort(const char* msg, const char* trace, const char* func_name) {
    (void)trace;
    (void)func_name;
    Debug::M_abort(msg);
}

void Debug::abort(std::string&& msg, const char* trace, const char* func_name) {
    (void)trace;
    (void)func_name;
    Debug::M_abort(msg);
}

LLVM_BUILDER_NS_END
