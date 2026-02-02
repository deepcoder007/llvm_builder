//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/defines.h"
#include "llvm_builder/type.h"
#include "llvm_builder/value.h"
#include "llvm_builder/analyze.h"
#include "llvm/context_impl.h"
#include "llvm_builder/function.h"
#include "llvm_builder/module.h"
#include "util/debug.h"
#include "util/string_util.h"
#include "llvm_builder/meta/noncopyable.h"
#include "ext_include.h"

LLVM_BUILDER_NS_BEGIN

//
// TagInfo
//
TagInfo::TagInfo() = default;

TagInfo::TagInfo(const std::string& value) {
    const std::vector<CString> l_values = CString{value}.split(c_delim);
    for (CString v : l_values) {
        m_values.emplace_back(v.str());
    }
}

TagInfo::TagInfo(std::vector<std::string>&& value) : m_values{std::move(value)} {
    for (const std::string& v : m_values) {
        add_entry(v);
    }
}

bool TagInfo::contains(CString v) const {
    for (const std::string& e : m_values) {
        if (v.equals(e)) {
            return true;
        }
    }
    return false;
}

void TagInfo::add_entry(const std::string& v) {
    CString c_str{v};
    if (c_str.contains(c_delim)) {
        for (CString v2 : c_str.split(c_delim)) {
            m_values.emplace_back(v2);
        }
    } else {
        m_values.emplace_back(v);
    }
}

TagInfo TagInfo::set_union(const TagInfo& o) const {
    TagInfo l_res = *this;
    for (const std::string& v: o.m_values) {
        if (std::find(m_values.begin(), m_values.end(), v) == m_values.end()) {
            l_res.m_values.emplace_back(v);
        }
    }
    return l_res;
}

//
// ValuePtrInfo
//
class ValuePtrInfo {
    friend class ValueInfo;
private:
    const TypeInfo m_orig_type;
    llvm::Value* const m_raw_value = nullptr;
    TypeInfo m_type;
    std::array<ValueInfo, 2u> m_index_list;
private:
    explicit ValuePtrInfo() = default;
    explicit ValuePtrInfo(const ValueInfo& v);
public:
    ~ValuePtrInfo() = default;
public:
    ValueInfo entry_ptr(uint32_t i) const;
    ValueInfo entry_ptr(const ValueInfo &idx_v) const {
        CODEGEN_FN
        return M_array_entry_ptr(idx_v);
    }
    ValueInfo entry_ptr(const std::string &field) const;
private:
    ValueInfo M_array_entry_ptr(const ValueInfo &idx_v) const {
        CODEGEN_FN
        if (idx_v.has_error()) {
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "try to access an invalid value index");
            return ValueInfo::null();
        }
        if (not m_type.is_array() and not m_type.is_vector()) {
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "This is not an array pointer");
            return ValueInfo::null();
        }
        ValuePtrInfo l_res = *this;
        l_res.m_type = m_type.base_type();
        l_res.m_index_list[1] = idx_v;
        return l_res.M_deref();
    }
    ValueInfo M_deref(const std::string &op_name = std::string{}) const {
        CODEGEN_FN
        if (CursorContextImpl::has_value()) {
            llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
            std::vector<llvm::Value*> l_raw_index_list;
            for (const ValueInfo& v : m_index_list) {
                l_raw_index_list.emplace_back(v.native_value());
            }
            llvm::Value* l_entry_idx = l_cursor.CreateGEP(m_orig_type.base_type().native_value(), m_raw_value, l_raw_index_list, llvm::Twine{op_name, "_index"});
            return ValueInfo{m_type.pointer_type(), TagInfo{}, l_entry_idx};
        } else {
            return ValueInfo::null();
        }
    }
};

//
// ValueInfo
//
ValueInfo::ValueInfo() : BaseT{State::ERROR} {
}

ValueInfo::ValueInfo(const TypeInfo& type_info,
                     const TagInfo& tag_info,
                     llvm::Value* raw_value)
    : BaseT{State::VALID}, m_type_info{type_info}, m_tag_info{tag_info}, m_raw_value{raw_value} {
    CODEGEN_FN
    if (m_raw_value == nullptr) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "ValueInfo got null object");
        M_mark_error();
        return;
    }
    if (m_type_info.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "Can't define value with invalid type");
        M_mark_error();
        return;
    }
    if (not m_type_info.check_sync(*this)) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "Type:[" << m_type_info.short_name() << "], does not match value:[" << this->type().short_name() << "]");
        M_mark_error();
        return;
    }
    object::Counter::singleton().on_new(object::Callback::object_t::VALUE, (uint64_t)this, "");
}
ValueInfo::~ValueInfo() {
    // object::Counter::singleton().on_delete(object::Callback::object_t::VALUE, (uint64_t)this, "");
}

bool ValueInfo::has_tag(CString v) const {
    return m_tag_info.contains(v);
}

bool ValueInfo::has_tag(const std::string& v) const {
    return has_tag(CString{v});
}

void ValueInfo::add_tag(const std::string& v) {
    m_tag_info.add_entry(v);
}

void ValueInfo::add_tag(const TagInfo& o) {
    m_tag_info = m_tag_info.set_union(o);
}

bool ValueInfo::operator == (const ValueInfo& v2) const {
    if (has_error() and v2.has_error()) {
        return true;
    }
    if (has_error() or v2.has_error()) {
        return false;
    }
    return m_raw_value == v2.m_raw_value and m_type_info == v2.m_type_info;
}

ValueInfo ValueInfo::cast(TypeInfo target_type, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    return target_type.type_cast(*this, op_name);
}

#define _BASE_BINARY_OP_IMPL_FN(FN_NAME, RETURN)                                                 \
    ValueInfo ValueInfo:: FN_NAME (ValueInfo v2, const std::string& op_name) const {             \
    CODEGEN_FN                                                                                   \
    if (has_error()) {                                                                           \
        return ValueInfo::null();                                                                \
    }                                                                                            \
    if (v2.has_error()) {                                                                        \
        return ValueInfo::null();                                                                \
    }                                                                                            \
    if (not equals_type(v2)) {                                                                   \
        M_mark_error();                                                                          \
        CODEGEN_PUSH_ERROR(VALUE_ERROR, #FN_NAME << " can't be defined for different type");     \
        return ValueInfo::null();                                                                \
    }                                                                                            \
    TagInfo l_tag_info = tag_info();                                                             \
    l_tag_info = l_tag_info.set_union(v2.tag_info());                                            \
    ValueInfo r = RETURN . FN_NAME (*this, v2, op_name);                                         \
    r.add_tag(l_tag_info);                                                                       \
    return r;                                                                                    \
}                                                                                                \
/**/

#define BINARY_OP_IMPL_FN(fn_name)     _BASE_BINARY_OP_IMPL_FN(fn_name, type())
#define BINARY_CMP_OP_IMPL_FN(fn_name)  _BASE_BINARY_OP_IMPL_FN(fn_name, TypeInfo::mk_bool())

FOR_EACH_ARITHEMATIC_OP(BINARY_OP_IMPL_FN)
FOR_EACH_LOGICAL_OP(BINARY_CMP_OP_IMPL_FN)

#undef BINARY_OP_IMPL_FN
#undef BINARY_CMP_OP_IMPL_FN
#undef _BASE_BINARY_OP_IMPL_FN

ValueInfo ValueInfo::cond(ValueInfo then_value, ValueInfo else_value, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (then_value.has_error() or else_value.has_error()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define cond operation over invalid values");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        if (not type().is_boolean()) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define cond operation for non boolean type");
            return ValueInfo::null();
        }
        if (not then_value.equals_type(else_value)) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "then and else value not of same type");
            return ValueInfo::null();
        }
        TagInfo l_tag_info = tag_info().set_union(then_value.tag_info()).set_union(else_value.tag_info());
        llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
        llvm::Value* l_entry = l_cursor.CreateSelect(m_raw_value, then_value.m_raw_value, else_value.m_raw_value, op_name);
        return ValueInfo{then_value.type(), l_tag_info, l_entry};
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

void ValueInfo::push(CString name) const {
    if (has_error()) {
        return;
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store/push an empty string as name");
    } else {
        CodeSectionContext::push(name, *this);
    }
}

ValueInfo ValueInfo::load(const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        if (not this->type().is_pointer()) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define load underlying operation for non pointer type");
            return ValueInfo::null();
        }
        llvm::Value* l_load_value = CursorContextImpl::builder().CreateLoad(type().base_type().native_value(), m_raw_value, op_name);
        return ValueInfo{type().base_type(), m_tag_info, l_load_value};
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

ValueInfo ValueInfo::entry(uint32_t i) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-pointer type");
        return ValueInfo::null();
    }
    if (not type().base_type().is_array()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-array pointer type");
        return ValueInfo::null();
    }
    return ValuePtrInfo{*this}.entry_ptr(i);
}

ValueInfo ValueInfo::entry(const ValueInfo& i) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-pointer type");
        return ValueInfo::null();
    }
    if (i.has_error()) {
        M_mark_error();
        return ValueInfo::null();
    }
    if (not type().base_type().is_array()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-array pointer type");
        return ValueInfo::null();
    }
    return ValuePtrInfo{*this}.entry_ptr(i);
}

ValueInfo ValueInfo::field(const std::string& s) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define struct-field access operation for non-pointer type");
        return ValueInfo::null();
    }
    if (s.empty()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define struct-field access operation for emptry field name");
        return ValueInfo::null();
    }
    if (not type().base_type().is_struct()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define struct-field access operation for non-struct pointer type");
        return ValueInfo::null();
    }
    return ValuePtrInfo{*this}.entry_ptr(s);
}


[[nodiscard]]
ValueInfo ValueInfo::load_vector_entry(uint32_t i) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    return load_vector_entry(i, LLVM_BUILDER_CONCAT << "load_vector_" << i);
}

ValueInfo ValueInfo::store_vector_entry(uint32_t i, ValueInfo value) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    return store_vector_entry(i, value, LLVM_BUILDER_CONCAT << "store_vector_" << i);
}

ValueInfo ValueInfo::load_vector_entry(const ValueInfo& idx_v, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    return M_load_vector_entry(idx_v.native_value(), op_name);
}

ValueInfo ValueInfo::store_vector_entry(const ValueInfo& idx_v, ValueInfo value, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    return M_store_vector_entry(idx_v.native_value(), value, op_name);
}

ValueInfo ValueInfo::load_vector_entry(const ValueInfo& idx_v) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    return M_load_vector_entry(idx_v.native_value(), LLVM_BUILDER_CONCAT << "load_vector_idx");
}

ValueInfo ValueInfo::store_vector_entry(const ValueInfo& idx_v, ValueInfo value) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    return M_store_vector_entry(idx_v.native_value(), value, LLVM_BUILDER_CONCAT << "load_vector_idx");
}

ValueInfo ValueInfo::M_load_vector_entry(llvm::Value* idx_value, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_value == nullptr) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't load value from invalid location");
        return ValueInfo::null();
    }
    if (not type().is_vector()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define vector entry access operation for non-vector type");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
        llvm::Value* l_entry = l_cursor.CreateExtractElement(m_raw_value, idx_value, op_name);
        return ValueInfo{type().base_type(), m_tag_info, l_entry};
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

ValueInfo ValueInfo::M_store_vector_entry(llvm::Value* idx_value, const ValueInfo& value, const std::string& op_name) const {
    CODEGEN_FN
    if (idx_value == nullptr) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store value to invalid location");
        return ValueInfo::null();
    }
    if (value.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store invalid value");
        return ValueInfo::null();
    }
    if (not type().is_vector()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define vector entry access operation for non-vector type");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
        llvm::Value* l_ret = l_cursor.CreateInsertElement(m_raw_value, value.native_value(), idx_value, op_name);
        return ValueInfo{type(), m_tag_info, l_ret};
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

void ValueInfo::store(const ValueInfo& value) const {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (value.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store invalid value");
        return;
    }
    if (CursorContextImpl::has_value()) {
        if (not type().is_pointer()) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define store operation for non pointer type");
            return;
        }
        if (not type().base_type().is_scalar() and not type().base_type().is_vector()) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define store operation for non scalar(bool, int, float) and non-vector object");
            return;
        }
        if (type().base_type() != value.type()) {
            M_mark_error();
            CODEGEN_PUSH_ERROR(VALUE_ERROR, "type mis-match between pointer and value type: expected:" << type().base_type().short_name() << " found:" << value.type().short_name());
            return;
        }
        CursorContextImpl::builder().CreateStore(value.m_raw_value, m_raw_value);
    } else {
        M_mark_error();
    }
}

auto ValueInfo::null() -> ValueInfo {
    // static ValueInfo s_null_value{};
    // TODO{vibhanshu}: is using null pointer the right thing to do?
    static ValueInfo s_null_value{};
    LLVM_BUILDER_ASSERT(s_null_value.has_error());
    return s_null_value;
}

template <>
ValueInfo ValueInfo::from_constant<float32_t>(float32_t v) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_float32();
        llvm::Constant* l_value = llvm::ConstantFP::get(CursorContextImpl::ctx(), llvm::APFloat(v));
        return ValueInfo{l_type_info, TagInfo{}, l_value};
    } else {
        return ValueInfo::null();
    }
}

template <>
ValueInfo ValueInfo::from_constant<float64_t>(float64_t v) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_float64();
        llvm::Constant* l_value = llvm::ConstantFP::get(CursorContextImpl::ctx(), llvm::APFloat(v));
        return ValueInfo{l_type_info, TagInfo{}, l_value};
    } else {
        return ValueInfo::null();
    }
}

template <>
ValueInfo ValueInfo::from_constant<bool>(bool v) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_bool();
        llvm::Constant* l_value = nullptr;
        if (v) {
            l_value = llvm::ConstantInt::getTrue(CursorContextImpl::ctx());
        } else {
            l_value = llvm::ConstantInt::getFalse(CursorContextImpl::ctx());
        }
        return ValueInfo{l_type_info, TagInfo{}, l_value};
    } else {
        return ValueInfo::null();
    }
}

#define FROM_CONSTANT_DEF(NUM_BIT)                                                                                           \
    template <>                                                                                                              \
    ValueInfo ValueInfo::from_constant<int##NUM_BIT##_t>(int##NUM_BIT##_t v) {                                               \
        CODEGEN_FN                                                                                                           \
        if (CursorContextImpl::has_value()) {                                                                                \
            TypeInfo l_type_info = TypeInfo::mk_int##NUM_BIT();                                                              \
            llvm::IntegerType* l_int_type = llvm::IntegerType::get(CursorContextImpl::ctx(), NUM_BIT##u);                    \
            llvm::ConstantInt* l_value = llvm::ConstantInt::getSigned(l_int_type, v);                                        \
            return ValueInfo{l_type_info, TagInfo{}, l_value};                                                               \
        } else {                                                                                                             \
            return ValueInfo::null();                                                                                        \
        }                                                                                                                    \
    }                                                                                                                        \
/**/

FROM_CONSTANT_DEF(8)
FROM_CONSTANT_DEF(16)
FROM_CONSTANT_DEF(32)
FROM_CONSTANT_DEF(64)

#undef FROM_CONSTANT_DEF

#define FROM_UCONSTANT_DEF(NUM_BIT)                                                                                          \
    template <>                                                                                                              \
    ValueInfo ValueInfo::from_constant<uint##NUM_BIT##_t>(uint##NUM_BIT##_t v) {                                             \
        CODEGEN_FN                                                                                                           \
        if (CursorContextImpl::has_value()) {                                                                                \
            TypeInfo l_type_info = TypeInfo::mk_uint##NUM_BIT();                                                             \
            llvm::IntegerType* l_int_type = llvm::IntegerType::get(CursorContextImpl::ctx(), NUM_BIT##u);                    \
            llvm::ConstantInt* l_value = llvm::ConstantInt::get(l_int_type, v, false);                                       \
            return ValueInfo{l_type_info, TagInfo{}, l_value};                                                               \
        } else {                                                                                                             \
            return ValueInfo::null();                                                                                        \
        }                                                                                                                    \
    }                                                                                                                        \
/**/

FROM_UCONSTANT_DEF(8)
FROM_UCONSTANT_DEF(16)
FROM_UCONSTANT_DEF(32)
FROM_UCONSTANT_DEF(64)

#undef FROM_UCONSTANT_DEF

ValueInfo ValueInfo::mk_pointer(TypeInfo type, const std::string& name) {
    CODEGEN_FN
    if (type.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define pointer for invalid type");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        llvm::AllocaInst* l_inst = CursorContextImpl::builder().CreateAlloca(type.native_value(), nullptr, name);
        LLVM_BUILDER_ASSERT(l_inst != nullptr);
        [[maybe_unused]] llvm::PointerType* ptr_type = l_inst->getType();
        LLVM_BUILDER_ASSERT(ptr_type != nullptr);
        LLVM_BUILDER_ASSERT(ptr_type->isLoadableOrStorableType(type.native_value()));
        return ValueInfo{type.pointer_type(), TagInfo{}, l_inst};
    } else {
        return ValueInfo::null();
    }
}

ValueInfo ValueInfo::mk_null_pointer() {
    CODEGEN_FN
    TypeInfo l_ptr = TypeInfo::mk_void().pointer_type();
    LLVM_BUILDER_ASSERT(llvm::PointerType::classof(l_ptr.native_value()));
    llvm::PointerType* l_ptr_type = (llvm::PointerType*)l_ptr.native_value();
    llvm::Value* l_null_ptr = llvm::ConstantPointerNull::get(l_ptr_type);
    return ValueInfo{l_ptr, TagInfo{}, l_null_ptr};
}

ValueInfo ValueInfo::mk_array(TypeInfo type, const std::string& name) {
    CODEGEN_FN
    if (type.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array for invalid type");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        if (not type.is_array()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type not a array");
            return ValueInfo::null();
        }
        llvm::AllocaInst* l_inst = CursorContextImpl::builder().CreateAlloca(type.native_value(), nullptr, name);
        LLVM_BUILDER_ASSERT(l_inst != nullptr);
        [[maybe_unused]] llvm::PointerType* ptr_type = l_inst->getType();
        LLVM_BUILDER_ASSERT(ptr_type != nullptr);
        LLVM_BUILDER_ASSERT(ptr_type->isLoadableOrStorableType(type.native_value()));
        return ValueInfo{type.pointer_type(), TagInfo{}, l_inst};
    } else {
        return ValueInfo::null();
    }
}

ValueInfo ValueInfo::mk_struct(TypeInfo type, const std::string& name) {
    CODEGEN_FN
    if (type.has_error()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define struct for invalid type");
        return ValueInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        if (not type.is_struct()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type not a struct");
            return ValueInfo::null();
        }
        llvm::AllocaInst* l_inst = CursorContextImpl::builder().CreateAlloca(type.native_value(), nullptr, name);
        LLVM_BUILDER_ASSERT(l_inst != nullptr);
        [[maybe_unused]] llvm::PointerType* ptr_type = l_inst->getType();
        LLVM_BUILDER_ASSERT(ptr_type != nullptr);
        LLVM_BUILDER_ASSERT(ptr_type->isLoadableOrStorableType(type.native_value()));
        return ValueInfo{type.pointer_type(), TagInfo{}, l_inst};
    } else {
        return ValueInfo::null();
    }
}

ValueInfo ValueInfo::calc_struct_size(TypeInfo type) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        if (not type.is_struct()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type not a struct");
            return ValueInfo::null();
        }
        return ValueInfo::from_constant<int32_t>((int32_t)type.size_in_bytes());
    } else {
        return ValueInfo::from_constant<int32_t>(-1);
    }
}

ValueInfo ValueInfo::calc_struct_field_count(TypeInfo type) {
    if (type.is_struct()) {
        return ValueInfo::from_constant<int32_t>((int32_t)type.num_elements());
    } else {
        return ValueInfo::from_constant<int32_t>(-1);
    }
}

ValueInfo ValueInfo::calc_struct_field_offset(TypeInfo type, ValueInfo idx) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        if (not type.is_struct()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type not a struct");
            return ValueInfo::null();
        }
        if (not idx.type().is_integer()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Index needs to be of int type");
            return ValueInfo::null();
        }
        const int32_t l_num_fields = type.num_elements();
        ValueInfo l_offset_result = ValueInfo::from_constant(-1);
        for (int32_t i = 0; i != l_num_fields; ++i) {
            ValueInfo l_curr_iter_idx = ValueInfo::from_constant(i);
            ValueInfo l_match_idx = l_curr_iter_idx.equal(idx, LLVM_BUILDER_CONCAT << "eq_field_idx_" << i);
            ValueInfo l_offset_curr = ValueInfo::from_constant(static_cast<int32_t>(type[i].offset()));
            l_offset_result = l_match_idx.cond(l_offset_curr, l_offset_result, LLVM_BUILDER_CONCAT << "field_offset_" << i);
        }
        return l_offset_result;
    } else {
        return ValueInfo::null();
    }
}

//
// ValuePtrInfo
//
ValuePtrInfo::ValuePtrInfo(const ValueInfo& v)
    : m_orig_type{v.type()}, m_raw_value{v.native_value()} {
    LLVM_BUILDER_ASSERT(not v.has_error());
    LLVM_BUILDER_ASSERT(m_raw_value != nullptr);
    LLVM_BUILDER_ASSERT(m_orig_type.is_pointer());
    m_type = m_orig_type.base_type();
    m_index_list[0] = ValueInfo::from_constant(0);
}

ValueInfo ValuePtrInfo::entry_ptr(uint32_t i) const {
    CODEGEN_FN
    if (i >= m_type.num_elements()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "Array is of size: " << m_type.num_elements() << ", can't access element:" << i);
    }
    return M_array_entry_ptr(ValueInfo::from_constant(i));
}

ValueInfo ValuePtrInfo::entry_ptr(const std::string &field) const {
    CODEGEN_FN
    if (field.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't access an empty name field");
        return ValueInfo::null();
    }
    if (not m_type.is_struct()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "This is not a struct");
        return ValueInfo::null();
    }
    TypeInfo::field_entry_t field_entry = m_type[field];
    ValuePtrInfo l_res = *this;
    l_res.m_type = field_entry.type();
    l_res.m_index_list[1] = ValueInfo::from_constant((int32_t)field_entry.idx());
    return l_res.M_deref();
}

//
// ValueInfo
//
ValueInfo ValueInfo::load_vector_entry(uint32_t i, const std::string& op_name) const {
    if (has_error()) {
        return ValueInfo::null();
    }
    return M_load_vector_entry(ValueInfo::from_constant((int32_t)i).native_value(), op_name);
}

ValueInfo ValueInfo::store_vector_entry(uint32_t i, ValueInfo value, const std::string& op_name) const {
    if (has_error()) {
        return ValueInfo::null();
    }
    return M_store_vector_entry(ValueInfo::from_constant((int32_t)i).native_value(), value, op_name);
}


LLVM_BUILDER_NS_END
