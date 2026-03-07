//
// Created by vibhanshu on 2026-03-06
//

#ifndef LLVM_BUILDER_LLVM_EVENT_H_
#define LLVM_BUILDER_LLVM_EVENT_H_

#include "llvm_builder/defines.h"
#include "llvm_builder/util/object.h"

LLVM_BUILDER_NS_BEGIN

class EventImpl;

class Event : public _BaseObject {
    using BaseT = _BaseObject;
    friend class EventImpl;
public:
    class Impl;
private:
    std::weak_ptr<Impl> m_impl;
public:
    explicit Event();
    explicit Event(EventImpl& impl);
    ~Event();
public:
    const std::string& name() const;
    bool operator == (const Event& o) const;
    bool is_global() const {
        return has_error();
    }
public:
    static Event from_name(const std::string& name);
    static Event null(const std::string& log = "");
};

class EventSet {
    std::vector<Event> m_events;
    bool m_is_global = false;
public:
    explicit EventSet() = default;
    explicit EventSet(const Event& event);
    ~EventSet() = default;
public:
    void add(const Event& event);
    void add(const EventSet& events);
    EventSet set_union(const EventSet& events) const;
    bool contains(const Event& event) const;
    bool is_global() const {
        return m_is_global;
    }
    bool is_null() const {
        return not m_is_global and m_events.empty();
    }
    const std::vector<Event>& values() const {
        return m_events;
    }
    static EventSet global();
private:
    void M_mark_global();
};
static_assert(sizeof(EventSet) == 32);

LLVM_BUILDER_NS_END

#endif // EVENT_H_
