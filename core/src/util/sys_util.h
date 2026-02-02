//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_SYS_UITL_H
#define CORE_SYS_UITL_H

#include "core/util/cstring.h"

#include <functional>
#include <filesystem>

CORE_NS_BEGIN

// TODO{vibhanshu}: remove char* and use CString instead

namespace fs = std::filesystem;

struct SysUtil {
  public:
    static uint32_t num_cpus();
    static pid_t pid();
    static const std::string& user();
    static CString try_getenv(CString name);
    static CString try_getenv(const std::string& name) {
        return try_getenv(CString{name});
    }
    static std::string getenv(CString name);
    static std::string getenv(const std::string& name) {
        return getenv(CString{name});
    }
    static std::string getenv(CString name, const std::string& default_value);
    static std::string getenv(CString name, CString default_value);
    static std::string getenv(const char* name) = delete;
    static void overwrite_env(CString name, CString value);
    static void unset_env(CString name);
    static fs::path cache_dir();
    static fs::path output_dir();

  private:
    static const std::string& M_cache_dir_env() {
        static std::string s_cache_dir = "CACHE_DIR";
        return s_cache_dir;
    }
    static const std::string& M_output_dir_env() {
        static std::string s_output_dir = "OUTPUT_DIR";
        return s_output_dir;
    }
};

CORE_NS_END

#endif
