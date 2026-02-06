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
#include "util/cstring.h"
#include "meta/noncopyable.h"
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

bool TagInfo::contains(std::string_view v) const {
    CString cv{v};
    for (const std::string& e : m_values) {
        if (cv.equals(e)) {
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
// ValueInfo::from_constant
//
template <>
ValueInfo ValueInfo::from_constant<float32_t>(float32_t v) {
    CODEGEN_FN
    // TODO{vibhanshu}: having constant inplace have drawback that they can't be transported to any other place
    //                  in next iteration, store only the raw c++ value in valueInfo and construct llvm::Value
    //                  object later jsut before using them in M_eval()
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_float32();
        ValueInfo l_ret{value_type_t::constant, l_type_info, std::vector<ValueInfo>{}};
        llvm::Constant* l_value = llvm::ConstantFP::get(CursorContextImpl::ctx(), llvm::APFloat(v));
        l_ret.m_const_value_cache = l_value;
        return l_ret;
    } else {
        return ValueInfo::null();
    }
}

template <>
ValueInfo ValueInfo::from_constant<float64_t>(float64_t v) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_float64();
        ValueInfo l_ret{value_type_t::constant, l_type_info, std::vector<ValueInfo>{}};
        llvm::Constant* l_value = llvm::ConstantFP::get(CursorContextImpl::ctx(), llvm::APFloat(v));
        l_ret.m_const_value_cache = l_value;
        return l_ret;
    } else {
        return ValueInfo::null();
    }
}

template <>
ValueInfo ValueInfo::from_constant<bool>(bool v) {
    CODEGEN_FN
    if (CursorContextImpl::has_value()) {
        TypeInfo l_type_info = TypeInfo::mk_bool();
        ValueInfo l_ret{value_type_t::constant, l_type_info, std::vector<ValueInfo>{}};
        llvm::Constant* l_value = nullptr;
        if (v) {
            l_value = llvm::ConstantInt::getTrue(CursorContextImpl::ctx());
        } else {
            l_value = llvm::ConstantInt::getFalse(CursorContextImpl::ctx());
        }
        l_ret.m_const_value_cache = l_value;
        return l_ret;
    } else {
        return ValueInfo::null();
    }
}

#define FROM_CONSTANT_DEF(NUM_BIT)                                      \
    template <>                                                         \
    ValueInfo ValueInfo::from_constant<int##NUM_BIT##_t>(int##NUM_BIT##_t v) { \
        CODEGEN_FN                                                      \
        if (CursorContextImpl::has_value()) {                       \
            TypeInfo l_type_info = TypeInfo::mk_int##NUM_BIT();     \
            ValueInfo l_ret{value_type_t::constant, l_type_info, std::vector<ValueInfo>{}}; \
            llvm::IntegerType* l_int_type = llvm::IntegerType::get(CursorContextImpl::ctx(), NUM_BIT##u); \
            llvm::ConstantInt* l_value = llvm::ConstantInt::getSigned(l_int_type, v); \
            l_ret.m_const_value_cache = l_value;                    \
            return l_ret;                                           \
        } else {                                                    \
            return ValueInfo::null();                               \
        }                                                           \
    }                                                                   \
/**/

FROM_CONSTANT_DEF(8)
FROM_CONSTANT_DEF(16)
FROM_CONSTANT_DEF(32)
FROM_CONSTANT_DEF(64)

#undef FROM_CONSTANT_DEF

#define FROM_UCONSTANT_DEF(NUM_BIT)                                     \
    template <>                                                         \
    ValueInfo ValueInfo::from_constant<uint##NUM_BIT##_t>(uint##NUM_BIT##_t v) { \
        CODEGEN_FN                                                      \
        if (CursorContextImpl::has_value()) {                       \
            TypeInfo l_type_info = TypeInfo::mk_uint##NUM_BIT();    \
            ValueInfo l_ret{value_type_t::constant, l_type_info, std::vector<ValueInfo>{}}; \
            llvm::IntegerType* l_int_type = llvm::IntegerType::get(CursorContextImpl::ctx(), NUM_BIT##u); \
            llvm::ConstantInt* l_value = llvm::ConstantInt::get(l_int_type, v, false); \
            l_ret.m_const_value_cache = l_value;                        \
            return l_ret;                                               \
        } else {                                                    \
            return ValueInfo::null();                               \
        }                                                           \
    }                                                                   \
/**/

FROM_UCONSTANT_DEF(8)
FROM_UCONSTANT_DEF(16)
FROM_UCONSTANT_DEF(32)
FROM_UCONSTANT_DEF(64)

#undef FROM_UCONSTANT_DEF


//
// ValueInfo
//
ValueInfo::ValueInfo() : BaseT{State::ERROR} {
}

ValueInfo::ValueInfo(value_type_t value_type,
                     const TypeInfo& type_info,
                     const std::vector<ValueInfo>& parent)
    : BaseT{State::VALID}
    , m_value_type{value_type}
    , m_type_info{type_info}
    , m_parent{parent} {
    if (value_type == value_type_t::null) {
        M_mark_error();
        return;
    }
    if (m_type_info.has_error()) {
        M_mark_error();
        return;
    }
    object::Counter::singleton().on_new(object::Callback::object_t::VALUE, (uint64_t)this, "");
}
ValueInfo::~ValueInfo() {
    // object::Counter::singleton().on_delete(object::Callback::object_t::VALUE, (uint64_t)this, "");
}

bool ValueInfo::has_tag(std::string_view v) const {
    return m_tag_info.contains(v);
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
    // TODO{vibhanshu}: for simplicity 2 values are never equal,
    //                  we will want to implement dedup in near future
    return false;
}

ValueInfo ValueInfo::cast(TypeInfo target_type) const {
    CODEGEN_FN
    if (has_error() or target_type.has_error()) {
        return ValueInfo::null();
    }
    ValueInfo r{value_type_t::typecast, target_type, std::vector<ValueInfo>{{*this}}};
    r.add_tag(tag_info());
    return r;
}

#define _BASE_BINARY_OP_IMPL_FN(FN_NAME, RETURN)                                                 \
    ValueInfo ValueInfo:: FN_NAME (ValueInfo v2) const {                                         \
    CODEGEN_FN                                                                                   \
    if (has_error() or v2.has_error()) {                                                         \
        return ValueInfo::null();                                                                \
    }                                                                                            \
    if (not equals_type(v2)) {                                                                   \
        M_mark_error();                                                                          \
        CODEGEN_PUSH_ERROR(VALUE_ERROR, #FN_NAME << " can't be defined for different type");     \
        return ValueInfo::null();                                                                \
    }                                                                                            \
    TagInfo l_tag_info = tag_info();                                                             \
    l_tag_info = l_tag_info.set_union(v2.tag_info());                                            \
    ValueInfo r{value_type_t::binary, RETURN, std::vector<ValueInfo>{{*this, v2}}};              \
    r.m_binary_op = &TypeInfo::M_##FN_NAME;                                                      \
    r.add_tag(l_tag_info);                                                                       \
    return r;                                                                                    \
}                                                                                                \
/**/                                                                                             \

#define BINARY_OP_IMPL_FN(fn_name)     _BASE_BINARY_OP_IMPL_FN(fn_name, type())
#define BINARY_CMP_OP_IMPL_FN(fn_name)  _BASE_BINARY_OP_IMPL_FN(fn_name, TypeInfo::mk_bool())

FOR_EACH_ARITHEMATIC_OP(BINARY_OP_IMPL_FN)
FOR_EACH_LOGICAL_OP(BINARY_CMP_OP_IMPL_FN)

#undef BINARY_OP_IMPL_FN
#undef BINARY_CMP_OP_IMPL_FN
#undef _BASE_BINARY_OP_IMPL_FN

ValueInfo ValueInfo::cond(ValueInfo then_value, ValueInfo else_value) const {
    CODEGEN_FN
    if (has_error() or then_value.has_error() or else_value.has_error()) {
        M_mark_error();
        return ValueInfo::null();
    }
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
    ValueInfo v{value_type_t::conditional, then_value.type(), std::vector<ValueInfo>{{*this, then_value, else_value}}};
    v.add_tag(l_tag_info);
    return v;
}

void ValueInfo::push(std::string_view name) const {
    if (has_error()) {
        return;
    }
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store/push an empty string as name");
    } else {
        CodeSectionContext::push(cname, *this);
    }
}

ValueInfo ValueInfo::load() const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not this->type().is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define load underlying operation for non pointer type");
        return ValueInfo::null();
    }
    return ValueInfo{value_type_t::load, type().base_type(), std::vector<ValueInfo>{{*this}}};
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
    const TypeInfo& base_type = type().base_type();
    if (not base_type.is_array()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-array pointer type");
        return ValueInfo::null();
    }
    if (i >= base_type.num_elements()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "Array is of size: " << base_type.num_elements() << ", can't access element:" << i);
        return ValueInfo::null();
    }
    ValueInfo v{value_type_t::inner_entry, base_type.base_type().pointer_type(), std::vector<ValueInfo>{{*this, ValueInfo::from_constant(i)}}};
    v.m_parent_ptr_type = base_type;
    return v;
}

ValueInfo ValueInfo::entry(const ValueInfo& i) const {
    CODEGEN_FN
    if (has_error() or i.has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-pointer type");
        return ValueInfo::null();
    }
    const TypeInfo& base_type = type().base_type();
    if (not base_type.is_array()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define array-entry operation for non-array pointer type");
        return ValueInfo::null();
    }
    // TODO{vibhanshu}: check if i is of type integer
    ValueInfo v{value_type_t::inner_entry, base_type.base_type().pointer_type(), std::vector<ValueInfo>{{*this, i}}};
    v.m_parent_ptr_type = base_type;
    return v;
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
    const TypeInfo& base_type = type().base_type();
    if (not base_type.is_struct()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define struct-field access operation for non-struct pointer type");
        return ValueInfo::null();
    }
    TypeInfo::field_entry_t field_entry = base_type[s];
    ValueInfo tgt_idx = ValueInfo::from_constant((int32_t)field_entry.idx());
    ValueInfo v{value_type_t::inner_entry, field_entry.type().pointer_type(), std::vector<ValueInfo>{{*this, tgt_idx}}};
    v.m_parent_ptr_type = base_type;
    return v;
}


ValueInfo ValueInfo::load_vector_entry(const ValueInfo& idx_v) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_vector()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define vector entry access operation for non-vector type");
        return ValueInfo::null();
    }
    ValueInfo v{value_type_t::load_vector_entry, type().base_type(), std::vector<ValueInfo>{{*this, idx_v}}};
    v.add_tag(tag_info());
    return v;
}

ValueInfo ValueInfo::store_vector_entry(const ValueInfo& idx_v, ValueInfo value) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (idx_v.has_error()) {
        return ValueInfo::null();
    }
    return ValueInfo{value_type_t::store_vector_entry, type(), std::vector<ValueInfo>{{*this, idx_v, value}}};
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
    // TODO{vibhanshu}: review if store should be called inpace, or we are allowed to delay execution
    ValueInfo{value_type_t::store, type(), std::vector<ValueInfo>{{*this, value}}}.M_eval();
}

auto ValueInfo::null() -> ValueInfo {
    static ValueInfo s_null_value{};
    LLVM_BUILDER_ASSERT(s_null_value.has_error());
    return s_null_value;
}

auto ValueInfo::from_context(const TypeInfo& ctx_type) -> ValueInfo {
    return ValueInfo{value_type_t::context, ctx_type, std::vector<ValueInfo>{}};
}

[[nodiscard]]
ValueInfo ValueInfo::load_vector_entry(uint32_t i) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_vector()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define vector entry access operation for non-vector type");
        return ValueInfo::null();
    }
    if (i > type().num_elements()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't access entry:" << i << ", total elements:" << type().num_elements());
        return ValueInfo::null();
    }
    ValueInfo v{value_type_t::load_vector_entry, type().base_type(), std::vector<ValueInfo>{{*this, ValueInfo::from_constant<int32_t>(i)}}};
    v.add_tag(tag_info());
    return v;
}

ValueInfo ValueInfo::store_vector_entry(uint32_t i, ValueInfo value) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (not type().is_vector()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't define vector entry access operation for non-vector type");
        return ValueInfo::null();
    }
    if (i > type().num_elements()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't access entry:" << i << ", total elements:" << type().num_elements());
        return ValueInfo::null();
    }
    ValueInfo v{value_type_t::store_vector_entry, type(), std::vector<ValueInfo>{{*this, ValueInfo::from_constant<int32_t>(i), value}}};
    v.add_tag(tag_info());
    return v;
}

ValueInfo ValueInfo::mk_pointer(TypeInfo type) {
    CODEGEN_FN
    if (type.has_error() or type.has_error()) {
        return ValueInfo::null();
    }
    ValueInfo r{value_type_t::mk_ptr, type.pointer_type(), std::vector<ValueInfo>{}};
    if (CursorContextImpl::has_value()) {
        llvm::AllocaInst* l_inst = CursorContextImpl::builder().CreateAlloca(type.native_value(), nullptr, "");
        LLVM_BUILDER_ASSERT(l_inst != nullptr);
        [[maybe_unused]] llvm::PointerType* ptr_type = l_inst->getType();
        LLVM_BUILDER_ASSERT(ptr_type != nullptr);
        LLVM_BUILDER_ASSERT(ptr_type->isLoadableOrStorableType(type.native_value()));
        r.m_const_value_cache = l_inst;
    }
    return r;
}

ValueInfo ValueInfo::mk_array(TypeInfo type) {
    CODEGEN_FN
    if (type.has_error()) {
        return ValueInfo::null();
    }
    LLVM_BUILDER_ABORT("unimplemented");
    return ValueInfo::null();
}

ValueInfo ValueInfo::mk_struct(TypeInfo type) {
    CODEGEN_FN
    if (type.has_error()) {
        return ValueInfo::null();
    }
    LLVM_BUILDER_ABORT("unimplemented");
    return ValueInfo::null();
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
            ValueInfo l_match_idx = l_curr_iter_idx.equal(idx);
            ValueInfo l_offset_curr = ValueInfo::from_constant(static_cast<int32_t>(type[i].offset()));
            l_offset_result = l_match_idx.cond(l_offset_curr, l_offset_result);
        }
        return l_offset_result;
    } else {
        return ValueInfo::null();
    }
}

llvm::Value* ValueInfo::M_eval() {
    CODEGEN_FN
    switch (m_value_type) {
        case value_type_t::null: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 0);
            LLVM_BUILDER_ASSERT(m_const_value_cache == nullptr);
            return nullptr;
            break;
        }
        case value_type_t::constant: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 0);
            LLVM_BUILDER_ASSERT(m_const_value_cache != nullptr);
            return m_const_value_cache;
            break;
        }
        case value_type_t::context: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 0);
            if (CursorContextImpl::has_value()) {
                Function fn = CodeSectionContext::current_function();
                return meta::remove_const(fn.context()).M_eval();
            } else {
                return nullptr;
            }
            break;
        }
        case value_type_t::binary: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 2);
            LLVM_BUILDER_ASSERT(m_binary_op != nullptr);
            llvm::Value* l_arg1 = m_parent[0].M_eval();
            llvm::Value* l_arg2 = m_parent[1].M_eval();
            if (l_arg1 == nullptr or l_arg2 == nullptr) {
                return nullptr;
            } else {
                LLVM_BUILDER_ASSERT(l_arg1 != nullptr);
                LLVM_BUILDER_ASSERT(l_arg2 != nullptr);
                return (m_type_info.*m_binary_op)(l_arg1, l_arg2);
            }
            break;
        }
        case value_type_t::conditional: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 3);
            llvm::Value* l_cond = m_parent[0].M_eval();
            llvm::Value* l_arg1 = m_parent[1].M_eval();
            llvm::Value* l_arg2 = m_parent[2].M_eval();
            if (l_cond == nullptr or l_arg1 == nullptr or l_arg2 == nullptr) {
                return nullptr;
            } else {
                LLVM_BUILDER_ASSERT(l_cond != nullptr);
                LLVM_BUILDER_ASSERT(l_arg1 != nullptr);
                LLVM_BUILDER_ASSERT(l_arg2 != nullptr);
                if (CursorContextImpl::has_value()) {
                    llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
                    llvm::Value* l_entry = l_cursor.CreateSelect(l_cond, l_arg1, l_arg2, "");
                    return l_entry;
                } else {
                    return nullptr;
                }
            }
            break;
        }
        case value_type_t::typecast: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 1);
            llvm::Value* l_src = m_parent[0].M_eval();
            if (l_src == nullptr) {
                return nullptr;
            } else {
                return m_type_info.M_type_cast(m_parent[0].type(), l_src);
            }
            break;
        }
        case value_type_t::inner_entry: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 2);
            LLVM_BUILDER_ASSERT(not m_parent_ptr_type.has_error());
            std::array<llvm::Value*, 2u> index_list;
            index_list[0] = ValueInfo::from_constant(0).M_eval();
            index_list[1] = m_parent[1].M_eval();
            llvm::Value* l_src = m_parent[0].M_eval();
            if (CursorContextImpl::has_value() and l_src != nullptr) {
                llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
                llvm::Value* l_entry_idx = l_cursor.CreateGEP(m_parent_ptr_type.native_value(), l_src, index_list, "_index");
                return l_entry_idx;
            } else {
                return nullptr;
            }
            break;
        }
        case value_type_t::load: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 1);
            llvm::Value* l_src = m_parent[0].M_eval();
            if (CursorContextImpl::has_value() and l_src != nullptr) {
                return CursorContextImpl::builder().CreateLoad(type().native_value(), l_src, "");
            } else {
                return nullptr;
            }
            break;
        }
        case value_type_t::store: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 2);
            llvm::Value* l_dst = m_parent[0].M_eval();
            llvm::Value* l_src = m_parent[1].M_eval();
            if (CursorContextImpl::has_value()) {
                CursorContextImpl::builder().CreateStore(l_src, l_dst);
                return l_src;
            } else {
                CODEGEN_PUSH_ERROR(VALUE_ERROR, "can't store value outside cursor context");
                return nullptr;
            }
            break;
        }
        case value_type_t::load_vector_entry: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 2);
            llvm::Value* l_vec = m_parent[0].M_eval();
            llvm::Value* l_idx = m_parent[1].M_eval();
            if (CursorContextImpl::has_value()) {
                llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
                return l_cursor.CreateExtractElement(l_vec, l_idx, "");
            } else {
                return nullptr;
            }
            break;
        }
        case value_type_t::store_vector_entry: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 3);
            llvm::Value* l_vec = m_parent[0].M_eval();
            llvm::Value* l_idx = m_parent[1].M_eval();
            llvm::Value* l_v = m_parent[2].M_eval();
            if (CursorContextImpl::has_value()) {
                llvm::IRBuilder<>& l_cursor = CursorContextImpl::builder();
                return l_cursor.CreateInsertElement(l_vec, l_v, l_idx, "");
            } else {
                return nullptr;
            }
            break;
        }
        case value_type_t::mk_ptr: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 0);
            LLVM_BUILDER_ASSERT(m_const_value_cache != nullptr);
            return m_const_value_cache;
            break;
        }
        case value_type_t::fn_call: {
            LLVM_BUILDER_ASSERT(m_parent.size() == 0);
            LLVM_BUILDER_ASSERT(m_const_value_cache != nullptr);
            return m_const_value_cache;
            break;
        }
    }
    LLVM_BUILDER_ABORT("value_type not handled");
    return nullptr;
}

LLVM_BUILDER_NS_END
