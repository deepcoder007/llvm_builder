//
// Created by vibhanshu on 2024-10-10
//

#ifndef CORE_LINKER_SHARED_LIB_H_
#define CORE_LINKER_SHARED_LIB_H_

#include "defines.h"
#include "core/meta/noncopyable.h"

CORE_LINKER_NS_BEGIN

class SharedLib : meta::noncopyable {
    const std::string m_lib_path;
    void* m_handle = nullptr;
public:
    explicit SharedLib(const std::string& lib_path);
    ~SharedLib();
public:
    bool is_init() const {
        return m_handle != nullptr;
    }
    void init();
    void* get_symbol(const std::string& name) const;
    template <typename T>
    T* get_typed_symbol(const std::string& name) const {
        return reinterpret_cast<T*>(get_symbol(name));
    }
};

CORE_LINKER_NS_END

#endif // SHARED_LIB_H_
