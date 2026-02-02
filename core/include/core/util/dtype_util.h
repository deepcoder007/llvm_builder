//
// Created by vibhanshu on 2023-06-11
//

#ifndef CORE_DTYPE_UTIL_H
#define CORE_DTYPE_UTIL_H

#include "core/util/defines.h"
#include "core/meta/noncopyable.h"
#include "core/util/cstring.h"
#include <type_traits>
#include <functional>

CORE_NS_BEGIN

enum class dtype_category_t : uint32_t {
    unknown_object,
    pod_binary,
    pod_int_signed,
    pod_int_unsigned,
    pod_float
};

std::ostream& operator << (std::ostream& os, const dtype_category_t& v);

struct _type_record {
    dtype_category_t m_category;
    uint32_t m_size;
    uint32_t m_align;
    uint32_t m_array_size;
public:
    bool is_unknown() const {
        return m_category == dtype_category_t::unknown_object;
    }
    bool is_binary() const {
        return m_category == dtype_category_t::pod_binary;
    }
    bool is_int_signed() const {
        return m_category == dtype_category_t::pod_int_signed;
    }
    bool is_int_unsigned() const {
        return m_category == dtype_category_t::pod_int_unsigned;
    }
    bool is_float() const {
        return m_category == dtype_category_t::pod_float;
    }
    uint32_t size() const {
        return m_size;
    }
    uint32_t align() const {
        return m_align;
    }
    uint32_t array_size() const {
        return m_array_size;
    }
};
static_assert(sizeof(_type_record) == 16u);

class base_dtype_t {
public:
    using print_fn_t = std::function<void(std::ostream& os, const std::byte* v)>;
  private:
    const std::string m_name;
    const std::string m_long_name;
    const dtype_category_t m_category;
    const uint32_t m_size = 0;
    const uint32_t m_align = 0;
    const uint32_t m_array_size = 0;
    print_fn_t m_print_fn;
public:
    explicit base_dtype_t();
protected:
    explicit base_dtype_t(const std::string& name,
                          const std::string& long_name,
                          dtype_category_t category,
                          uint32_t size,
                          uint32_t align,
                          uint32_t array_size);

public:
    ~base_dtype_t() = default;
public:
    base_dtype_t(const base_dtype_t&) = default;
    base_dtype_t(base_dtype_t&&) = default;
    base_dtype_t& operator=(const base_dtype_t&) = delete;
    base_dtype_t& operator=(base_dtype_t&&) = delete;
public:
    bool is_valid() const {
        return m_category != dtype_category_t::unknown_object;
    }
    const std::string& primary_name() const {
        return m_name;
    }
    const std::string& long_name() const {
        return m_long_name;
    }
    dtype_category_t type() const {
        return m_category;
    }
    bool is_binary() const {
        return m_category == dtype_category_t::pod_binary;
    }
    bool is_int_signed() const {
        return m_category == dtype_category_t::pod_int_signed;
    }
    bool is_int_unsigned() const {
        return m_category == dtype_category_t::pod_int_unsigned;
    }
    bool is_float() const {
        return m_category == dtype_category_t::pod_float;
    }
    uint32_t size() const {
        return m_size;
    }
    uint32_t align() const {
        return m_align;
    }
    bool is_array() const {
        return m_array_size > 0;
    }
    uint32_t array_size() const {
        return m_array_size;
    }
    base_dtype_t mk_array(uint32_t num_elements) const;
    bool operator==(const base_dtype_t& o) const {
        if (this == &o) {
            return true;
        }
        if (m_name == o.m_name and m_long_name == o.m_long_name
            and m_category == o.m_category and m_size == o.m_size
            and m_align == o.m_align and m_array_size == o.m_array_size) {
            return true;
        }
        return false;
    }
    void set_print_fn(print_fn_t&& fn) {
        m_print_fn = std::move(fn);
    }
    void print_value(std::ostream& os, const std::byte* v) const;
    base_dtype_t* to_heap() const {
        return new base_dtype_t{*this};
    }
    _type_record encode_info() const {
        return _type_record{m_category, m_size, m_align, m_array_size};
    }
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(base_dtype_t)
public:
    template <typename T>
    static base_dtype_t from_type();
    static base_dtype_t from_info(const std::string& name, _type_record record);
    static void register_primitives();
private:
    template <typename T>
    static std::string M_primitive_type_name();
    static std::string M_strip_type_suffix(const std::string& name);
};

class DtypeUtil : meta::noncopyable {
    std::unordered_map<std::string, base_dtype_t> s_types;
public:
    base_dtype_t from_name(CString name);
    base_dtype_t from_array_name(CString base_type, uint32_t num_elements);
    base_dtype_t from_name(const std::string& name) {
        return from_name(CString{name});
    }
    base_dtype_t from_array_name(const std::string& base_type, uint32_t num_elements) {
        return from_array_name(CString{base_type}, num_elements);
    }
    bool has_name(CString name);
    void register_elementary_type(base_dtype_t&& type);
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(DtypeUtil)
private:                                                                                                             \
    explicit DtypeUtil();                                                                                                  \
public:                                                                                                              \
    static DtypeUtil& singleton();                                                                                         \
};

template <typename T>
struct is_std_array : std::false_type {
};

template <typename T, size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {
};

template <typename T>
auto base_dtype_t::from_type() -> base_dtype_t {
    static_assert(std::is_standard_layout_v<T>);
    static_assert(not std::is_pointer_v<T>);
    if constexpr (std::is_arithmetic_v<T>) {
        return DtypeUtil::singleton().from_name(M_primitive_type_name<T>());
    } else if constexpr(is_std_array<T>::value) {
        using element_t = typename T::value_type;
        static_assert(std::is_standard_layout_v<element_t>);
        enum : size_t {
            c_num_array_elements = sizeof(T) / sizeof(element_t)
        };
        return DtypeUtil::singleton().from_array_name(M_primitive_type_name<element_t>(), c_num_array_elements);
    } else if constexpr(std::is_array_v<T>) {
        static_assert(std::rank_v<T> == 1);
        using element_t = std::remove_extent_t<T>;
        static_assert(std::is_standard_layout_v<element_t>);
        enum : size_t {
            c_num_array_elements = std::extent_v<T>
        };
        return DtypeUtil::singleton().from_array_name(M_primitive_type_name<element_t>(), c_num_array_elements);
    } else {
        return base_dtype_t{};
    }
}

CORE_NS_END

#endif
