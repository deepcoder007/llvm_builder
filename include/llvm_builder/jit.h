//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_LLVM_JIT_H_
#define LLVM_BUILDER_LLVM_JIT_H_

#include "llvm_builder/defines.h"
#include "llvm_builder/module.h"
#include "llvm_builder/util/object.h"
#include <memory>
#include <vector>
#include <string>

LLVM_BUILDER_NS_BEGIN

class JustInTimeRunner;

namespace runtime {

class Array;
class Namespace;
class Struct;
class Field;
class EventFn;

enum class type_t {
    unknown,
    boolean,
    int8,
    int16,
    int32,
    int64,
    uint8,
    uint16,
    uint32,
    uint64,
    float32,
    float64,
    pointer_struct,
    pointer_array,
};

// TODO{vibhanshu}: check if all the pointer type fields are initialized
//                 to valid values, before this object is used in event
class Object : public _BaseObject<Field> {
    using BaseT = _BaseObject<Field>;
    friend class Struct;
    friend class Field;
    friend class EventFn;
    class Impl;
private:
    std::shared_ptr<Impl> m_impl;
private:
    explicit Object(const Struct& parent);
public:
    explicit Object();
    Object(const Object&);
    Object(Object&&);
    Object& operator = (const Object&);
    Object& operator = (Object&&);
    ~Object();
public:
    bool is_frozen() const;
    bool try_freeze();
    std::vector<Field> null_fields() const;
    bool is_instance_of(const Struct &o) const;
    const Struct& struct_def() const;
    // TODO{vibhanshu}: remove ref() once this api is stable
    void* ref() const;
    template <typename T>
    T get(const std::string& fname) const;
    template <typename T>
    void set(const std::string& name, T v) const;
    Object get_object(const std::string& name) const;
    void set_object(const std::string& name, const Object& v) const;
    Array get_array(const std::string& name) const;
    void set_array(const std::string& name, const Array& v) const;
    bool operator == (const Object& rhs) const;
    static const Object& null();
};


// TODO{vibhanshu}: possible to merge it with Object?
class Array : public _BaseObject<Field> {
    using BaseT = _BaseObject<Field>;
    class Impl;
private:
    std::shared_ptr<Impl> m_impl;
private:
    explicit Array(type_t element_type, uint32_t size);
public:
    // TODO{vibhanshu}: add type info also to array
    explicit Array();
    Array(const Array&);
    Array(Array&&);
    Array& operator = (const Array&);
    Array& operator = (Array&&);
    ~Array();
public:
    bool is_scalar() const;
    bool is_pointer() const;
    bool is_frozen() const;
    bool try_freeze();
    uint32_t num_elements() const;
    type_t element_type() const;
    uint32_t element_size() const;
    // TODO{vibhanshu}: remove ref() once this api is stable
    void* ref() const;
    template <typename T>
    T get(uint32_t i) const;
    template <typename T>
    void set(uint32_t i, T v) const;
    Object get_object(uint32_t i) const;
    void set_object(uint32_t i, const Object& v) const;
    Array get_array(uint32_t i) const;
    void set_array(uint32_t i, const Array& v) const;
    bool operator == (const Array& rhs) const;
    static const Array& null();
public:
    static Array from(type_t type, uint32_t size);
};

class Field : public _BaseObject<Field> {
    using BaseT = _BaseObject<Field>;
    class Impl;
    struct construct_t {};
    friend class Struct;
    friend class Object;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit Field();
    explicit Field(const Struct& parent, int32_t idx, int32_t offset, const std::string &name, const TypeInfo& type, construct_t);
    ~Field();
public:
    const Struct& struct_def() const;
    int32_t idx() const;
    int32_t offset() const;
    const std::string &name() const;
    bool is_bool() const;
    bool is_struct_pointer() const;
    bool is_array_pointer() const;
    bool is_int8() const;
    bool is_int16() const;
    bool is_int32() const;
    bool is_int64() const;
    bool is_uint8() const;
    bool is_uint16() const;
    bool is_uint32() const;
    bool is_uint64() const;
    bool is_float32() const;
    bool is_float64() const;
    bool operator == (const Field& rhs) const;
    static const Field& null();
    static type_t get_type(const TypeInfo& type);
    static uint32_t get_raw_size(const TypeInfo& type);
private:
    void log_values(std::ostream& os, void* data) const;
};

// TODO{vibhanshu}: add API to return a aggregate or some view of different set of underlying variables
// e.g a group of ints, arg1, arg2, ... argN  can be transformed to an array view based API like int[N]
// this is very similar to memroy view protocol
class Struct : public _BaseObject<Struct> {
    using BaseT = _BaseObject<Struct>;
    class Impl;
    friend class Namespace;
    friend class Object;
    friend class Array;
    friend class Field;
    struct construct_t {};
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit Struct();
    explicit Struct(const TypeInfo& type, construct_t);
    ~Struct() = default;
public:
    const std::string& name() const;
    int32_t size_in_bytes() const;
    int32_t num_fields() const;
    const std::vector<std::string>& field_names() const;
    Object mk_object() const;
    Field operator[] (const std::string& s) const;
    bool operator == (const Struct& rhs) const;
    static const Struct& null();
private:
    void log_values(std::ostream& os, void* data, uint32_t size) const;
};

Field operator""_field(const char* s, size_t len);

class EventFn : public _BaseObject<EventFn> {
    using BaseT = _BaseObject<EventFn>;
    friend class Namespace;
    class Impl;
    struct construct_t{};
public:
    using event_fn_t = int32_t(void*);
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit EventFn();
    explicit EventFn(JustInTimeRunner& runner, const std::string& name, construct_t);
    ~EventFn() = default;
public:
    bool is_init() const;
    void init();
    int32_t on_event(const Object& o) const;
    bool operator == (const EventFn& rhs) const;
    static const EventFn& null();
};

// TODO{vibhanshu}: Namespace can't have circular dependency,
//           if a event in namespace A depends on event in namespace B
//           then there can  be no dependency of any event in B on any event of A
//           implement this check
class Namespace : public _BaseObject<Namespace> {
    friend class LLVM_BUILDER_NS()::JustInTimeRunner;
    using BaseT = _BaseObject<Namespace>;
    class Impl;
    struct construct_t {};
public:
    using symbol_type = LinkSymbol::symbol_type;
    using event_fn_t = void(void*);
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit Namespace();
    explicit Namespace(JustInTimeRunner& runner, const std::string& ns, construct_t);
    ~Namespace() = default;
public:
    const std::string& name() const;
    bool is_bind() const;
    bool is_global() const;
    void bind();
    Struct struct_info(const std::string& name) const;
    EventFn event_fn_info(const std::string& name) const;
    bool operator == (const Namespace& rhs) const;
    static const Namespace& null();
private:
    void add_struct(const TypeInfo& struct_type);
    void add_event(const std::string& e);
};

} // namespace runtime

// Backwards compatibility aliases
using RuntimeObject = runtime::Object;
using RuntimeArray = runtime::Array;
using RuntimeField = runtime::Field;
using RuntimeStruct = runtime::Struct;
using RuntimeEventFn = runtime::EventFn;
using RuntimeNamespace = runtime::Namespace;
using runtime_type_t = runtime::type_t;

class JustInTimeRunner : public _BaseObject<JustInTimeRunner> {
    using BaseT = _BaseObject<JustInTimeRunner>;
    class Impl;
    using fn_t = int32_t (void*);
    std::shared_ptr<Impl> m_impl;
    CONTEXT_DECL(JustInTimeRunner)
    struct null_tag_t {};
    explicit JustInTimeRunner(null_tag_t);
public:
    explicit JustInTimeRunner();
    ~JustInTimeRunner();
public:
    void bind();
    bool is_bind() const;
    bool contains_symbol_definition(const std::string& name) const;
    // TODO{vibhanshu}: allow ability to customize passes based on user requirement
    bool process_module_fn(Function& fn);
    void add_module(Cursor& cursor);
    fn_t* get_fn(const std::string& symbol) const;
    runtime::Namespace get_namespace(const std::string& name) const;
    runtime::Namespace get_global_namespace() const;
    bool operator == (const JustInTimeRunner& o) const;
    static const JustInTimeRunner& null();
};

LLVM_BUILDER_NS_END

#endif // JIT_H_
