//
// Created by vibhanshu on 2025-12-16
//

#include "util/debug.h"
#include "meta/noncopyable.h"
#include "llvm_builder/jit.h"
#include "llvm_builder/module.h"
#include "ds/fixed_string.h"
#include "llvm/context_impl.h"
#include "util/string_util.h"
#include "ext_include.h"

#include <memory>
#include <unordered_map>

LLVM_BUILDER_NS_BEGIN

namespace runtime {

//
// Object::Impl
//
class Object::Impl : meta::noncopyable {
    struct ObjectInfo {
        uint64_t m_field_addr;
        Object m_obj;
    };
    struct ArrayInfo {
        uint64_t m_field_addr;
        Array m_arr;
    };
private:
    const Struct& m_parent;
    void* m_buf = nullptr;
    uint32_t m_size = 0;
    std::unordered_map<std::string, ObjectInfo> m_linked_objects;
    std::unordered_map<std::string, ArrayInfo> m_linked_arrays;
    bool m_is_frozen = false;
public:
    explicit Impl(const Struct &parent)
        : m_parent{parent} {
        LLVM_BUILDER_ASSERT(not m_parent.has_error());
        const uint32_t size = parent.size_in_bytes();
        m_buf = std::malloc(size);
        std::memset(m_buf, 0, size);
        m_size = size;
    }
    ~Impl() {
        LLVM_BUILDER_ASSERT(m_buf != nullptr);
        std::free(m_buf);
        m_buf = nullptr;
    }
public:
    bool is_frozen() const {
        return m_is_frozen;
    }
    bool freeze() {
        LLVM_BUILDER_ASSERT(not is_frozen());
        for (const std::string& fname : m_parent.field_names()) {
            const Field l_field = m_parent[fname];
            if (l_field.is_struct_pointer()) {
                Object l_object = get_object(fname);
                if (l_object.has_error()) {
                    return false;
                }
            } else if (l_field.is_array_pointer()) {
                Array l_array = get_array(fname);
                if (l_array.has_error()) {
                    return false;
                }
            } else if (l_field.is_fn_pointer()) {
                uint64_t* l_ptr = get_field_location<uint64_t>(l_field);              
                if (*l_ptr == 0) {
                    return false;
                }
            }
        }
        m_is_frozen = true;
        return m_is_frozen;
    }
    std::vector<Field> null_fields() const {
        std::vector<Field> l_result;
        for (const std::string& fname : m_parent.field_names()) {
            const Field l_field = m_parent[fname];
            if (l_field.is_struct_pointer()) {
                Object l_object = get_object(fname);
                if (l_object.has_error()) {
                    l_result.emplace_back(l_field);
                }
            } else if (l_field.is_array_pointer()) {
                Array l_arr = get_array(fname);
                if (l_arr.has_error()) {
                    l_result.emplace_back(l_field);
                }
            } else if (l_field.is_fn_pointer()) {
                uint64_t* l_ptr = get_field_location<uint64_t>(l_field);              
                if (*l_ptr == 0) {
                    l_result.emplace_back(l_field);
                }
            }
        }
        return l_result;
    }
    bool is_instance_of(const Struct& o) const {
        LLVM_BUILDER_ASSERT(not o.has_error());
        return m_parent == o;
    }
    const Struct& struct_def() const {
        return m_parent;
    }
    void *ref() const {
        LLVM_BUILDER_ASSERT(m_buf != nullptr);
        return m_buf;
    }
    template <typename T>
    T* get_field_location(const Field& field) const {
        LLVM_BUILDER_ASSERT(is_instance_of(field.struct_def()));
        int32_t l_offset = field.offset();
        char* l_field_ptr = M_get_buf((uint32_t)l_offset);
        LLVM_BUILDER_ASSERT(l_field_ptr  != nullptr);
        return reinterpret_cast<T*>(l_field_ptr);
    }
    void add_object(const std::string& field_name, uint64_t field_addr, const Object& o) {
        LLVM_BUILDER_ASSERT(field_addr != 0);
        LLVM_BUILDER_ASSERT(not o.has_error());
        LLVM_BUILDER_ASSERT(o.is_frozen());
        m_linked_objects[field_name] = {field_addr, o};
    }
    Object get_object(const std::string& field_name) const {
        if (m_linked_objects.contains(field_name)) {
            return m_linked_objects.at(field_name).m_obj;
        } else {
            return Object::null();
        }
    }
    void add_array(const std::string& field_name, uint64_t field_addr, const Array& o) {
        LLVM_BUILDER_ASSERT(field_addr != 0);
        LLVM_BUILDER_ASSERT(not o.has_error());
        LLVM_BUILDER_ASSERT(o.is_frozen());
        m_linked_arrays[field_name] = {field_addr, o};
    }
    Array get_array(const std::string& field_name) const {
        if (m_linked_arrays.contains(field_name)) {
            return m_linked_arrays.at(field_name).m_arr;
        } else {
            return Array::null();
        }
    }
private:
    char* M_get_buf(uint32_t i) const {
        if (i < m_size) {
            return reinterpret_cast<char*>(ref()) + i;
        } else {
            return nullptr;
        }
    }
};

//
// Object
//
Object::Object() : BaseT{State::ERROR} {
}

Object::Object(const Struct& parent)
    : BaseT{State::VALID} {
    CODEGEN_FN
    if (parent.has_error()) {
        M_mark_error();
    } else {
        m_impl = std::make_shared<Impl>(parent);
    }
}

Object::Object(const Object&) = default;

Object::Object(Object&&) = default;

Object& Object::operator = (const Object&) = default;

Object& Object::operator = (Object&&) = default;

Object::~Object() = default;

bool Object::is_frozen() const {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_frozen();
}

bool Object::freeze() {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    if (m_impl->is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "Trying to re-freeze a object")
        return false;
    }
    return m_impl->freeze();
}

auto Object::null_fields() const -> std::vector<Field> {
    CODEGEN_FN;
    if (has_error()) {
        return std::vector<Field>{};
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->null_fields();
}

bool Object::is_instance_of(const Struct &o) const {
    if (has_error() or o.has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_instance_of(o);
}

const Struct& Object::struct_def() const {
    if (has_error()) {
        return Struct::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->struct_def();
}

void *Object::ref() const {
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "Trying to access reference of invalid object")
        return nullptr;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->ref();
}

Object Object::get_object(const std::string& name) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return Object::null();
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return Object::null();
    }
    if (l_field.is_struct_pointer()) {
        return m_impl->get_object(name);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not struct:" << name);
        return Object::null();
    }
}

void Object::set_object(const std::string& name, const Object& v) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return;
    }
    if (v.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't set invalid object for field:" << name);
        return;
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return;
    }
    if (not v.is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "can only set a frozen object for field:" << name);
        return;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "object already frozen, can't set new value");
        return;
    }
    // TODO{vibhanshu}: check if struct object v is compatible with type
    if (l_field.is_struct_pointer()) {
        uint64_t* l_ptr = m_impl->get_field_location<uint64_t>(l_field);
        LLVM_BUILDER_ASSERT(v.ref() != nullptr);
        *l_ptr = (uint64_t)v.ref();
        m_impl->add_object(name, (uint64_t)l_ptr, v);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not struct:" << name);
    }
}

Array Object::get_array(const std::string& name) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return Array::null();
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return Array::null();
    }
    if (l_field.is_array_pointer()) {
        return m_impl->get_array(name);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not array:" << name);
        return Array::null();
    }

}

void Object::set_array(const std::string& name, const Array& v) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return;
    }
    if (v.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't set invalid object for field:" << name);
        return;
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return;
    }
    if (not v.is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "can only set a frozen object for field:" << name);
        return;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "object already frozen, can't set new value");
        return;
    }
    if (l_field.is_array_pointer()) {
        uint64_t* l_ptr = m_impl->get_field_location<uint64_t>(l_field);
        LLVM_BUILDER_ASSERT(v.ref() != nullptr);
        *l_ptr = (uint64_t)v.ref();
        m_impl->add_array(name, (uint64_t)l_ptr, v);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not array:" << name);
    }
}

auto Object::get_fn_ptr(const std::string& name) const -> event_fn_t* {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return nullptr;
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return nullptr;
    }
    if (l_field.is_fn_pointer()) {
        uint64_t l_raw = *m_impl->get_field_location<uint64_t>(l_field);
        return reinterpret_cast<event_fn_t*>(l_raw);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not fn_pointer:" << name);
        return nullptr;
    }
}

void Object::set_fn_ptr(const std::string& name, event_fn_t* fn) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);
        return;
    }
    Field l_field = struct_def()[name];
    if (l_field.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);
        return;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "object already frozen, can't set new value");
        return;
    }
    if (l_field.is_fn_pointer()) {
        uint64_t* l_ptr = m_impl->get_field_location<uint64_t>(l_field);
        *l_ptr = reinterpret_cast<uint64_t>(fn);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "field type not fn_pointer:" << name);
    }
}

#define DEF_OBJECT_FN(type)                                                            \
template <>                                                                             \
type##_t Object::get(const std::string& name) const {                                   \
    CODEGEN_FN                                                                          \
    if (has_error()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);       \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
    Field l_field = struct_def()[name];                                                 \
    if (l_field.has_error()) {                                                          \
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);                           \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
    if (l_field.is_##type()) {                                                          \
        return *m_impl->get_field_location<type##_t>(l_field);                          \
    } else {                                                                            \
        CODEGEN_PUSH_ERROR(JIT, "Field type not " << #type << ":" << name);             \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
}                                                                                       \
template <>                                                                             \
void Object::set(const std::string& name, type##_t v) const {                           \
    CODEGEN_FN                                                                          \
    if (has_error()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "can't access field in invalid object:" << name);       \
        return;                                                                         \
    }                                                                                   \
    Field l_field = struct_def()[name];                                                 \
    if (l_field.has_error()) {                                                          \
        CODEGEN_PUSH_ERROR(JIT, "can't find field:" << name);                           \
        return;                                                                         \
    }                                                                                   \
    if (is_frozen()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "object already frozen, can't set new value");          \
        return;                                                                         \
    }                                                                                   \
    if (l_field.is_##type()) {                                                          \
        *m_impl->get_field_location<type##_t>(l_field) = v;                             \
    } else {                                                                            \
        CODEGEN_PUSH_ERROR(JIT, "Field type not " << #type << ":" << name);             \
    }                                                                                   \
}                                                                                       \
/**/

DEF_OBJECT_FN(bool)
DEF_OBJECT_FN(int8)
DEF_OBJECT_FN(int16)
DEF_OBJECT_FN(int32)
DEF_OBJECT_FN(int64)
DEF_OBJECT_FN(uint8)
DEF_OBJECT_FN(uint16)
DEF_OBJECT_FN(uint32)
DEF_OBJECT_FN(uint64)
DEF_OBJECT_FN(float32)
DEF_OBJECT_FN(float64)

#undef DEF_OBJECT_FN

bool Object::operator==(const Object &rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const Object& Object::null() {
    static Object s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// Array::Impl
//
class Array::Impl : meta::noncopyable {
    const uint32_t m_size = 0;
    const type_t m_element_type = type_t::unknown;
    const uint32_t m_element_size = 0;
    void* m_buf = nullptr;
    // TODO{vibhanshu}: v1 assuming, black-box pointers, add meta-info about types maybe ?
    Object* m_array_objects = nullptr;
    Array* m_array2_objects = nullptr;
    bool m_is_frozen = false;
public:
    explicit Impl(type_t element_type, uint32_t size)
        : m_size{size}
        , m_element_type{element_type}
        , m_element_size{M_element_size(m_element_type)} {
        LLVM_BUILDER_ASSERT(m_size > 0);
        LLVM_BUILDER_ASSERT(m_element_size != std::numeric_limits<uint32_t>::max())
        m_buf = std::aligned_alloc(128, m_size * m_element_size);
        std::memset(m_buf, 0, m_size * m_element_size);
        if (is_pointer()) {
            if (m_element_type == type_t::pointer_struct) {
                m_array_objects = new Object[m_size];
            } else {
                LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_array);
                m_array2_objects = new Array[m_size];
            }
        }
    }
    ~Impl() {
        std::free(m_buf);
        m_buf = nullptr;
        if (m_array_objects) {
            delete[] m_array_objects;
        }
        if (m_array2_objects) {
            delete[] m_array2_objects;
        }
    }
public:
    bool is_scalar() const {
        return is_pointer();
    }
    bool is_pointer() const {
        return m_element_type == type_t::pointer_array or m_element_type == type_t::pointer_struct;
    }
    bool is_frozen() const {
        return m_is_frozen;
    }
    bool freeze() {
        LLVM_BUILDER_ASSERT(not is_frozen());
        if (is_pointer()) {
            if (m_element_type == type_t::pointer_struct) {
                for (uint32_t i = 0; i != m_size; ++i) {
                    if (m_array_objects[i].has_error()) {
                        return false;
                    }
                }
            } else {
                LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_array);
                for (uint32_t i = 0; i != m_size; ++i) {
                    if (m_array2_objects[i].has_error()) {
                        return false;
                    }
                }
            }
        }
        m_is_frozen = true;
        return m_is_frozen;
    }
    uint32_t num_elements() const {
        return m_size;
    }
    type_t element_type() const {
        return m_element_type;
    }
    uint32_t element_size() const {
        return m_element_size;
    }
    void* ref() const {
        LLVM_BUILDER_ASSERT(m_buf != nullptr);
        return m_buf;
    }
    void set_object(uint32_t i, const Object& v) {
        LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_struct);
        LLVM_BUILDER_ASSERT(m_array_objects != nullptr);
        LLVM_BUILDER_ASSERT(i < m_size);
        m_array_objects[i] = v;
    }
    void set_array(uint32_t i, const Array& v) {
        LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_array);
        LLVM_BUILDER_ASSERT(m_array2_objects != nullptr);
        LLVM_BUILDER_ASSERT(i < m_size);
        m_array2_objects[i] = v;
    }
    Object get_object(uint32_t i) const {
        LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_struct);
        LLVM_BUILDER_ASSERT(m_array_objects != nullptr);
        LLVM_BUILDER_ASSERT(i < m_size);
        return m_array_objects[i];
    }
    Array get_array(uint32_t i) const {
        LLVM_BUILDER_ASSERT(m_element_type == type_t::pointer_array);
        LLVM_BUILDER_ASSERT(m_array2_objects != nullptr);
        LLVM_BUILDER_ASSERT(i < m_size);
        return m_array2_objects[i];
    }
    void log_values(std::ostream &os) const {
#define LOG_CASE(type)  case type_t::type:  M_print_type<type##_t>(os); break;
        switch(m_element_type) {
            case type_t::boolean: M_print_type<bool>(os);     break;
            LOG_CASE(int8)
            LOG_CASE(int16)
            LOG_CASE(int32)
            LOG_CASE(int64)
            LOG_CASE(uint8)
            LOG_CASE(uint16)
            LOG_CASE(uint32)
            LOG_CASE(uint64)
            LOG_CASE(float32)
            LOG_CASE(float64)
            case type_t::pointer_struct: {
                const uint64_t* l_ref = reinterpret_cast<const uint64_t*>(ref());
                os << "[";
                for (uint32_t i = 0; i != m_size; ++i) {
                    os << ", " << l_ref[i];
                }
                os << "]";
                break;
            }
            case type_t::pointer_array: {
                const uint64_t* l_ref = reinterpret_cast<const uint64_t*>(ref());
                os << "[";
                for (uint32_t i = 0; i != m_size; ++i) {
                    os << ", " << l_ref[i];
                }
                os << "]";
                break;
            }
            default:
                os << "[type not printable]";
        }
#undef LOG_CASE
    }
private:
    template <typename T>
    void M_print_type(std::ostream& os) const {
        const T* l_ref = reinterpret_cast<const T*>(ref());
        os << "[";
        for (uint32_t i = 0; i != m_size; ++i) {
            os << ", " << l_ref[i];
        }
        os << "]";
    }
    static uint32_t M_element_size(type_t type) {
        switch (type) {
        case type_t::unknown:
          return std::numeric_limits<uint32_t>::max();
          break;
        case type_t::boolean: return 1; break;
        case type_t::int8:    return 1; break;
        case type_t::int16:   return 2; break;
        case type_t::int32:   return 4; break;
        case type_t::int64:   return 8; break;
        case type_t::uint8:   return 1; break;
        case type_t::uint16:  return 2; break;
        case type_t::uint32:  return 4; break;
        case type_t::uint64:  return 8; break;
        case type_t::float32: return 4; break;
        case type_t::float64: return 4; break;
        case type_t::pointer_struct: return sizeof(uint64_t); break;
        case type_t::pointer_array:  return sizeof(uint64_t); break;
        default:   return std::numeric_limits<uint32_t>::max(); break;
        }
    }
};

//
// Array
//
Array::Array() : BaseT{State::ERROR} {
}

Array::Array(type_t element_type, uint32_t size)
    : BaseT{State::VALID} {
    if (size == 0) {
        CODEGEN_PUSH_ERROR(JIT, "Can't define array of length 0")
        M_mark_error();
    } else if (element_type == type_t::unknown) {
        CODEGEN_PUSH_ERROR(JIT, "Can't define array of invalid type")
        M_mark_error();
    } else {
        m_impl = std::make_shared<Impl>(element_type, size);
    }
}

Array::Array(const Array&) = default;
Array::Array(Array&&) = default;
Array& Array::operator = (const Array&) = default;
Array& Array::operator = (Array&&) = default;

Array::~Array() = default;

bool Array::is_scalar() const {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_scalar();
}

bool Array::is_pointer() const {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_pointer();
}

bool Array::is_frozen() const {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_frozen();
}

bool Array::freeze() {
    CODEGEN_FN;
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    if (m_impl->is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "Trying to re-freeze array")
        return false;
    }
    return m_impl->freeze();
}

uint32_t Array::num_elements() const {
    if (has_error()) {
        return std::numeric_limits<uint32_t>::max();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->num_elements();
}

type_t Array::element_type() const {
    if (has_error()) {
        return type_t::unknown;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->element_type();
}

uint32_t Array::element_size() const {
    if (has_error()) {
        return std::numeric_limits<uint32_t>::max();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->element_size();
}

void* Array::ref() const {
    if (has_error()) {
        return nullptr;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->ref();
}

#define DEF_ARRAY_FN(type)                                                              \
template <>                                                                             \
type##_t Array::get(uint32_t i) const {                                                 \
    CODEGEN_FN                                                                          \
    if (has_error()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");                         \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
    if (i >= num_elements()) {                                                          \
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);                      \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
    if (element_type() == type_t::type) {                                               \
        const type##_t* l_arr = reinterpret_cast<const type##_t*>(ref());               \
        LLVM_BUILDER_ASSERT(l_arr != nullptr);                                                  \
        return l_arr[i];                                                                \
    } else {                                                                            \
        CODEGEN_PUSH_ERROR(JIT, "Field type not " << #type);                            \
        return std::numeric_limits<type##_t>::max();                                    \
    }                                                                                   \
}                                                                                       \
template <>                                                                             \
void Array::set(uint32_t i, type##_t v) const {                                         \
    CODEGEN_FN                                                                          \
    if (has_error()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");                         \
        return;                                                                         \
    }                                                                                   \
    if (i >= num_elements()) {                                                          \
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);                      \
        return;                                                                         \
    }                                                                                   \
    if (is_frozen()) {                                                                  \
        CODEGEN_PUSH_ERROR(JIT, "object already frozen, can't set new value");          \
        return;                                                                         \
    }                                                                                   \
    if (element_type() == type_t::type) {                                               \
        type##_t* l_arr = reinterpret_cast<type##_t*>(ref());                           \
        LLVM_BUILDER_ASSERT(l_arr != nullptr);                                                  \
        l_arr[i] = v;                                                                   \
    } else {                                                                            \
        CODEGEN_PUSH_ERROR(JIT, "Field type not " << #type);                            \
        return;                                                                         \
    }                                                                                   \
}                                                                                       \
/**/

// DEF_ARRAY_FN(bool)
DEF_ARRAY_FN(int8)
DEF_ARRAY_FN(int16)
DEF_ARRAY_FN(int32)
DEF_ARRAY_FN(int64)
DEF_ARRAY_FN(uint8)
DEF_ARRAY_FN(uint16)
DEF_ARRAY_FN(uint32)
DEF_ARRAY_FN(uint64)

#undef DEF_ARRAY_FN

Object Array::get_object(uint32_t i) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");
        return Object::null();
    }
    if (i >= num_elements()) {
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);
        return Object::null();
    }
    LLVM_BUILDER_ASSERT(m_impl)
    if (m_impl->element_type() == type_t::pointer_struct) {
        return m_impl->get_object(i);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "array entry not of type struct");
        return Object::null();
    }
}

void Array::set_object(uint32_t i, const Object& v) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");
        return;
    }
    if (v.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't set invalid object:");
        return;
    }
    if (i >= num_elements()) {
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);
        return;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "array already frozen, can't set new value");
        return;
    }
    if (not v.is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "array entry object to be set should be frozen");
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl)
    if (m_impl->element_type() == type_t::pointer_struct) {
        uint64_t* l_arr = reinterpret_cast<uint64_t*>(ref());
        l_arr[i] = (uint64_t)v.ref();
        m_impl->set_object(i, v);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "array entry not of type struct");
        return;
    }
}

Array Array::get_array(uint32_t i) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");
        return Array::null();
    }
    if (i >= num_elements()) {
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);
        return Array::null();
    }
    LLVM_BUILDER_ASSERT(m_impl)
    if (m_impl->element_type() == type_t::pointer_array) {
        return m_impl->get_array(i);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "array entry not of type array");
        return Array::null();
    }
}

void Array::set_array(uint32_t i, const Array& v) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't access invalid array:");
        return;
    }
    if (v.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't set invalid array entry:");
        return;
    }
    if (i >= num_elements()) {
        CODEGEN_PUSH_ERROR(JIT, "array index out of range:" << i);
        return;
    }
    if (is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "array already frozen, can't set new value");
        return;
    }
    if (not v.is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "array entry to be set should be frozen");
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl)
    if (m_impl->element_type() == type_t::pointer_array) {
        uint64_t* l_arr = reinterpret_cast<uint64_t*>(ref());
        l_arr[i] = (uint64_t)v.ref();
        m_impl->set_array(i, v);
    } else {
        CODEGEN_PUSH_ERROR(JIT, "array entry not of type struct");
        return;
    }
}

bool Array::operator == (const Array& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const Array& Array::null() {
    static Array s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

auto Array::from(type_t type, uint32_t size) -> Array {
    CODEGEN_FN
    if (type == type_t::unknown) {
        CODEGEN_PUSH_ERROR(JIT, "Can't create array of unknown type")
        return Array::null();
    }
    if (size == 0 or size == std::numeric_limits<uint32_t>::max()) {
        CODEGEN_PUSH_ERROR(JIT, "Can't create array of invalid size")
        return Array::null();
    }
    return Array{type, size};
}

//
// Field::Impl
//
class Field::Impl : meta::noncopyable {
    const Struct& m_parent;
    const int32_t m_idx;
    const int32_t m_offset;
    const std::string m_name;
    const type_t m_type = type_t::unknown;
    Struct m_underlying_struct;
public:
    Impl(const Struct& parent,
            int32_t idx, int32_t offset,
            const std::string &name,
            const TypeInfo& type)
        : m_parent{parent}
        , m_idx{idx}
        , m_offset{offset}
        , m_name{name}
        , m_type{get_type(type)} {
        LLVM_BUILDER_ASSERT(not m_parent.has_error())
        LLVM_BUILDER_ASSERT(m_idx != std::numeric_limits<int32_t>::max())
        LLVM_BUILDER_ASSERT(m_offset != std::numeric_limits<int32_t>::max())
        LLVM_BUILDER_ASSERT(not m_name.empty());
        if (is_struct_pointer()) {
            LLVM_BUILDER_ASSERT(type.is_pointer());
            LLVM_BUILDER_ASSERT(type.base_type().is_struct());
            m_underlying_struct = Struct{type.base_type(), Struct::construct_t{}};
        } else if (is_array_pointer()) {
            // TODO{vibhanshu}: do`we need to go deep and extract more info about internals of array`
        }
    }
    ~Impl() = default;
public:
    const Struct& struct_def() const {
        return m_parent;
    }
    type_t type() const {
        return m_type;
    }
    bool is_bool() const {
        return m_type == type_t::boolean;
    }
    bool is_struct_pointer() const {
        return m_type == type_t::pointer_struct;
    }
    bool is_array_pointer() const {
        return m_type == type_t::pointer_array;
    }
    bool is_fn_pointer() const {
        return m_type == type_t::pointer_fn;
    }
    bool is_pointer() const {
        return is_struct_pointer() or is_array_pointer() or is_fn_pointer();
    }
    bool is_int8() const {
        return m_type == type_t::int8;
    }
    bool is_int16() const {
        return m_type == type_t::int16;
    }
    bool is_int32() const {
        return m_type == type_t::int32;
    }
    bool is_int64() const {
        return m_type == type_t::int64;
    }
    bool is_uint8() const {
        return m_type == type_t::uint8;
    }
    bool is_uint16() const {
        return m_type == type_t::uint16;
    }
    bool is_uint32() const {
        return m_type == type_t::uint32;
    }
    bool is_uint64() const {
        return m_type == type_t::uint64;
    }
    bool is_float32() const {
        return m_type == type_t::float32;
    }
    bool is_float64() const {
        return m_type == type_t::float64;
    }
    int32_t idx() const {
        return m_idx;
    }
    int32_t offset() const {
        return m_offset;
    }
    const std::string &name() const {
        return m_name;
    }
    void log_values(std::ostream& os, void* data) const {
        if (data != nullptr) {
            switch (m_type) {
            case type_t::unknown:  os << "<unknown type>"; break;
            case type_t::boolean:  os << *((bool*)data); break;
            case type_t::int8:     os << *((int8_t*)data); break;
            case type_t::int16:    os << *((int16_t*)data); break;
            case type_t::int32:    os << *((int32_t*)data); break;
            case type_t::int64:    os << *((int64_t*)data); break;
            case type_t::uint8:    os << *((uint8_t*)data); break;
            case type_t::uint16:   os << *((uint16_t*)data); break;
            case type_t::uint32:   os << *((uint32_t*)data); break;
            case type_t::uint64:   os << *((uint64_t*)data); break;
            case type_t::float32:  os << *((float32_t*)data); break;
            case type_t::float64:  os << *((float64_t*)data); break;
            case type_t::pointer_struct: {
                            os << "{<pointer>" << *((uint64_t *)data);
                            LLVM_BUILDER_ASSERT(not m_underlying_struct.has_error());
                            m_underlying_struct.log_values(os, (void*)*((uint64_t*)data), m_underlying_struct.size_in_bytes());
                            os << "}";
                            break;
                        }
            case type_t::pointer_array: {
                            os << "<pointer>" << *((uint64_t*)data);
                            // TODO{vibhanshu}: log underlying array
                            break;
                        }
            case type_t::pointer_fn: {
                            os << "<fn_pointer>" << *((uint64_t*)data);
                            break;
                        }
            }
        } else {
            os << "<invalid_buffer>";
        }
    }
};


//
// Field
//
Field::Field() : BaseT{State::ERROR} {
}

Field::Field(const Struct& parent,
                           int32_t idx, int32_t offset,
                           const std::string &name,
                           const TypeInfo& type, construct_t)
    : BaseT{State::VALID} {
    m_impl = std::make_shared<Impl>(parent, idx, offset, name, type);
}

Field::~Field() = default;

const Struct& Field::struct_def() const {
    if (has_error()) {
        return Struct::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->struct_def();
}

int32_t Field::idx() const {
    if (has_error()) {
        return std::numeric_limits<int32_t>::max();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->idx();
}

int32_t Field::offset() const {
    if (has_error()) {
        return std::numeric_limits<int32_t>::max();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->offset();
}

const std::string& Field::name() const {
    if (has_error()) {
        return StringManager::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->name();
}

#define IS_TYPE_FWD(type)                          \
    bool Field::is_##type() const {                \
        if (has_error()) {                         \
            return false;                          \
        }                                          \
        LLVM_BUILDER_ASSERT(m_impl);                       \
        return m_impl->is_##type();                \
    }                                              \
/**/                                               \

IS_TYPE_FWD(bool)
IS_TYPE_FWD(struct_pointer)
IS_TYPE_FWD(array_pointer)
IS_TYPE_FWD(fn_pointer)
IS_TYPE_FWD(int8)
IS_TYPE_FWD(int16)
IS_TYPE_FWD(int32)
IS_TYPE_FWD(int64)
IS_TYPE_FWD(uint8)
IS_TYPE_FWD(uint16)
IS_TYPE_FWD(uint32)
IS_TYPE_FWD(uint64)
IS_TYPE_FWD(float32)
IS_TYPE_FWD(float64)

#undef IS_TYPE_FWD

void Field::log_values(std::ostream& os, void* data) const {
    if (has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->log_values(os, data);
}

bool Field::operator == (const Field& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const Field &Field::null() {
    static Field s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

type_t Field::get_type(const TypeInfo& type) {
    if (type.has_error()) {
        return type_t::unknown;
    }
    const uint32_t l_size = type.size_in_bytes();
    if (type.is_boolean()) {
        return type_t::boolean;
    } else if (type.is_pointer()) {
        const TypeInfo& base_type = type.base_type();
        if (base_type.is_struct()) {
            return type_t::pointer_struct;
        } else if (base_type.is_array() or base_type.is_vector()) {
            return type_t::pointer_array;
        } else if (base_type.is_function()) {
            return type_t::pointer_fn;
        }
    } else if (type.is_float()) {
        if (l_size == 4) {
            return type_t::float32;
        } else if (l_size == 8) {
            return type_t::float64;
        }
    } else if (type.is_integer()) {
        if (type.is_signed_integer()) {
            if (l_size == 8) {
                return type_t::int64;
            } else if (l_size == 4) {
                return type_t::int32;
            } else if (l_size == 2) {
                return type_t::int16;
            } else if (l_size == 1) {
                return type_t::int8;
            }
        } else {
            LLVM_BUILDER_ASSERT(type.is_unsigned_integer());
            if (l_size == 8) {
                return type_t::uint64;
            } else if (l_size == 4) {
                return type_t::uint32;
            } else if (l_size == 2) {
                return type_t::uint16;
            } else if (l_size == 1) {
                return type_t::uint8;
            }
        }
    }
    return type_t::unknown;
}

uint32_t Field::get_raw_size(const TypeInfo& type) {
    if (type.has_error()) {
        return std::numeric_limits<uint32_t>::max();
    }
    const uint32_t l_size = type.size_in_bytes();
    if (type.is_pointer()) {
        return sizeof(int*);
    } else if (type.is_scalar()) {
        return l_size;
    } else {
        return std::numeric_limits<uint32_t>::max();
    }
}

//
// Struct::Impl
//
class Struct::Impl : meta::noncopyable {
    const std::string m_name;
    const int32_t m_size = 0;
    std::unordered_map<std::string, Field> m_fields;
    std::vector<std::string> m_field_names;
public:
    explicit Impl(const Struct& parent, const TypeInfo &type)
        : m_name{type.struct_name()}, m_size{(int32_t)type.struct_size_bytes()} {
        LLVM_BUILDER_ASSERT(type.is_struct());
        const uint32_t l_num_fields = type.num_elements();
        for (uint32_t i = 0; i != l_num_fields; ++i) {
            TypeInfo::field_entry_t l_field = type[i];
            const std::string field_name = l_field.name();
            LLVM_BUILDER_ASSERT(l_field.idx() == i);
            LLVM_BUILDER_DEBUG(auto it = ) m_fields.try_emplace(field_name, parent, l_field.idx(), l_field.offset(), field_name, l_field.type(), Field::construct_t{});
            LLVM_BUILDER_ASSERT(it.second);
            m_field_names.emplace_back(field_name);
        }
        LLVM_BUILDER_ASSERT(num_fields() == (int32_t)type.num_elements());
    }
    ~Impl() = default;
public:
    const std::string &name() const {
        return m_name;
    }
    Object mk_object(const Struct& parent) const {
        CODEGEN_FN
        Object l_object{parent};
        LLVM_BUILDER_ASSERT(l_object.m_impl);
        LLVM_BUILDER_ASSERT(not l_object.has_error());
        return l_object;
    }
    int32_t size_in_bytes() const {
        return m_size;
    }
    int32_t num_fields() const {
        return (int32_t)m_fields.size();
    }
    const std::vector<std::string>& field_names() const {
        return m_field_names;
    }
    Field operator[](const std::string &s) const {
        if (m_fields.contains(s)) {
            return m_fields.at(s);
        } else {
            return Field::null();
        }
    }
    void log_values(std::ostream& os, void* data, uint32_t size) const {
        if (data != nullptr) {
            os << "\n";
            for (const std::string& fname : m_field_names) {
                os << fname << ":";
                const Field& l_field = m_fields.at(fname);
                if (l_field.offset() < (int32_t)size) {
                    l_field.log_values(os, (char*)data + l_field.offset());
                } else {
                    os << "<NULL>";
                }
                os << ";\n";
            }
        } else {
            os << "<INVALID_BUFFER>";
        }
    }
};

//
// Struct
//
Struct::Struct() : BaseT{State::ERROR} {
}

Struct::Struct(const TypeInfo& type, construct_t)
  : BaseT{State::VALID} {
    if (not type.is_struct()) {
        CODEGEN_PUSH_ERROR(JIT, "Type not a struct:" << type.short_name());
        M_mark_error();
    } else {
        m_impl = std::make_shared<Impl>(*this, type);
    }
}

const std::string& Struct::name() const {
    if (has_error()) {
        return StringManager::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->name();
}

int32_t Struct::size_in_bytes() const {
    if (has_error()) {
        return 0;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->size_in_bytes();
}

int32_t Struct::num_fields() const {
    if (has_error()) {
        return 0;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->num_fields();
}

const std::vector<std::string>& Struct::field_names() const {
    if (has_error()) {
        static const std::vector<std::string> l_null_vec{};
        return l_null_vec;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->field_names();
}

void Struct::log_values(std::ostream& os, void* data, uint32_t size) const {
    if (has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->log_values(os, data, size);
}


Object Struct::mk_object() const {
    if (has_error()) {
        return Object::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->mk_object(*this);
}

Field Struct::operator[] (const std::string& s) const {
    if (has_error()) {
        return Field::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->operator[](s);
}

bool Struct::operator == (const Struct& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const Struct& Struct::null() {
    static Struct s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// EventFn::Impl
//
class EventFn::Impl : meta::noncopyable {
    JustInTimeRunner& m_runner;
    const std::string m_name;
    event_fn_t* m_event_fn = nullptr;
    bool m_is_init = false;
public:
    explicit Impl(JustInTimeRunner& runner, const std::string& name)
        : m_runner{runner}, m_name{name} {
    }
    ~Impl() = default;
public:
    bool is_init() const {
        return m_is_init;
    }
    void init() {
        CODEGEN_FN
        if (not is_init()) {
            m_event_fn = m_runner.get_fn(m_name);
        }
        m_is_init = true;
    }
    int32_t on_event(const Object &o) const {
        LLVM_BUILDER_ASSERT(is_init());
        LLVM_BUILDER_ASSERT(not o.has_error());
        LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
        LLVM_BUILDER_ASSERT(o.is_frozen());
        // TODO{vibhanshu}: add check that struct type is compatible
        //    with event
        return m_event_fn(o.m_impl->ref());
    }
};

//
// EventFn
//
EventFn::EventFn() : BaseT{State::ERROR} {
}

EventFn::EventFn(JustInTimeRunner& runner, const std::string& name, construct_t)
    : BaseT{State::VALID} {
    m_impl = std::make_shared<Impl>(runner, name);
}

bool EventFn::is_init() const {
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_init();
}

void EventFn::init() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->init();
}

int32_t EventFn::on_event(const Object& o) const {
    CODEGEN_FN
    if (has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't run event on invalid object")
        return -1;
    }
    if (o.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't run event on invalid object")
        return -1;
    }
    if (ErrorContext::has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't run event when there are outstanding error")
        return -1;
    }
    if (not o.is_frozen()) {
        CODEGEN_PUSH_ERROR(JIT, "can't use a object which is not frozen yet")
        return -1;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->on_event(o);
}

bool EventFn::operator==(const EventFn &rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const EventFn& EventFn::null() {
    static EventFn s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// Namespace::Impl
//
class Namespace::Impl : meta::noncopyable {
    JustInTimeRunner& m_runner;
    const std::string m_namespace;
    std::unordered_map<std::string, Struct> m_structs;
    std::unordered_map<std::string, EventFn> m_event_fns;
    bool m_is_bind = false;
    bool m_is_global = false;
public :
    explicit Impl(JustInTimeRunner &runner, const std::string &ns)
        : m_runner{runner}, m_namespace{ns}, m_is_global{ns.empty()} {
    }
    ~Impl() = default;
public:
    const std::string& name() const {
        return m_namespace;
    }
    bool is_bind() const {
        return m_is_bind;
    }
    bool is_global() const {
        return m_is_global;
    }
    void bind(const Namespace& parent) {
        CODEGEN_FN
        // TODO{vibhanshu}: to disallow circular dependency on namespace, disallow any new event addition
        //                  once a namespace is frozen. how to handle the case of global namespace ?
        //                  should we simply freeze global namespace at beginning and later only
        //                  allow namespaced events ?
        if (not is_bind()) {
            for (auto& kv : m_event_fns) {
                kv.second.init();
            }
            m_is_bind = true;
        } else {
            CODEGEN_PUSH_ERROR(JIT, "Namespace already bound:" << m_namespace);
            parent.M_mark_error();
        }
    }
    void add_struct(const Namespace& parent, const TypeInfo &struct_type) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not parent.has_error())
        LLVM_BUILDER_ASSERT(not struct_type.has_error())
        LLVM_BUILDER_ASSERT(struct_type.is_struct())
        LLVM_BUILDER_ASSERT(is_global());
        LLVM_BUILDER_ASSERT(not is_bind());
        const std::string name = struct_type.struct_name();
        auto it = m_structs.try_emplace(name, struct_type, Struct::construct_t{});
        if (not it.second) {
            CODEGEN_PUSH_ERROR(JIT, "Duplicate struct name found:" << name);
            parent.M_mark_error();
            return;
        }
    }
    void add_event(const std::string &e) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not e.empty())
        LLVM_BUILDER_ASSERT(not is_bind());
        auto it = m_event_fns.try_emplace(e, m_runner, e, typename EventFn::construct_t{});
        if (not it.second) {
            // TODO{vibhanshu}: what to do in case of redifinition of event ?
        }
    }
    Struct struct_info(const std::string &name) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        if (m_structs.contains(name)) {
            return m_structs.at(name);
        } else {
            return Struct::null();
        }
    }
    EventFn event_fn_info(const std::string &name) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        if (m_event_fns.contains(name)) {
            return m_event_fns.at(name);
        } else {
            return EventFn::null();
        }
    }
};

//
// Namespace
//
Namespace::Namespace() : BaseT{State::ERROR} {
}

Namespace::Namespace(JustInTimeRunner &runner, const std::string &ns, construct_t)
  : BaseT{State::VALID} {
    CODEGEN_FN
    LLVM_BUILDER_ASSERT(not runner.has_error());
    m_impl = std::make_shared<Impl>(runner, ns);
}

auto Namespace::name() const -> const std::string& {
    if (has_error()) {
        return StringUtil::s_empty;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->name();
}

bool Namespace::is_bind() const {
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_bind();
}

bool Namespace::is_global() const {
    if (has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_global();
}

void Namespace::bind() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->bind(*this);
}

void Namespace::add_struct(const TypeInfo& struct_type) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (struct_type.has_error()) {
        CODEGEN_PUSH_ERROR(JIT, "can't add invalid struct");
        return;
    }
    if (not struct_type.is_struct()) {
        CODEGEN_PUSH_ERROR(JIT, "type not a valid struct");
        return;
    }
    if (m_impl->is_bind()) {
        CODEGEN_PUSH_ERROR(JIT, "Namespace already bound can't add more values");
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->add_struct(*this, struct_type);
}

void Namespace::add_event(const std::string &e) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (e.empty()) {
        CODEGEN_PUSH_ERROR(JIT, "can't add empty event name");
        return;
    }
    if (m_impl->is_bind()) {
        CODEGEN_PUSH_ERROR(JIT, "Namespace already bound can't add more event");
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->add_event(e);
}

auto Namespace::struct_info(const std::string& name) const -> Struct {
    if (has_error()) {
        return Struct::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->struct_info(name);
}

auto Namespace::event_fn_info(const std::string& name) const -> EventFn {
    if (has_error()) {
        return EventFn::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->event_fn_info(name);
}

bool Namespace::operator == (const Namespace& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_impl.get() == rhs.m_impl.get();
}

const Namespace& Namespace::null() {
    static Namespace s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

} // namespace runtime

LLVM_BUILDER_NS_END
