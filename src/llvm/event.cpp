//
// Created by vibhanshu on 2026-03-06
//

#include "llvm_builder/event.h"
#include "context_impl.h"
#include "meta/noncopyable.h"
#include "util/debug.h"
#include "util/string_util.h"

LLVM_BUILDER_NS_BEGIN

//
// Event::Impl
//
class Event::Impl : meta::noncopyable {
private:
    const std::string m_name;
public:
    explicit Impl(const std::string& name) : m_name{name} {
    }
    ~Impl() = default;
public:
    const std::string& name() const {
        return m_name;
    }
    bool operator == (const Impl& o) const {
        return m_name == o.m_name;
    }
};

//
// EventImpl
//
EventImpl::EventImpl(const std::string& name)
    : m_impl{std::make_shared<Impl>(name)} {
}

//
// Event
//
Event::Event() : BaseT{State::ERROR} {
}

Event::Event(EventImpl& impl) : BaseT{State::VALID} {
    CODEGEN_FN
    if (impl.is_valid()) {
        m_impl = impl.ptr();
    } else {
        M_mark_error();
    }
}

Event::~Event() = default;

auto Event::name() const -> const std::string& {
    CODEGEN_FN
    if (has_error()) {
        return StringUtil::s_empty;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->name();
    } else {
        M_mark_error();
        return StringUtil::s_empty;
    }
}

auto Event::null(const std::string& log) -> Event {
    static Event s_null_event{};
    LLVM_BUILDER_ASSERT(s_null_event.has_error());
    Event result = s_null_event;
    result.M_mark_error(log);
    return result;
}

auto Event::from_name(const std::string& name) -> Event {
    return CursorContextImpl::find_event(name);
}

bool Event::operator == (const Event& o) const {
    if (has_error() and o.has_error()) {
        return true;
    }
    if (has_error() or o.has_error()) {
        return false;
    }
    std::shared_ptr<Impl> ptr1 = m_impl.lock();
    std::shared_ptr<Impl> ptr2 = o.m_impl.lock();
    if (ptr1 and ptr2) {
        return ptr1.get() == ptr2.get();
    } else {
        return false;
    }
}

//
// EventSet
//
EventSet::EventSet(const Event& event) {
    add(event);
}

void EventSet::M_mark_global() {
    m_is_global = true;
    m_events.clear();
}

void EventSet::add(const Event& event) {
    if (not m_is_global) {
        if (event.is_global()) {
            M_mark_global();
        } else {
            for (const Event& l_event : m_events) {
                if (l_event == event) {
                    return;
                }
            }
            m_events.emplace_back(event);
        }
    }
}

void EventSet::add(const EventSet& events) {
    if (m_is_global or events.m_is_global) {
        M_mark_global();
    } else {
        for (const Event& l_event : events.m_events) {
            add(l_event);
        }
    }
}

EventSet EventSet::set_union(const EventSet& events) const {
    EventSet l_ret;
    l_ret.add(events);
    return l_ret;
}

bool EventSet::contains(const Event& event) const {
    if (m_is_global) {
        return true;
    }
    for (const Event& l_entry : m_events) {
        if (event == l_entry) {
            return true;
        }
    }
    return false;
}

auto EventSet::global() -> EventSet {
    EventSet l_ret;
    l_ret.M_mark_global();
    return l_ret;
}

LLVM_BUILDER_NS_END
