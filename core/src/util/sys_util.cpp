//
// Created by vibhanshu on 2024-06-07
//

#include "sys_util.h"
#include "util/debug.h"
#include "logger.h"
#include "core/util/cstring.h"
#include "string_util.h"

#include "meta/simple.h"

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <cerrno>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sched.h>
#include <pthread.h>
#include <thread>

CORE_NS_BEGIN

using meta::add_const;

pid_t SysUtil::pid() {
    static pid_t pid = ::getpid();
    return pid;
}

const std::string& SysUtil::user() {
    static const std::string s_user = []() -> std::string {
        const std::string l_user = SysUtil::getenv("USER"_cs);
        CORE_ALWAYS_ASSERT(not l_user.empty())
        return l_user;
    }();
    return s_user;
}

CString SysUtil::try_getenv(CString name) {
    CORE_ALWAYS_ASSERT(not name.empty());
    CORE_ALWAYS_ASSERT(name.is_c_str());
    const char* r = ::getenv(name.data());
    return CString::from_pointer(r);
}

std::string SysUtil::getenv(CString name) {
    CString value = try_getenv(name);
    if (value.empty()) {
        return StringUtil::s_empty;
    }
    return value.str();
}

std::string SysUtil::getenv(CString name, const std::string& default_value) {
    CString value = try_getenv(name);
    if (value.empty()) {
        return default_value;
    }
    return value.str();
}

std::string SysUtil::getenv(CString name, CString default_value) {
    CString value = try_getenv(name);
    if (value.empty()) {
        return default_value.str();
    }
    return value.str();
}

void SysUtil::overwrite_env(CString name, CString value) {
    CORE_ALWAYS_ASSERT(name.is_c_str());
    CORE_ALWAYS_ASSERT(not name.empty());
    CORE_ALWAYS_ASSERT(value.is_c_str());
    if (not value.empty()) {
        ::setenv(name.data(), value.data(), 1);
        CORE_ALWAYS_ASSERT(errno != ENOMEM);
    } else {
        unset_env(name);
    }
}

void SysUtil::unset_env(CString name) {
    CORE_ALWAYS_ASSERT(name.is_c_str());
    CORE_ALWAYS_ASSERT(not name.empty());
    ::unsetenv(name.data());
    CORE_ALWAYS_ASSERT(errno != ENOMEM);
}

uint32_t SysUtil::num_cpus() {
    static const uint32_t s_num_cpus = std::thread::hardware_concurrency();
    return s_num_cpus;
}

fs::path SysUtil::cache_dir() {
    const std::string l_path = SysUtil::getenv(CString{M_cache_dir_env()}, "."_cs);
    fs::path l_fs_path{l_path};
    CORE_ALWAYS_ASSERT_INFO(fs::is_directory(l_fs_path), CORE_CONCAT << "not a valid directory:" << l_fs_path);
    return l_fs_path;
}

fs::path SysUtil::output_dir() {
    const std::string l_path = SysUtil::getenv(CString{M_output_dir_env()}, "."_cs);
    fs::path l_fs_path{l_path};
    CORE_ALWAYS_ASSERT_INFO(fs::is_directory(l_fs_path), CORE_CONCAT << "not a valid directory:" << l_fs_path);
    return l_fs_path;
}

CORE_NS_END
