//
// Created by vibhanshu on 2026-03-07
//

#include "llvm_builder/util/object.h"

LLVM_BUILDER_NS_BEGIN

_BaseObject::_BaseObject(State state) : m_state{state} {
}

_BaseObject::_BaseObject(_BaseObject&& o) : m_log{std::move(o.m_log)}, m_state{o.m_state} {
}

_BaseObject::_BaseObject(const _BaseObject& o) : m_log{o.m_log}, m_state{o.m_state} {
}

_BaseObject& _BaseObject::operator = (const _BaseObject& o) {
    m_log = o.m_log;
    m_state = o.m_state;
    return *this;
}

_BaseObject& _BaseObject::operator = (_BaseObject&& o) {
    m_log = std::move(o.m_log);
    m_state = o.m_state;
    return *this;
}

LLVM_BUILDER_NS_END
