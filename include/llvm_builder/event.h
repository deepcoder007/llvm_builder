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
public:
    static Event from_name(const std::string& name);
    static Event null(const std::string& log = "");
};

LLVM_BUILDER_NS_END

#endif // EVENT_H_
