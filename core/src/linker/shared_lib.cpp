//
// Created by vibhanshu on 2024-10-10
//

#include "core/linker/shared_lib.h"
#include "util/debug.h"
#include "util/string_util.h"

#include <dlfcn.h>

CORE_LINKER_NS_BEGIN

SharedLib::SharedLib(const std::string& lib_path)
    : m_lib_path{lib_path} {
}

SharedLib::~SharedLib() = default;

void SharedLib::init() {
    CORE_ALWAYS_ASSERT(not is_init());
    m_handle = ::dlopen(m_lib_path.c_str(), RTLD_LAZY);
    void* l_error = ::dlerror();
    CORE_ALWAYS_ASSERT_INFO(l_error == nullptr, CORE_CONCAT << "Lib not found:" << m_lib_path << ", error:" << (char*)l_error);
}

void* SharedLib::get_symbol(const std::string& name) const {
    CORE_ALWAYS_ASSERT(is_init());
    void* symbol = ::dlsym(m_handle, name.c_str());
    void* l_error = ::dlerror();
    CORE_ALWAYS_ASSERT_INFO(l_error == nullptr, CORE_CONCAT << name << " event not found in lib:" << m_lib_path << ", error:" << (char*)l_error);
    CORE_ALWAYS_ASSERT(symbol != nullptr);
    return symbol;
}

CORE_LINKER_NS_END
