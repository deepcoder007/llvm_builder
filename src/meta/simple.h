//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_META_SIMPLE_H_
#define LLVM_BUILDER_META_SIMPLE_H_

#include "basic_meta.h"

LLVM_BUILDER_META_NS_BEGIN

inline constexpr bool bool_if(bool a, bool b) {
    return (not a) or b;
}

template <typename T>
inline decltype(auto) remove_const(T&& x) {
    using T2 = std::remove_reference_t<T>;
    if constexpr (std::is_pointer_v<T2>) {
        using T3 = std::remove_pointer_t<T2>;
        static_assert(std::is_const_v<T3>);
        using T4 = std::remove_const_t<T3>;
        return const_cast<T4*>(x);
    } else {
        static_assert(std::is_const_v<T2>);
        using T3 = std::remove_const_t<T2>;
        return const_cast<T3&>(x);
    }
}

LLVM_BUILDER_META_NS_END

#endif
