//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_META_MAKE_DERIVED_H
#define CORE_META_MAKE_DERIVED_H

#include "simple.h"
#include "core/util/debug.h"
#include <memory>

CORE_META_NS_BEGIN

template <typename T>
inline auto implicit_cast(T x) -> T {
    static_assert(std::is_arithmetic_v<T> or std::is_reference_v<T> or std::is_pointer_v<T>);
    return x;
}

template <typename BaseT>
inline auto make_base(BaseT& x) -> BaseT& {
    return x;
}

template <typename DerivedT, typename BaseT>
inline auto make_derived(BaseT& x) -> DerivedT& {
    static_assert(std::is_base_of_v<BaseT, DerivedT>);
    if constexpr (std::is_polymorphic_v<BaseT>) {
        CORE_DEBUG(const DerivedT* derived = dynamic_cast<DerivedT*>(&x));
        CORE_ASSERT(derived != nullptr);
        CORE_ASSERT(derived == &static_cast<DerivedT&>(x));
    }
    return static_cast<DerivedT&>(x);
}

template <typename DerivedT, typename BaseT>
inline auto safe_make_derived(BaseT& x) -> DerivedT& {
    if constexpr (std::is_same_v<BaseT, DerivedT>) {
        return x;
    } else {
        static_assert(std::is_base_of_v<BaseT, DerivedT>);
        static_assert(std::is_polymorphic_v<BaseT>);
        return dynamic_cast<DerivedT&>(x);
    }
}

CORE_META_NS_END

#endif
