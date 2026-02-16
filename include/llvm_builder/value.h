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
    class Function;
};

LLVM_BUILDER_NS_BEGIN

class Function;
class CodeSection;
class ValuePtrInfo;

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

// TODO{vibhanshu}: cache M_eval() output per code-section to remove redundant computation
class ValueInfo : public _BaseObject<ValueInfo> {
    using BaseT = _BaseObject<ValueInfo>;
    using binary_op_fn_t = llvm::Value* (TypeInfo::*) (llvm::Value*, llvm::Value*) const;
    friend class CodeSection;
    friend class Function;
    struct construct_const_t{};
    struct construct_entry_t{};
    struct construct_binary_op_t{};
    struct construct_fn_t{};
public:
    enum class value_type_t {
        null,
        constant,
        context,
        binary,
        conditional,
        typecast,
        inner_entry,
        load,
        store,
        load_vector_entry,
        store_vector_entry,
        mk_ptr,
        fn_call,
        fn_ptr_call,
    };
    struct Impl;
private:
    std::shared_ptr<Impl> m_impl;
    void M_set_const_value_cache(llvm::Value* v);
private:
    explicit ValueInfo(value_type_t value_type,
                       const TypeInfo& type_info,
                       const std::vector<ValueInfo>& parent);
    explicit ValueInfo(const TypeInfo& type_info, llvm::Value* v, construct_const_t);
    explicit ValueInfo(const ValueInfo& parent, const TypeInfo& entry_type, const ValueInfo& entry_idx, construct_entry_t);
    explicit ValueInfo(const TypeInfo& res_type, const ValueInfo& v1, const ValueInfo& v2, binary_op_fn_t fn, construct_binary_op_t);
    explicit ValueInfo(llvm::Function* fn, const ValueInfo& arg, construct_fn_t);
public:
    explicit ValueInfo();
    ~ValueInfo();
public:
    bool has_tag(std::string_view v) const;
    void add_tag(const std::string &v);
    void add_tag(const TagInfo &o);
    value_type_t value_type() const;
    const TypeInfo& type() const;
    const TagInfo& tag_info() const;
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
    ValueInfo operator+(const ValueInfo& v2) const {
        return add(v2);
    }
    [[nodiscard]]
    ValueInfo operator-(const ValueInfo& v2) const {
        return sub(v2);
    }
    [[nodiscard]]
    ValueInfo operator*(const ValueInfo& v2) const {
        return mult(v2);
    }
    [[nodiscard]]
    ValueInfo operator/(const ValueInfo& v2) const {
        return div(v2);
    }
    [[nodiscard]]
    ValueInfo operator%(const ValueInfo& v2) const {
        return remainder(v2);
    }
    [[nodiscard]]
    ValueInfo operator<(const ValueInfo& v2) const {
        return less_than(v2);
    }
    [[nodiscard]]
    ValueInfo operator<=(const ValueInfo& v2) const {
        return less_than_equal(v2);
    }
    [[nodiscard]]
    ValueInfo operator>(const ValueInfo& v2) const {
        return greater_than(v2);
    }
    [[nodiscard]]
    ValueInfo operator>=(const ValueInfo& v2) const {
        return greater_than_equal(v2);
    }
public:
    bool equals_type(const ValueInfo& o) const;
    bool equals_type(TypeInfo t) const;
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
    [[nodiscard]]
    ValueInfo call_fn() const;
public:
    static ValueInfo null();
    static ValueInfo from_context(const TypeInfo& ctx_type);
    template <typename T>
    [[nodiscard]]
    static ValueInfo from_constant(T v);
    [[nodiscard]]
    static ValueInfo mk_pointer(TypeInfo type);
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
    llvm::Value* M_eval();
    void M_self_intern();
    bool M_is_value_sink() const;
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_VALUE_H_
