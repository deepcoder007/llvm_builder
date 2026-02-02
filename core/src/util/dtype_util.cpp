//
// Created by vibhanshu on 2023-07-12
//

#include "core/util/dtype_util.h"
#include "core/util/defines.h"
#include "util/string_util.h"
#include "meta/basic_meta.h"
#include <type_traits>

CORE_NS_BEGIN

CORE_SINGLETON_SLOW_VAR_FUNC_IMPL(DtypeUtil)

std::ostream& operator << (std::ostream& os, const dtype_category_t& v) {
    switch (v) {
        case dtype_category_t::unknown_object: os << "unknown_object"; break;
        case dtype_category_t::pod_binary: os << "pod_binary"; break;
        case dtype_category_t::pod_int_signed: os << "pod_int_signed"; break;
        case dtype_category_t::pod_int_unsigned: os << "pod_int_unsigned"; break;
        case dtype_category_t::pod_float: os << "pod_float"; break;
    }
    return os;
}

#define REGISTER_BASE_TYPE(TYPE_NAME, CATEGORY)                                                                        \
    {                                                                                                                  \
        static_assert(not std::is_enum_v<TYPE_NAME>);                                                                  \
        static_assert(not std::is_array_v<TYPE_NAME>);                                                                 \
        base_dtype_t l_type{#TYPE_NAME,                                                                                \
                            #TYPE_NAME,                                                                                \
                            dtype_category_t::CATEGORY,                                                                \
                            sizeof(TYPE_NAME),                                                                         \
                            alignof(TYPE_NAME),                                                                        \
                            0};                                                                                    \
        l_type.set_print_fn([](std::ostream& os, const std::byte* v) {                                                 \
            const TYPE_NAME* l_value_ptr = reinterpret_cast<const TYPE_NAME*>(v);                                      \
            os << *l_value_ptr;                                                                                        \
        });                                                                                                            \
        dtype_util.register_elementary_type(std::move(l_type));                                                                   \
    }                                                                                                                  \
    /**/

#define REGISTER_TYPE(TYPE_NAME, CATEGORY)                                                                             \
    {                                                                                                                  \
        static_assert(std::is_standard_layout_v<TYPE_NAME>);                                                           \
        static_assert(not std::is_pointer_v<TYPE_NAME>);                                                               \
        static_assert(std::is_arithmetic_v<TYPE_NAME>);                                                                \
        REGISTER_BASE_TYPE(TYPE_NAME, pod_##CATEGORY)                                                                  \
    }                                                                                                                  \
    /**/

//
// base_dtype_t
//
base_dtype_t::base_dtype_t()
    : m_name{}
    , m_long_name{}
    , m_category{dtype_category_t::unknown_object} {
}

base_dtype_t::base_dtype_t(const std::string& name,
                        const std::string& long_name,
                        dtype_category_t category,
                        uint32_t size,
                        uint32_t align,
                        uint32_t array_size)
    : m_name{M_strip_type_suffix(name)},
        m_long_name{long_name},
        m_category{category},
        m_size{size},
        m_align{align},
        m_array_size{array_size} {
}

std::string base_dtype_t::M_strip_type_suffix(const std::string& name) {
    if (name.size() > 2 && name.substr(name.size() - 2) == "_t") {
        return name.substr(0, name.size() - 2);
    }
    return name;
}


base_dtype_t base_dtype_t::mk_array(uint32_t num_elements) const {
    if (is_array()) {
        return base_dtype_t{};
    }
    if (num_elements == 0) {
        return base_dtype_t{};
    }
    return base_dtype_t{primary_name(), long_name(), type(), size(), align(), num_elements};
}

base_dtype_t base_dtype_t::from_info(const std::string& name, _type_record record) {
    return base_dtype_t{name, name, record.m_category, record.m_size, record.m_align, record.m_array_size};
}

void base_dtype_t::register_primitives() {
    DtypeUtil& dtype_util = DtypeUtil::singleton();
    if (dtype_util.has_name("int"_cs)) {
        return;
    }
    REGISTER_TYPE(bool, binary)
    REGISTER_TYPE(char, int_signed)
    REGISTER_TYPE(short, int_signed)
    REGISTER_TYPE(int, int_signed)
    REGISTER_TYPE(long, int_signed)
    REGISTER_TYPE(unsigned char, int_unsigned)
    REGISTER_TYPE(unsigned short, int_unsigned)
    REGISTER_TYPE(unsigned int, int_unsigned)
    REGISTER_TYPE(unsigned long, int_unsigned)

    REGISTER_TYPE(int8_t, int_signed)
    REGISTER_TYPE(int16_t, int_signed)
    REGISTER_TYPE(int32_t, int_signed)
    REGISTER_TYPE(int64_t, int_signed)

    REGISTER_TYPE(uint8_t, int_unsigned)
    REGISTER_TYPE(uint16_t, int_unsigned)
    REGISTER_TYPE(uint32_t, int_unsigned)
    REGISTER_TYPE(uint64_t, int_unsigned)

    REGISTER_TYPE(float, float)
    REGISTER_TYPE(double, float)
    REGISTER_TYPE(float32_t, float)
    REGISTER_TYPE(float64_t, float)
}

#undef REGISTER_TYPE
#undef REGISTER_BASE_TYPE

void base_dtype_t::print(std::ostream& os) const {
    os << "{Dtype:name:" << m_name
       << ", long_name:" << m_long_name
       << ", category:" << m_category
       << ", size:" << (uint32_t)m_size
       << ", align:" << (uint32_t)m_align
       << ", is_array:" << is_array()
       << ", array_size:" << m_array_size
       << "}";
}

void base_dtype_t::print_value(std::ostream& os, const std::byte* v) const {
    if (m_print_fn) {
        m_print_fn(os, v);
    } else {
        os << "<printer-not-found>";
    }
}

template <>
std::string base_dtype_t::M_primitive_type_name<int8_t>() {
    return "int8_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<uint8_t>() {
    return "uint8_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<int16_t>() {
    return "int16_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<uint16_t>() {
    return "uint16_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<int32_t>() {
    return "int32_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<uint32_t>() {
    return "uint32_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<int64_t>() {
    return "int64_t";
}

template <>
std::string base_dtype_t::M_primitive_type_name<uint64_t>() {
    return "uint64_t";
}

//
// DtypeUtil
//
DtypeUtil::DtypeUtil() = default;

void DtypeUtil::print(std::ostream& os) const {
    os << "DtypeUtil:{";
    separator_t sep{", "_cs};
    uint32_t key_count = 0;
    for (auto it = s_types.begin(); it != s_types.end(); ++it, ++key_count) {
        os << sep << " [" << key_count << "]" << it->first << "=" << it->second;
    }
    os << "}";
}

auto DtypeUtil::from_name(CString name) -> base_dtype_t {
    if (s_types.contains(name.str())) {
        return s_types.at(name.str());
    } else {
        return base_dtype_t{};
    }
}

auto DtypeUtil::from_array_name(CString base_type, uint32_t num_elements) -> base_dtype_t {
    base_dtype_t l_base_dtype_obj = from_name(base_type);
    return l_base_dtype_obj.mk_array(num_elements);
}

auto DtypeUtil::has_name(CString name) -> bool {
    return s_types.contains(name.str());
}

void DtypeUtil::register_elementary_type(base_dtype_t&& type) {
    const std::string l_name = type.primary_name();
    s_types.try_emplace(l_name, std::move(type));
}

[[gnu::constructor]] void _register_primitives() {
    base_dtype_t::register_primitives();
}

CORE_NS_END
