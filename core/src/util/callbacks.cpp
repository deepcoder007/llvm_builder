//
// Created by vibhanshu on 2024-08-10
//

#include "core/util/callbacks.h"
#include "util/debug.h"

CORE_NS_BEGIN

//
// BaseCallback
//
BaseCallback::BaseCallback(fixed_string name)
    : m_name{name} {
}

BaseCallback::~BaseCallback() = default;

//
// NamedCallback
//
NamedCallback::NamedCallback(fixed_string name, callback_t&& callback)
    : BaseT{name}, m_callback{std::move(callback)} {
    CORE_ASSERT(m_callback);
}

NamedCallback::~NamedCallback() = default;

void NamedCallback::print(std::ostream& os) const {
    os << "{NamedCallack:name=" << BaseT::name() << "}";
}

template class _CallbackManager<NamedCallback>;

CORE_NS_END
