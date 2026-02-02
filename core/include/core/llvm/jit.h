//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_LLVM_JIT_H_
#define CORE_LLVM_JIT_H_

#include "defines.h"
#include "core/meta/noncopyable.h"
#include "core/llvm/module.h"
#include "core/util/object.h"
#include <memory>
#include <vector>
#include <string>

LLVM_NS_BEGIN

class JustInTimeRunner;
class RuntimeArray;
class RuntimeNamespace;
class RuntimeStruct;
class RuntimeField;
class RuntimeEventFn;

enum class runtime_type_t {
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
class RuntimeObject : public _BaseObject<RuntimeField> {
    using BaseT = _BaseObject<RuntimeField>;
    friend class RuntimeStruct;
    friend class RuntimeField;
    friend class RuntimeEventFn;
    class Impl;
private:
    std::shared_ptr<Impl> m_impl;
private:
    explicit RuntimeObject(const RuntimeStruct& parent);
public:
    explicit RuntimeObject();
    RuntimeObject(const RuntimeObject&);
    RuntimeObject(RuntimeObject&&);
    RuntimeObject& operator = (const RuntimeObject&);
    RuntimeObject& operator = (RuntimeObject&&);
    ~RuntimeObject();
public:
    bool is_frozen() const;
    bool try_freeze();
    std::vector<RuntimeField> null_fields() const;
    bool is_instance_of(const RuntimeStruct &o) const;
    const RuntimeStruct& struct_def() const;
    // TODO{vibhanshu}: remove ref() once this api is stable
    void* ref() const;
    template <typename T>
    T get(const std::string& fname) const;
    template <typename T>
    void set(const std::string& name, T v) const;
    RuntimeObject get_object(const std::string& name) const;
    void set_object(const std::string& name, const RuntimeObject& v) const;
    RuntimeArray get_array(const std::string& name) const;
    void set_array(const std::string& name, const RuntimeArray& v) const;
    bool operator == (const RuntimeObject& rhs) const;
    static const RuntimeObject& null();
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeObject)
};


// TODO{vibhanshu}: possible to merge it with RuntimeObject?
class RuntimeArray : public _BaseObject<RuntimeField> {
    using BaseT = _BaseObject<RuntimeField>;
    class Impl;
private:
    std::shared_ptr<Impl> m_impl;
private:
    explicit RuntimeArray(runtime_type_t element_type, uint32_t size);
public:
    // TODO{vibhanshu}: add type info also to array
    explicit RuntimeArray();
    RuntimeArray(const RuntimeArray&);
    RuntimeArray(RuntimeArray&&);
    RuntimeArray& operator = (const RuntimeArray&);
    RuntimeArray& operator = (RuntimeArray&&);
    ~RuntimeArray();
public:
    bool is_scalar() const;
    bool is_pointer() const;
    bool is_frozen() const;
    bool try_freeze();
    uint32_t num_elements() const;
    runtime_type_t element_type() const;
    uint32_t element_size() const;
    // TODO{vibhanshu}: remove ref() once this api is stable
    void* ref() const;
    template <typename T>
    T get(uint32_t i) const;
    template <typename T>
    void set(uint32_t i, T v) const;
    RuntimeObject get_object(uint32_t i) const;
    void set_object(uint32_t i, const RuntimeObject& v) const;
    RuntimeArray get_array(uint32_t i) const;
    void set_array(uint32_t i, const RuntimeArray& v) const;
    bool operator == (const RuntimeArray& rhs) const;
    static const RuntimeArray& null();
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeArray)
public:
    static RuntimeArray from(runtime_type_t type, uint32_t size);
};

class RuntimeField : public _BaseObject<RuntimeField> {
    using BaseT = _BaseObject<RuntimeField>;
    class Impl;
    struct construct_t {};
    friend class RuntimeStruct;
    friend class RuntimeObject;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit RuntimeField();
    explicit RuntimeField(const RuntimeStruct& parent, int32_t idx, int32_t offset, const std::string &name, const TypeInfo& type, construct_t);
    ~RuntimeField();
public:
    const RuntimeStruct& struct_def() const;
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
    bool operator == (const RuntimeField& rhs) const;
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeField)
    static const RuntimeField& null();
    static runtime_type_t get_type(const TypeInfo& type);
    static uint32_t get_raw_size(const TypeInfo& type);
private:
    void log_values(std::ostream& os, void* data) const;
};

// TODO{vibhanshu}: add API to return a aggregate or some view of different set of underlying variables
// e.g a group of ints, arg1, arg2, ... argN  can be transformed to an array view based API like int[N]
// this is very similar to memroy view protocol
class RuntimeStruct : public _BaseObject<RuntimeStruct> {
    using BaseT = _BaseObject<RuntimeStruct>;
    class Impl;
    friend class RuntimeNamespace;
    friend class RuntimeObject;
    friend class RuntimeArray;
    friend class RuntimeField;
    struct construct_t {};
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit RuntimeStruct();
    explicit RuntimeStruct(const TypeInfo& type, construct_t);
    ~RuntimeStruct() = default;
public:
    const std::string& name() const;
    int32_t size_in_bytes() const;
    int32_t num_fields() const;
    const std::vector<std::string>& field_names() const;
    RuntimeObject mk_object() const;
    RuntimeField operator[] (const std::string& s) const;
    bool operator == (const RuntimeStruct& rhs) const;
    static const RuntimeStruct& null();
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeStruct)
private:
    void log_values(std::ostream& os, void* data, uint32_t size) const;
};

RuntimeField operator""_field(const char* s, size_t len);

class RuntimeEventFn : public _BaseObject<RuntimeEventFn> {
    using BaseT = _BaseObject<RuntimeEventFn>;
    friend class RuntimeNamespace;
    class Impl;
    struct construct_t{};
public:
    using event_fn_t = int32_t(void*);
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit RuntimeEventFn();
    explicit RuntimeEventFn(JustInTimeRunner& runner, const std::string& name, construct_t);
    ~RuntimeEventFn() = default;
public:
    bool is_init() const;
    void init();
    int32_t on_event(const RuntimeObject& o) const;
    bool operator == (const RuntimeEventFn& rhs) const;
    static const RuntimeEventFn& null();
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeEventFn)
};

// TODO{vibhanshu}: Namespace can't have circular dependency,
//           if a event in namespace A depends on event in namespace B
//           then there can  be no dependency of any event in B on any event of A
//           implement this check
class RuntimeNamespace : public _BaseObject<RuntimeNamespace> {
    friend class JustInTimeRunner;
    using BaseT = _BaseObject<RuntimeNamespace>;
    class Impl;
    friend class JustInTimeRunner;
    struct construct_t {};
public:
    using symbol_type = LinkSymbol::symbol_type;
    using event_fn_t = void(void*);
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit RuntimeNamespace();
    explicit RuntimeNamespace(JustInTimeRunner& runner, const std::string& ns, construct_t);
    ~RuntimeNamespace() = default;
public:
    const std::string& name() const;
    bool is_bind() const;
    bool is_global() const;
    void bind();
    RuntimeStruct struct_info(const std::string& name) const;
    RuntimeEventFn event_fn_info(const std::string& name) const;
    bool operator == (const RuntimeNamespace& rhs) const;
    static const RuntimeNamespace& null();
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(RuntimeNamespace)
private:
    void add_struct(const TypeInfo& struct_type);
    void add_event(const std::string& e);
};

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
    RuntimeNamespace get_namespace(const std::string& name) const;
    RuntimeNamespace get_global_namespace() const;
    bool operator == (const JustInTimeRunner& o) const;
    static const JustInTimeRunner& null();
    void print(std::ostream &os) const;
    OSTREAM_FRIEND(JustInTimeRunner)
};

LLVM_NS_END

#endif // JIT_H_
