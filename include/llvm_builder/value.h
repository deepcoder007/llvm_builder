//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_LLVM_VALUE_H_
#define LLVM_BUILDER_LLVM_VALUE_H_

#include "llvm_builder/defines.h"
#include "llvm_builder/util/object.h"
#include "type.h"

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>

namespace llvm {
    class Value;
};

LLVM_BUILDER_NS_BEGIN

class Function;
class CodeSection;

// TODO{vibhanshu}: build this tag info infra to debug from binary to the source code
class TagInfo {
    enum : uint8_t {
        c_delim = '|'
    };
    std::vector<std::string> m_values;
public:
    explicit TagInfo();
    explicit TagInfo(const std::string& value);
    explicit TagInfo(std::vector<std::string>&& value);
    TagInfo(const TagInfo&) = default;
    TagInfo(TagInfo&&) = default;
    TagInfo& operator=(const TagInfo&) = default;
    TagInfo& operator=(TagInfo&&) = default;
public:
    bool contains(std::string_view v) const;
    void add_entry(const std::string& v);
    TagInfo set_union(const TagInfo& o) const;
};

// TODO{vibhanshu}: attach a value to a code-section and develop benchmarks over it
class ValueInfo : public _BaseObject<ValueInfo> {
    using BaseT = _BaseObject<ValueInfo>;
private:
    TypeInfo m_type_info;
    TagInfo m_tag_info;
    llvm::Value* m_raw_value = nullptr;
public:
    explicit ValueInfo(const TypeInfo& type_info, const TagInfo& tag_info, llvm::Value* raw_value);
    explicit ValueInfo();
    ~ValueInfo();
public:
    bool is_valid() const {
        return m_raw_value != nullptr and m_type_info.is_valid();
    }
    bool has_tag(std::string_view v) const;
    void add_tag(const std::string &v);
    void add_tag(const TagInfo &o);
    const TypeInfo& type() const {
        return m_type_info;
    }
    const TagInfo& tag_info() const {
        return m_tag_info;
    }
    [[nodiscard]]
    ValueInfo cast(TypeInfo target_type) const;
#define MK_BINARY_FN(FN_NAME)                                              \
    [[nodiscard]]                                                          \
    ValueInfo FN_NAME(ValueInfo v2) const;                                 \
    /**/
    FOR_EACH_BINARY_OP(MK_BINARY_FN) 
#undef MK_BINARY_FN
    [[nodiscard]]
    ValueInfo cond(ValueInfo then_value, ValueInfo else_value) const;
    void push(std::string_view name) const;
public:
    [[nodiscard]]
    ValueInfo operator+(const ValueInfo &v2) const {
        return add(v2);
    }
    [[nodiscard]]
    ValueInfo operator-(const ValueInfo &v2) const {
        return sub(v2);
    }
    [[nodiscard]]
    ValueInfo operator*(const ValueInfo &v2) const {
        return mult(v2);
    }
    [[nodiscard]]
    ValueInfo operator/(const ValueInfo &v2) const {
        return div(v2);
    }
    [[nodiscard]]
    ValueInfo operator%(const ValueInfo &v2) const {
        return remainder(v2);
    }
    [[nodiscard]]
    ValueInfo operator<(const ValueInfo &v2) const {
        return less_than(v2);
    }
    [[nodiscard]]
    ValueInfo operator<=(const ValueInfo &v2) const {
        return less_than_equal(v2);
    }
    [[nodiscard]]
    ValueInfo operator>(const ValueInfo &v2) const {
        return greater_than(v2);
    }
    [[nodiscard]]
    ValueInfo operator>=(const ValueInfo &v2) const {
        return greater_than_equal(v2);
    }
public:
    bool equals_type(const ValueInfo& o) const {
        return m_type_info == o.m_type_info;
    }
    bool equals_type(TypeInfo t) const {
        return m_type_info == t;
    }
    llvm::Value* native_value() const {
        return m_raw_value;
    }
    bool operator == (const ValueInfo& rhs) const;
public:
    void store(const ValueInfo& value) const;
    [[nodiscard]]
    ValueInfo load() const;
    [[nodiscard]]
    ValueInfo entry(uint32_t i) const;
    [[nodiscard]]
    ValueInfo entry(const ValueInfo& i) const;
    [[nodiscard]]
    ValueInfo field(const std::string& s) const;
    [[nodiscard]]
    ValueInfo load_vector_entry(uint32_t i) const;
    ValueInfo store_vector_entry(uint32_t i, ValueInfo value) const;
    [[nodiscard]]
    ValueInfo load_vector_entry(const ValueInfo& idx_v) const;
    ValueInfo store_vector_entry(const ValueInfo& idx_v, ValueInfo value) const;
public:
    static ValueInfo null();
    template <typename T>
    [[nodiscard]]
    static ValueInfo from_constant(T v);
    [[nodiscard]]
    static ValueInfo mk_pointer(TypeInfo type);
    [[nodiscard]]
    static ValueInfo mk_null_pointer();
    [[nodiscard]]
    static ValueInfo mk_array(TypeInfo type);
    [[nodiscard]]
    static ValueInfo mk_struct(TypeInfo type);
    [[nodiscard]]
    static ValueInfo calc_struct_size(TypeInfo type);
    [[nodiscard]]
    static ValueInfo calc_struct_field_count(TypeInfo type);
    [[nodiscard]]
    static ValueInfo calc_struct_field_offset(TypeInfo type, ValueInfo idx);
private:
    ValueInfo M_load_vector_entry(llvm::Value* idx_value) const;
    ValueInfo M_store_vector_entry(llvm::Value* idx_value, const ValueInfo& value) const;
};


LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_VALUE_H_
