//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/type.h"
#include "llvm_builder/value.h"
#include "llvm_builder/module.h"
#include "llvm_builder/analyze.h"
#include "llvm/context_impl.h"

#include "util/debug.h"
#include "util/string_util.h"
#include "ext_include.h"
#include <ostream>
#include <string>
#include <sstream>
#include <unordered_set>

LLVM_BUILDER_NS_BEGIN

std::string LinkSymbolName::s_ns_separator{"_"};

// TODO{vibhanshu}: add support for tensor type which is a multi-dimensional generalization of vector
enum class type_id_t : uint8_t {
    invalid_t,
    void_t,
    bool_t,
    int_t,
    float_t,
    pointer_t,
    array_t,
    vector_t,
    struct_t
};

//
// TypeInfo::derived_info_t
//
struct TypeInfo::derived_info_t : meta::noncopyable {
    struct full_field_info_t : public member_field_entry {
        using BaseT = member_field_entry;
    private:
        uint32_t m_offset = std::numeric_limits<uint32_t>::max();
    public:
        explicit full_field_info_t() = default;
        explicit full_field_info_t(uint32_t offset, const std::string& name, const TypeInfo& type)
            : BaseT{name, type}, m_offset{offset} {
            LLVM_BUILDER_ASSERT(offset != std::numeric_limits<uint32_t>::max())
            LLVM_BUILDER_ASSERT(not name.empty())
            LLVM_BUILDER_ASSERT(not type.has_error())
            LLVM_BUILDER_ASSERT(not type.has_error());
        }
        explicit full_field_info_t(uint32_t offset, const member_field_entry& entry)
            : BaseT{entry}, m_offset{offset} {
            LLVM_BUILDER_ASSERT(offset != std::numeric_limits<uint32_t>::max())
        }
        ~full_field_info_t() = default;
    public:
        uint32_t offset() const {
            return m_offset;
        }
        bool operator == (const std::string& s) const {
            return BaseT::name() == s;
        }
        std::string short_name() const {
            return LLVM_BUILDER_CONCAT << "{" << BaseT::name() << ":" << BaseT::type().short_name() << "}";
        }
        bool operator == (const full_field_info_t& rhs) const {
            return m_offset == rhs.m_offset
                and name() == rhs.name()
                and type() == rhs.type();
        }
    };
    using type_list_t = std::vector<full_field_info_t>;
private:
    const type_id_t   m_type = type_id_t::invalid_t;
    // TODO{vibhanshu}: remove usage of std::vector<> instead use something which
    //                  does not generate garbage
    const std::string m_name;
    type_list_t m_type_list;
    std::unordered_set<std::string> m_type_name_set;
    const uint32_t m_num_elements =  0;
    uint32_t m_struct_size_bytes = 0;
public:
    explicit derived_info_t(type_id_t type, TypeInfo base_type)
        : m_type{type} {
        LLVM_BUILDER_ASSERT(type == type_id_t::pointer_t);
        LLVM_BUILDER_ASSERT(not base_type.has_error());
        m_type_list.emplace_back(0, "underlying", base_type);
    }
    explicit derived_info_t(type_id_t type, TypeInfo element_type, uint32_t num_elements)
        : m_type{type}, m_num_elements{num_elements} {
        LLVM_BUILDER_ASSERT(is_array() or is_vector());
        LLVM_BUILDER_ASSERT(m_num_elements > 0);
        LLVM_BUILDER_ASSERT(not element_type.has_error());
        m_type_list.emplace_back(0, "entry", element_type);
    }
    explicit derived_info_t(type_id_t type
                            , const std::string& name
                            , const std::vector<member_field_entry>& field_types
                            , const llvm::StructLayout& struct_layout)
        : m_type{type}, m_name{name}, m_num_elements{static_cast<uint32_t>(field_types.size())} {
        LLVM_BUILDER_ASSERT(type == type_id_t::struct_t);
        LLVM_BUILDER_ASSERT(not m_name.empty());
        LLVM_BUILDER_ASSERT(field_types.size() > 0);
        // TODO{vibhanshu}: assuming all the modules will have same data layout
        std::vector<uint32_t> field_offsets;
        for (const llvm::TypeSize& l_type_size : struct_layout.getMemberOffsets()) {
            field_offsets.emplace_back(l_type_size);
        }
        LLVM_BUILDER_ASSERT(field_types.size() == field_offsets.size());
        const uint32_t l_num_fields = static_cast<uint32_t>(field_types.size());
        for (uint32_t i = 0; i != l_num_fields; ++i) {
            m_type_list.emplace_back(field_offsets[i], field_types[i]);
            [[maybe_unused]] auto it = m_type_name_set.emplace(field_types[i].name());
            LLVM_BUILDER_ASSERT(it.second);
        }
        LLVM_BUILDER_ASSERT(m_num_elements == m_type_list.size());
        m_struct_size_bytes = (uint32_t)struct_layout.getSizeInBytes();
    }
public:
    bool is_valid() const {
        if (is_pointer()) {
            return m_type_list[0].is_valid();
        } else if (is_array() or is_vector()) {
            return m_num_elements > 0 and m_type_list[0].is_valid();
        } else {
            LLVM_BUILDER_ASSERT(is_struct());
            bool l_all_valid = true;
            for (const full_field_info_t& l_info : m_type_list) {
                l_all_valid &= l_info.is_valid();
            }
            return l_all_valid;
        }
    }
    bool is_pointer() const {
        return m_type == type_id_t::pointer_t;
    }
    bool is_array() const {
        return m_type == type_id_t::array_t;
    }
    bool is_vector() const {
        return m_type == type_id_t::vector_t;
    }
    bool is_struct() const {
        return m_type == type_id_t::struct_t;
    }
    const std::string& name() const {
        return m_name;
    }
    const TypeInfo& base_type() const {
        if (is_pointer() or is_array() or is_vector()) {
            return m_type_list[0].type();
        } else {
            CODEGEN_PUSH_ERROR(VALUE_ERROR, LLVM_BUILDER_CONCAT << "can't define base-type of type:" << short_name())
            return TypeInfo::null();
        }
    }
    field_entry_t  operator [] (uint32_t i) const {
        CODEGEN_FN
        if (is_struct() and i < m_type_list.size()) {
            const full_field_info_t& l_field_info = m_type_list[i];
            return field_entry_t{i, l_field_info.offset(), l_field_info.name(), l_field_info.type(), l_field_info.is_readonly()};
        } else {
            CODEGEN_PUSH_ERROR(VALUE_ERROR, LLVM_BUILDER_CONCAT << "unable to find field:" << i << " in struct:" << name())
            return field_entry_t::null();
        }
    }
    field_entry_t operator [] (const std::string& s) const {
        CODEGEN_FN
        if (is_struct() and s.size() > 0) {
            uint32_t i = 0;
            for (const full_field_info_t& e : m_type_list) {
                if (e == s) {
                    return field_entry_t{i, e.offset(), e.name(), e.type(), e.is_readonly()};
                }
                ++i;
            }
        }
        CODEGEN_PUSH_ERROR(VALUE_ERROR, LLVM_BUILDER_CONCAT << "unable to find field:" << s << " in struct:" << name())
        return field_entry_t::null();
    }
    uint32_t num_elements() const {
        if (is_array() or is_vector() or is_struct()) {
            return m_num_elements;
        } else {
            return std::numeric_limits<uint32_t>::max();
        }
    }
    uint32_t struct_size_bytes() const {
        if (is_struct()) {
            return m_struct_size_bytes;
        } else {
            return std::numeric_limits<uint32_t>::max();
        }
    }
    std::string short_name() const {
        if (is_pointer()) {
            return LLVM_BUILDER_CONCAT << "ptr[" << m_type_list[0].type().short_name() << "]";
        } else if (is_array()) {
            return LLVM_BUILDER_CONCAT << "array[" << m_type_list[0].type().short_name()
                << ":" << m_num_elements << "]";
        } else if (is_vector()) {
            return LLVM_BUILDER_CONCAT << "vector[" << m_type_list[0].type().short_name()
                << ":" << m_num_elements << "]";
        } else {
            LLVM_BUILDER_ASSERT(is_struct());
            std::stringstream os;
            os << "struct[" << name() << "]";
            return os.str();
        }
    }
    bool operator==(const derived_info_t &o) const {
        if (m_type != o.m_type
            or m_name != o.m_name
            or m_num_elements != o.m_num_elements
            or m_type_list.size() != o.m_type_list.size()) {
            return false;
        }
        for (uint32_t i = 0; i != m_type_list.size(); ++i) {
            if (m_type_list[i] != o.m_type_list[i]) {
                return false;
            }
        }
        return true;
    }
};

//
// TypeInfo::field_entry_t
//
TypeInfo::field_entry_t::field_entry_t(const uint32_t idx,
                                       const uint32_t offset,
                                       const std::string& name,
                                       const TypeInfo& type,
                                       bool is_readonly)
    : BaseT{State::VALID}
    , m_idx{idx}
    , m_offset{offset}
    , m_name{name}
    , m_type{type}
    , m_is_readonly{is_readonly} {
    if (idx == std::numeric_limits<uint32_t>::max()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "trying to build invalid field entry")
        M_mark_error();
    } else if (offset == std::numeric_limits<uint32_t>::max()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "trying to build invalid field entry")
        M_mark_error();
    } else if (name.empty()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "trying to build field entry without name")
        M_mark_error();
    } else if (type.has_error()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "trying to build field entry wit invalid type")
        M_mark_error();
    }
}

// TypeInfo::field_entry_t::field_entry_t(const uint32_t idx, const uint32_t offset, const std::pair<std::string, TypeInfo>& entry)
//     : m_idx{idx}, m_offset{offset}, m_name{entry.first}, m_type{entry.second} {
// }

TypeInfo::field_entry_t::~field_entry_t() = default;

bool TypeInfo::field_entry_t::operator==(const field_entry_t &o) const {
    if (has_error() and o.has_error()) {
        return true;
    }
    if (has_error() or o.has_error()) {
        return false;
    }
    const bool l_idx_match = (m_idx == o.m_idx);
    const bool l_offset_match = (m_offset == o.m_offset);
    const bool l_name_match = (m_name == o.m_name);
    const bool l_type_match = (m_type == o.m_type);
    return l_idx_match and l_offset_match and l_name_match and l_type_match;
}

auto TypeInfo::field_entry_t::null() -> field_entry_t& {
    const uint32_t l_uint_max = std::numeric_limits<uint32_t>::max();
    static const std::string s_name{};
    static const TypeInfo s_null_type{};
    static field_entry_t s_null_entry{l_uint_max, l_uint_max, s_name, s_null_type, true};
    return s_null_entry;
}

#define DECL_EQUIV_FN(fn)                                               \
    bool is_##fn##_equiv () const {                                     \
        return is_##fn () or (is_vector() and base_type().is_##fn ());  \
    }                                                                   \
    /**/                                                                \
                                                                        
#define BINARY_OP_IMPL_FN(FN_NAME, SIGN_FN, UNSIGNED_FN, FLOAT_FN)      \
    llvm::Value* FN_NAME (llvm::Value* lhs, llvm::Value* rhs, const std::string& op_name) { \
        CODEGEN_FN                                                      \
        LLVM_BUILDER_ASSERT(lhs != nullptr)                                 \
        LLVM_BUILDER_ASSERT(rhs != nullptr)                                 \
        if (not is_scalar() and not is_vector())  {                 \
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type needs to be either scalar or vector"); \
            return nullptr;                                         \
        }                                                           \
        llvm::IRBuilder<>& l_builder = m_cursor_impl.builder();         \
        if (is_vector()) {                                              \
            const TypeInfo& l_base_type = base_type();                  \
            if (l_base_type.is_signed_integer()) {                      \
                return l_builder. SIGN_FN (lhs, rhs, op_name);          \
            } else {                                                    \
                LLVM_BUILDER_ASSERT(l_base_type.is_unsigned_integer());         \
                return l_builder. UNSIGNED_FN (lhs, rhs, op_name);      \
            }                                                           \
        } else if (is_integer()) {                                      \
            if (is_signed_integer()) {                                  \
                return l_builder. SIGN_FN (lhs, rhs, op_name);          \
            } else {                                                    \
                LLVM_BUILDER_ASSERT(is_unsigned_integer());                     \
                return l_builder. UNSIGNED_FN (lhs, rhs, op_name);      \
            }                                                           \
        } else if (is_boolean()) {                                      \
            return l_builder. UNSIGNED_FN(lhs, rhs, op_name);           \
        } else if(is_float_equiv()) {                                   \
            return l_builder. FLOAT_FN (lhs, rhs, op_name);             \
        } else {                                                        \
            CODEGEN_PUSH_ERROR(TYPE_ERROR, " operation not supported yet for type:" << short_name()); \
            return nullptr;                                             \
        }                                                               \
    }                                                                   \
    /**/                                                                \

//
// TypeInfo::Impl
//
class TypeInfo::Impl : meta::noncopyable {
private:
    llvm::Type* m_type = nullptr;
    CursorPtr m_cursor_impl;
    uint32_t m_num_bytes = 0;
    type_id_t m_type_id = type_id_t::invalid_t;
    bool m_is_signed = true;
    std::unique_ptr<derived_info_t> m_derived_info;
public:
    explicit Impl(void_construct_t, CursorPtr cursor_impl, llvm::Type *type) : m_type{type}, m_cursor_impl{cursor_impl}, m_type_id{type_id_t::void_t} {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        LLVM_BUILDER_ASSERT(m_type->isVoidTy());
        LLVM_BUILDER_ASSERT(m_type->getPrimitiveSizeInBits() == m_num_bytes);
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(int_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes) : m_type{type}, m_cursor_impl{cursor_impl}, m_num_bytes{num_bytes} {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        LLVM_BUILDER_ASSERT(m_type->isIntegerTy());
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        if (m_type->getPrimitiveSizeInBits() == 1) {
            LLVM_BUILDER_ASSERT(m_num_bytes == 1);
            m_type_id = type_id_t::bool_t;
        } else {
            m_type_id = type_id_t::int_t;
        }
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(float_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes)
        : m_type{type}, m_cursor_impl{cursor_impl}, m_num_bytes{num_bytes}, m_type_id{type_id_t::float_t} {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        LLVM_BUILDER_ASSERT(m_type->isFloatTy() or m_type->isDoubleTy());
        LLVM_BUILDER_ASSERT((m_type->getPrimitiveSizeInBits() / 8) == m_num_bytes);
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(pointer_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes, const TypeInfo& base_type)
        : m_type{type}, m_cursor_impl{cursor_impl}, m_num_bytes{num_bytes}, m_type_id{type_id_t::pointer_t} {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        LLVM_BUILDER_ASSERT(m_type->isPointerTy());
        LLVM_BUILDER_ASSERT(m_num_bytes == sizeof(int*));
        LLVM_BUILDER_ASSERT(not base_type.has_error());
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        m_derived_info = std::make_unique<derived_info_t>(m_type_id, base_type);
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(int_construct_t, CursorPtr cursor_impl, llvm::IntegerType* type, bool is_signed)
        :  m_type{(llvm::Type*)type}, m_cursor_impl{cursor_impl}, m_num_bytes{type->getBitWidth() / 8u}, m_is_signed{is_signed} {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        LLVM_BUILDER_ASSERT(m_type->isIntegerTy());
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        m_type_id = type_id_t::int_t;
        LLVM_BUILDER_ASSERT(m_num_bytes == 1 or m_num_bytes == 2 or m_num_bytes == 4 or m_num_bytes == 8);
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(array_construct_t, CursorPtr cursor_impl, TypeInfo element_type, uint32_t array_size)
      :  m_type{(llvm::Type*)llvm::ArrayType::get(element_type.native_value(), array_size)}
        , m_cursor_impl{cursor_impl}
        , m_num_bytes{element_type.size_in_bytes() * array_size}
        , m_type_id{type_id_t::array_t} {
        LLVM_BUILDER_ASSERT(not element_type.has_error());
        LLVM_BUILDER_ASSERT(array_size > 0);
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        m_derived_info = std::make_unique<derived_info_t>(m_type_id, element_type, array_size);
        LLVM_BUILDER_ASSERT(m_derived_info->is_valid());
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(vector_construct_t, CursorPtr cursor_impl, TypeInfo element_type, uint32_t vector_size)
        : m_type{(llvm::Type*)llvm::FixedVectorType::get(element_type.native_value(), vector_size)}
        , m_cursor_impl{cursor_impl}
        , m_num_bytes{element_type.size_in_bytes() * vector_size}
        , m_type_id{type_id_t::vector_t} {
        LLVM_BUILDER_ASSERT(not element_type.has_error());
        LLVM_BUILDER_ASSERT(element_type.is_scalar());
        LLVM_BUILDER_ASSERT(vector_size > 0);
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        m_derived_info = std::make_unique<derived_info_t>(m_type_id, element_type, vector_size);
        LLVM_BUILDER_ASSERT(m_derived_info->is_valid());
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    explicit Impl(struct_construct_t, CursorPtr cursor_impl, const std::string &name,
                  const std::vector<member_field_entry> &field_types,
                  const bool packed)
        : m_cursor_impl{cursor_impl}
        , m_type_id{type_id_t::struct_t} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        LLVM_BUILDER_ASSERT(field_types.size() > 0);
        LLVM_BUILDER_ASSERT(not m_cursor_impl.is_bind_called());
        std::vector<llvm::Type*> l_raw_type_list;
        std::unordered_set<std::string> l_field_types_set;
        for (const member_field_entry& l_info : field_types) {
            LLVM_BUILDER_ASSERT(l_info.is_valid());
            const TypeInfo& l_type = l_info.type();
            LLVM_BUILDER_ASSERT(l_type.is_valid())
            LLVM_BUILDER_ASSERT(l_type.is_scalar() or l_type.is_pointer());
            l_raw_type_list.emplace_back(l_type.native_value());
            auto it = l_field_types_set.emplace(l_info.name());
            if (not it.second) {
                CODEGEN_PUSH_ERROR(TYPE_ERROR, "duplicate field name in struct:" << name << "." << l_info.name());
                return;
            }
        }
        m_type = (llvm::Type*)llvm::StructType::create(m_cursor_impl.ctx(), l_raw_type_list, name, packed);
        llvm::DataLayout l_data_layout = CursorContextImpl::get_host_data_layout();
        const llvm::StructLayout* l_struct_layout = l_data_layout.getStructLayout((llvm::StructType*)m_type);
        m_derived_info = std::make_unique<derived_info_t>(m_type_id, name, field_types, *l_struct_layout);
        LLVM_BUILDER_ASSERT(m_derived_info->is_valid());
        m_num_bytes = static_cast<uint32_t>(l_struct_layout->getSizeInBytes());
        object::Counter::singleton().on_new(object::Callback::object_t::TYPE, (uint64_t)this, short_name());
    }
    ~Impl() {
        if (m_type != nullptr) {
            // TODO{vibhanshu}: sequence of deleteion needs to be set reverse of insertion for this to work
            object::Counter::singleton().on_delete(object::Callback::object_t::TYPE, (uint64_t)this, "");
        }
        m_type = nullptr;
        m_num_bytes = 0;
    }
public:
    uint32_t size_in_bytes() const {
        return m_num_bytes;
    }
    bool is_valid() const {
        return m_type != nullptr and m_type_id != type_id_t::invalid_t;
    }
    bool is_void() const {
        return m_type_id == type_id_t::void_t;
    }
    bool is_boolean() const {
        return m_type_id == type_id_t::bool_t;
    }
    bool is_integer() const {
        return m_type_id == type_id_t::int_t;
    }
    bool is_pointer() const {
        return m_type_id == type_id_t::pointer_t;
    }
    bool is_unsigned_integer() const {
        if (is_integer()) {
            return not m_is_signed;
        } else {
            return false;
        }
    }
    bool is_signed_integer() const {
        if (is_integer()) {
            return m_is_signed;
        } else {
            return false;
        }
    }
    bool is_float() const {
        return m_type_id == type_id_t::float_t;
    }
    bool is_scalar() const {
        return is_boolean() or is_integer() or is_float();
    }
    bool is_array() const {
        return (m_type_id == type_id_t::array_t);
    }
    bool is_vector() const {
        return (m_type_id == type_id_t::vector_t);
    }
    bool is_struct() const {
        return (m_type_id == type_id_t::struct_t);
    }
    bool operator==(const Impl& o) const {
        // TODO{vibhashu}: think of a way so that adding a new property will automatically
        //                 lead to it's comparison being done
        if (m_num_bytes != o.m_num_bytes) {
            return false;
        }
        if (m_type_id != o.m_type_id) {
            return false;
        }
        if (m_is_signed != o.m_is_signed) {
            return false;
        }
        if (static_cast<bool>(m_derived_info) xor static_cast<bool>(o.m_derived_info)) {
            return false;
        }
        if (m_derived_info and o.m_derived_info) {
            if (not (*m_derived_info == *o.m_derived_info)) {
                return false;
            }
        }
        return true;
    }
    llvm::Type* native_value() const {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        return m_type;
    }
    const TypeInfo& base_type() const {
        CODEGEN_FN
        if (m_derived_info) {
            return m_derived_info->base_type();
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type is scalar");
            return TypeInfo::null();
        }
    }
    field_entry_t operator[](uint32_t i) const {
        CODEGEN_FN
        if (m_derived_info) {
            return (*m_derived_info)[i];
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type is scalar");
            return field_entry_t::null();
        }
    }
    field_entry_t operator[](const std::string &s) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not s.empty());
        if (m_derived_info) {
            return (*m_derived_info)[s];
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type is scalar");
            return field_entry_t::null();
        }
    }
    uint32_t num_elements() const {
        CODEGEN_FN
        if (m_derived_info) {
            return m_derived_info->num_elements();
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type is scalar");
            return std::numeric_limits<uint32_t>::max();
        }
    }
    std::string struct_name() const {
        CODEGEN_FN
        if (is_struct()) {
            return m_derived_info->name();
        } else {
            return std::string{"INVALID_STRUCT"};
        }
    }
    uint32_t struct_size_bytes() const {
        CODEGEN_FN
        if (is_struct()) {
            return m_derived_info->struct_size_bytes();
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type is not struct");
            return std::numeric_limits<uint32_t>::max();
        }
    }
    [[gnu::noinline]]
    std::string short_name() const {
        CODEGEN_FN
        if (is_struct() or is_vector() or is_array() or is_pointer()) {
            return m_derived_info->short_name();
        } else if (is_boolean()) {
            return "bool";
        } else if (is_void()) {
            return "void";
        } else if (is_integer()) {
            if (is_signed_integer()) {
                return LLVM_BUILDER_CONCAT << "int" << (m_num_bytes * 8);
            } else {
                LLVM_BUILDER_ASSERT(is_unsigned_integer());
                return LLVM_BUILDER_CONCAT << "uint" << (m_num_bytes * 8);
            }
        } else if (is_float()) {
            return LLVM_BUILDER_CONCAT << "float" << (m_num_bytes * 8);
        } else {
            return LLVM_BUILDER_CONCAT << "<UNKNOWN SHORT NAME>";
        }
    }
    [[gnu::noinline]]
    void print(std::ostream &os) const {
        os << "{Type:value:" << m_type
            << ", num_bytes:" << m_num_bytes
            << ", is_boolean:" << is_boolean()
            << ", is_pointer:" << is_pointer()
            << ", is_integer:" << is_integer()
            << ", is_signed:" << m_is_signed
            << ", is_float:" << is_float()
            << ", is_array:" << is_array()
            << ", is_vector:" << is_vector()
            << ", is_struct:" << is_struct()
            << "}";
    }
    [[gnu::noinline]]
    void dump_llvm_type_info(std::ostream &os) const {
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        os << " Debug llvm::Type : " << m_type << std::endl
        << "    > num_bites :" << m_type->getPrimitiveSizeInBits() << std::endl
        << "    > is_void  : " << m_type->isVoidTy() << std::endl
        << "    > is_float : " << m_type->isFloatTy() << std::endl
        << "    > is_double : " << m_type->isDoubleTy() << std::endl
        << "    > is_integer : " << m_type->isIntegerTy() << std::endl
        << "    > is_pointer : "  << m_type->isPointerTy() << std::endl;
        for (uint32_t i = 1; i <= 8; ++i) {
            os << "      > is_integer:" << i << " - " << m_type->isIntegerTy(i) << std::endl;
        }
        os << "    > is_single_value:" << m_type->isSingleValueType() << std::endl
        << "    > is_aggregate:" << m_type->isAggregateType() << std::endl
        << "    > size in bits:" << m_type->getPrimitiveSizeInBits() << std::endl
        << "    > scalar size in bits:" << m_type -> getScalarSizeInBits() << std::endl;
    }
    [[gnu::noinline]]
    bool check_sync(const ValueInfo &value) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not value.has_error());
        LLVM_BUILDER_ASSERT(m_type != nullptr);
        // TODO{vibhanshu}: this is a WIP fn, implement it correctly using per type statisticas
        // TODO{vibhanshu}: add ability to register callbacks if a type check fails in a global context
        //                  which can be initialized outsie the function at top level
        // TODO{vibhanshu}: this function should be defined recursively to check inner fields also
        bool l_res = is_valid();
        llvm::Type* l_type = value.native_value()->getType();
        LLVM_BUILDER_ASSERT(l_type != nullptr);
        if (is_boolean()) {
            l_res &= l_type->getPrimitiveSizeInBits() == 1;
            l_res &= (m_type == l_type);
        } else if (is_integer()) {
            l_res &= l_type->isIntegerTy();
            if (l_res) {
                l_res &= l_type->getPrimitiveSizeInBits() >= 8;
                l_res &= (m_type->getPrimitiveSizeInBits() == l_type->getPrimitiveSizeInBits());
            }
            if (l_res) {
                l_res &= (is_signed_integer() == value.type().is_signed_integer());
            }
            l_res &= (m_type == l_type);
        } else if (is_float()) {
            l_res &= ((l_type->isFloatTy() and m_num_bytes == sizeof(float32_t))
                    or (l_type->isDoubleTy() and m_num_bytes == sizeof(float64_t)));
            l_res &= (m_type == l_type);
        } else if (is_array()) {
            l_res &= l_type->isArrayTy() and l_type->isAggregateType();
            if (l_res) {
                LLVM_BUILDER_ASSERT(llvm::ArrayType::classof(m_type));
                LLVM_BUILDER_ASSERT(llvm::ArrayType::classof(l_type));
                const uint64_t l_target_size = ((llvm::ArrayType*)m_type)->getNumElements();
                const uint64_t l_src_size = ((llvm::ArrayType*)l_type)->getNumElements();
                l_res &= (l_target_size == l_src_size);
            }
            l_res &= (m_type == l_type);
        } else if (is_vector()) {
            l_res &= l_type->isVectorTy();
            if (l_res) {
                LLVM_BUILDER_ASSERT(llvm::FixedVectorType::classof(m_type));
                LLVM_BUILDER_ASSERT(llvm::FixedVectorType::classof(l_type));
                const uint32_t l_target_size = ((llvm::FixedVectorType*)m_type)->getNumElements();
                const uint32_t l_src_size = ((llvm::FixedVectorType*)l_type)->getNumElements();
                l_res &= (l_target_size == l_src_size);
            }
            l_res &= (m_type == l_type);
        } else if (is_struct()) {
            l_res &= l_type->isStructTy() and l_type->isAggregateType();
            // TODO{vibhanshu}: test for contents of the struct
            if (l_res) {
                LLVM_BUILDER_ASSERT(llvm::StructType::classof(m_type));
                LLVM_BUILDER_ASSERT(llvm::StructType::classof(l_type));
                llvm::StructType* i_struct_type = (llvm::StructType*)m_type;
                llvm::StructType* o_struct_type = (llvm::StructType*)l_type;
                l_res &= (i_struct_type->isPacked() == o_struct_type->isPacked());
                l_res &= (i_struct_type->getNumElements() == o_struct_type->getNumElements());
                if (l_res) {
                    const uint32_t l_num_elements = i_struct_type->getNumElements();
                    // TODO{vibhanshu}: currently this does only a shallow type comparison
                    //                  do deep type comparison also
                    for (uint32_t i = 0; i != l_num_elements; ++i) {
                        llvm::Type* i_elem = i_struct_type->getElementType(i);
                        llvm::Type* o_elem = o_struct_type->getElementType(i);
                        l_res &= (i_elem->getTypeID() == o_elem->getTypeID());
                    }
                }
            }
            l_res &= (m_type == l_type);
        } else if (is_pointer()) {
            l_res &= l_type->isPointerTy();
            l_res &= (m_type == l_type);
        } else if (is_void()) {
            // TODO{vibhanshu}: handle case of isVoidTy
            // l_res &= l_type->isVoidTy();
        } else {
            l_res = false;
        }
        // TODO{vibhanshu}: add checks for bool , array, vector and struct types
        return l_res;
    }
public:
    TypeInfo pointer_type(const TypeInfo& parent) const {
        CODEGEN_FN
        if (m_type == nullptr) {
            return TypeInfo::null();
        } else if (parent.has_error()) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "can't make pointer of invalid type");
            return TypeInfo::null();
        } else if (m_cursor_impl.is_valid()){
            // TODOP{vibhanshu}: try to make mk_type_pointer fn call const
            return meta::remove_const(m_cursor_impl).mk_type_pointer(parent);
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Cursor not found");
            return TypeInfo::null();
        }
    }
public:
    // NOTE{vibhanshu}: unary operators
    llvm::Value *M_fixed_point_type_cast(const ValueInfo &src_value,
                                        const std::string &op_name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not src_value.has_error());
        LLVM_BUILDER_ASSERT(m_type != nullptr and not src_value.has_error());
        llvm::Value* src = src_value.native_value();
        llvm::IRBuilder<>& l_builder = CursorContextImpl::builder();
        if (not is_integer()) {
            return nullptr;
        }
        if (not src_value.type().is_integer() and not src_value.type().is_boolean()) {
            return nullptr;
        }
        if (not src->getType()->isIntegerTy()) {
            return nullptr;
        }
        if (is_signed_integer()) {
            return l_builder.CreateSExtOrTrunc(src, m_type, op_name);
        } else {
            LLVM_BUILDER_ASSERT(is_unsigned_integer());
            return l_builder.CreateZExtOrTrunc(src, m_type, op_name);
        }
    }
    llvm::Value *M_floating_point_type_cast(const ValueInfo &src_value,
                                            const std::string &op_name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not src_value.has_error());
        LLVM_BUILDER_ASSERT(m_type != nullptr and not src_value.has_error());
        llvm::Value* src = src_value.native_value();
        llvm::IRBuilder<>& l_builder = CursorContextImpl::builder();
        if (not is_float()) {
            return nullptr;
        }
        if (not src_value.type().is_float()) {
            return nullptr;
        }
        if (not src->getType()->isFloatTy() and not src->getType()->isDoubleTy()) {
            return nullptr;
        }
        return l_builder.CreateFPCast(src, m_type, op_name);
    }
    llvm::Value *M_fixed_to_floating_type_cast(const ValueInfo &src_value,
                                               const std::string &op_name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not src_value.has_error());
        LLVM_BUILDER_ASSERT(m_type != nullptr and not src_value.has_error());
        llvm::Value* src = src_value.native_value();
        llvm::IRBuilder<>& l_builder = CursorContextImpl::builder();
        if (not is_float()) {
            return nullptr;
        }
        if (not src_value.type().is_integer()) {
            return nullptr;
        }
        if (not src->getType()->isIntegerTy()) {
            return nullptr;
        }
        if (src_value.type().is_signed_integer()) {
            return l_builder.CreateSIToFP(src, m_type, op_name);
        } else {
            LLVM_BUILDER_ASSERT(src_value.type().is_unsigned_integer());
            return l_builder.CreateUIToFP(src, m_type, op_name);
        }
    }
    llvm::Value *M_floating_to_fixed_type_cast(const ValueInfo &src_value,
                                               const std::string &op_name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not src_value.has_error());
        LLVM_BUILDER_ASSERT(m_type != nullptr and not src_value.has_error());
        llvm::Value* src = src_value.native_value();
        llvm::IRBuilder<>& l_builder = CursorContextImpl::builder();
        if (not is_integer()) {
            return nullptr;
        }
        if (not src_value.type().is_float()) {
            return nullptr;
        }
        if (not src->getType()->isFloatTy()) {
            return nullptr;
        }
        if (is_signed_integer()) {
            return l_builder.CreateFPToSI(src, m_type, op_name);
        } else {
            LLVM_BUILDER_ASSERT(is_unsigned_integer());
            return l_builder.CreateFPToUI(src, m_type, op_name);
        }
    }
    llvm::Value* type_cast(const ValueInfo &src_value,
                             const std::string &op_name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not src_value.has_error());
        if (m_type == nullptr or src_value.has_error()) {
            return nullptr;
        }
        const TypeInfo& src_type = src_value.type();
        if (src_type.is_integer() and is_integer()) {
            return M_fixed_point_type_cast(src_value, op_name);
        } else if (src_type.is_float() and is_float()) {
            return M_floating_point_type_cast(src_value, op_name);
        } else if (src_type.is_integer() and is_float()) {
            return M_fixed_to_floating_type_cast(src_value, op_name);
        } else if (src_type.is_float() and is_integer()) {
            return M_floating_to_fixed_type_cast(src_value, op_name);
        } else if (src_type.is_boolean() and is_integer()) {
            llvm::Value* src = src_value.native_value();
            llvm::IRBuilder<>& l_builder = CursorContextImpl::builder();
            LLVM_BUILDER_ASSERT(src->getType()->isIntegerTy());
            return l_builder.CreateZExtOrTrunc(src, m_type, op_name);
        } else {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type cast combination not supported");
            return nullptr;
        }
    }
public:
    DECL_EQUIV_FN(boolean)
    DECL_EQUIV_FN(integer)
    DECL_EQUIV_FN(float)
    DECL_EQUIV_FN(signed_integer)
    DECL_EQUIV_FN(unsigned_integer)
    // TODO{vibhanshu}: explore how to integrate NSW and NUW flag when running in safe mode
    BINARY_OP_IMPL_FN(add, CreateAdd, CreateAdd, CreateFAdd)
    BINARY_OP_IMPL_FN(sub, CreateSub, CreateSub, CreateFSub)
    BINARY_OP_IMPL_FN(mult, CreateMul, CreateMul, CreateFMul)
    BINARY_OP_IMPL_FN(div, CreateSDiv, CreateUDiv, CreateFDiv)
    BINARY_OP_IMPL_FN(remainder, CreateSRem, CreateURem, CreateFRem)
    BINARY_OP_IMPL_FN(less_than, CreateICmpSLT, CreateICmpULT, CreateFCmpOLT)
    BINARY_OP_IMPL_FN(less_than_equal, CreateICmpSLE, CreateICmpULE, CreateFCmpOLE)
    BINARY_OP_IMPL_FN(greater_than, CreateICmpSGT, CreateICmpUGT, CreateFCmpOGT)
    BINARY_OP_IMPL_FN(greater_than_equal, CreateICmpSGE, CreateICmpUGE, CreateFCmpOGE)
    BINARY_OP_IMPL_FN(not_equal, CreateICmpNE, CreateICmpNE, CreateFCmpONE)
    BINARY_OP_IMPL_FN(equal, CreateICmpEQ, CreateICmpEQ, CreateFCmpOEQ)
};
#undef DECL_EQUIV_FN
#undef BINARY_OP_IMPL_FN

//
// TypeInfo
//
TypeInfo::TypeInfo() : BaseT{State::ERROR} {
}

TypeInfo::TypeInfo(TypeInfoImpl &impl) : BaseT{State::VALID} {
    CODEGEN_FN
    if (impl.is_valid()) {
        m_impl = impl.ptr();
    } else {
        M_mark_error();
    }
}


TypeInfo::~TypeInfo() = default;

auto TypeInfo::null() -> TypeInfo& {
    static TypeInfo s_null_type{};
    LLVM_BUILDER_ASSERT(s_null_type.has_error());
    return s_null_type;
}

bool TypeInfo::operator == (const TypeInfo& o) const {
    // TODO{vibhashu}: think of a way so that adding a new property will automatically
    //                 lead to it's comparison being done
    if (has_error() and o.has_error()) {
        return true;
    }
    if (has_error() or o.has_error()) {
        return false;
    }
    std::shared_ptr<Impl> ptr1 = m_impl.lock();
    std::shared_ptr<Impl> ptr2 = o.m_impl.lock();
    if (ptr1 and ptr2) {
        return ptr1.get() == ptr2.get();
    } else {
        return false;
    }
}

#define DEF_GETTER(FN_NAME, TYPE, DEFAULT_VALUE)                    \
auto TypeInfo::FN_NAME() const -> TYPE {                        \
    CODEGEN_FN                                                  \
        if (has_error()) {                                      \
            return DEFAULT_VALUE;                               \
        }                                                       \
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {            \
        return ptr-> FN_NAME ();                                \
    } else {                                                    \
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted"); \
        return DEFAULT_VALUE;                                   \
    }                                                           \
}                                                               \
/**/                                                            \

DEF_GETTER(size_in_bytes, uint32_t, std::numeric_limits<uint32_t>::max())
DEF_GETTER(is_valid, bool, false)
DEF_GETTER(is_void, bool, false)
DEF_GETTER(is_boolean, bool, false)
DEF_GETTER(is_integer, bool, false)
DEF_GETTER(is_pointer, bool, false)
DEF_GETTER(is_unsigned_integer, bool, false)
DEF_GETTER(is_signed_integer, bool, false)
DEF_GETTER(is_float, bool, false)
DEF_GETTER(is_scalar, bool, false)
DEF_GETTER(is_array, bool, false)
DEF_GETTER(is_vector, bool, false)
DEF_GETTER(is_struct, bool, false)
DEF_GETTER(native_value, llvm::Type*, nullptr)
DEF_GETTER(base_type, const TypeInfo&, TypeInfo::null())
DEF_GETTER(num_elements, uint32_t, std::numeric_limits<uint32_t>::max())
DEF_GETTER(struct_size_bytes, uint32_t, std::numeric_limits<uint32_t>::max())
DEF_GETTER(struct_name, std::string, "INVALID_STRUCT")
DEF_GETTER(short_name, std::string, "INVALID_NAME")

bool TypeInfo::is_valid_pointer_field() const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_pointer()) {
            const TypeInfo& l_base = ptr->base_type();
            if (l_base.is_struct()) {
                uint32_t l_num_elements = l_base.num_elements();
                for (uint32_t i = 0; i != l_num_elements; ++i) {
                    TypeInfo::field_entry_t l_entry = l_base[i];
                    if (not l_entry.type().is_valid_struct_field()) {
                        return false;
                    }
                }
                return true;
            } else if (l_base.is_array() or l_base.is_vector()) {
                return l_base.base_type().is_valid_struct_field();
            } else if (l_base.is_pointer()) {
                return l_base.is_valid_pointer_field();
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        M_mark_error();
        return false;
    }
}

bool TypeInfo::is_valid_struct_field() const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_scalar()) {
            return true;
        } else if (ptr->is_pointer()) {
            return is_valid_pointer_field();
        } else {
            return false;
        }
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        M_mark_error();
        return false;
    }
}

auto TypeInfo::operator[] (uint32_t i) const -> field_entry_t {
    CODEGEN_FN
    if (has_error()) {
        return field_entry_t::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->operator [](i);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        M_mark_error();
        return field_entry_t::null();
    }
}

auto TypeInfo::operator[] (const std::string& s) const -> field_entry_t {
    CODEGEN_FN
    if (has_error()) {
        return field_entry_t::null();
    }
    if (s.empty()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "can't search for empty field name in struct");
        M_mark_error();
        return field_entry_t::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->operator [](s);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        M_mark_error();
        return field_entry_t::null();
    }
}

void TypeInfo::dump_llvm_type_info(std::ostream& os) const {
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->dump_llvm_type_info(os);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
    }
}

bool TypeInfo::check_sync(const ValueInfo& value) const {
    if (has_error() or value.has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->check_sync(value);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        return false;
    }
}

TypeInfo TypeInfo::pointer_type() const {
    if (has_error()) {
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->pointer_type(*this);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        return TypeInfo::null();
    }
}

#define DEF_MK_TYPE(TYPE_NAME)                                          \
    TypeInfo TypeInfo::mk_ ##TYPE_NAME() {                              \
        CODEGEN_FN                                                      \
        return CursorContextImpl::mk_type_ ##TYPE_NAME();               \
    }                                                                   \
/**/                                                                    

DEF_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DEF_MK_TYPE)

#undef DEF_MK_TYPE

#define DEF_MK_TYPE2(TYPE_NAME)                    \
    template <>                                    \
    TypeInfo TypeInfo::mk_type<TYPE_NAME##_t>() {  \
        return TypeInfo::mk_## TYPE_NAME();        \
    }                                              \
/**/                                               
                                                   
template <>                                    
TypeInfo TypeInfo::mk_type<void>() {  
    return TypeInfo::mk_void();        
}                                              
FOR_EACH_LLVM_TYPE(DEF_MK_TYPE2)

#undef DEF_MKT_TYPE2

TypeInfo TypeInfo::mk_int_context() {
    CODEGEN_FN
    return CursorContextImpl::mk_int_context();
}

TypeInfo TypeInfo::mk_type_from_name(const std::string& name) {
    CODEGEN_FN
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type name can't be empty");
        return TypeInfo::null();
    }
    CString name_view{name};
    if (name_view.equals("bool"_cs)) {
        return TypeInfo::mk_bool();
    } else if(name_view.equals("float"_cs) or name_view.equals("float32_t"_cs)) {
        return TypeInfo::mk_float32();
    } else if(name_view.equals("double"_cs) or name_view.equals("float64_t"_cs)) {
        return TypeInfo::mk_float64();
    } else if(name_view.equals("int8_t"_cs)) {
        return TypeInfo::mk_int8();
    } else if(name_view.equals("uint8_t"_cs)) {
        return TypeInfo::mk_uint8();
    } else if(name_view.equals("int16_t"_cs)) {
        return TypeInfo::mk_int16();
    } else if(name_view.equals("uint16_t"_cs)) {
        return TypeInfo::mk_uint16();
    } else if(name_view.equals("int32_t"_cs)) {
        return TypeInfo::mk_int32();
    } else if(name_view.equals("uint32_t"_cs)) {
        return TypeInfo::mk_uint32();
    } else if(name_view.equals("int64_t"_cs)) {
        return TypeInfo::mk_int64();
    } else if(name_view.equals("uint64_t"_cs)) {
        return TypeInfo::mk_uint64();
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type name not identified:" << name);
        return TypeInfo::null();
    }
}

TypeInfo TypeInfo::mk_array(TypeInfo element_type, uint32_t num_elements) {
    CODEGEN_FN
    if (element_type.has_error()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "element_type invalid for array");
        return TypeInfo::null();
    } else if (element_type.has_error() or num_elements == 0) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "number of elements can't be 0 in array");
        return TypeInfo::null();
    } else if (not element_type.is_scalar() and not element_type.is_pointer()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "array can be formed of only scalar type or of pointer");
        return TypeInfo::null();
    } else {
        return CursorContextImpl::mk_type_array(element_type, num_elements);
    }
}

TypeInfo TypeInfo::mk_vector(TypeInfo element_type, uint32_t num_elements) {
    CODEGEN_FN
    if (element_type.has_error()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "element_type invalid for vector");
        return TypeInfo::null();
    } else if (element_type.has_error() or num_elements == 0) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "number of elements can't be 0 in vector");
        return TypeInfo::null();
    } else if (not element_type.is_scalar()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "vector can be formed of only scalar type");
        return TypeInfo::null();
    } else {
        return CursorContextImpl::mk_type_vector(element_type, num_elements);
    }
}

TypeInfo TypeInfo::mk_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed) {
    CODEGEN_FN
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "struct name can't be empty");
        return TypeInfo::null();
    }
    if (element_list.size() == 0) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "struct can't have 0 fields");
        return TypeInfo::null();
    }
    if (CursorContextImpl::has_value()) {
        return CursorContextImpl::mk_type_struct(name, element_list, is_packed);
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Not in context");
        return TypeInfo::null();
    }
}

TypeInfo TypeInfo::from_raw(llvm::Type* l_raw_type) {
    CODEGEN_FN
    LLVM_BUILDER_ASSERT(l_raw_type != nullptr);
    if (l_raw_type->isVoidTy()) {
        return mk_void();
    } else if (l_raw_type->isFloatTy()) {
        return mk_float32();
    } else if (l_raw_type->isDoubleTy()) {
        return mk_float64();
    } else if (l_raw_type->isIntegerTy(8)) {
        return mk_int8();
    } else if (l_raw_type->isIntegerTy(16)) {
        return mk_int16();
    } else if (l_raw_type->isIntegerTy(32)) {
        return mk_int32();
    } else if (l_raw_type->isIntegerTy(64)) {
        return mk_int64();
    } else if (l_raw_type->isPointerTy()) {
        return mk_void().pointer_type();
    } else if (l_raw_type->isStructTy()) {
        LLVM_BUILDER_ABORT("Struct Type yet to be implemented");
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type not supported");
        return TypeInfo::null();
    }
}

ValueInfo TypeInfo::type_cast(const ValueInfo& src_value, const std::string& op_name) const {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (src_value.has_error()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Can't typecast an invalid value");
        return ValueInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        llvm::Value* l_res = ptr->type_cast(src_value, op_name);
        return ValueInfo{*this, TagInfo{}, l_res};
    } else {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");
        return ValueInfo::null();
    }
}


#define BINARY_VALUE_OP(OP_NAME)                                                                          \
  ValueInfo TypeInfo::OP_NAME(const ValueInfo& lhs, const ValueInfo& rhs,                                 \
                              const std::string& op_name) const {                                         \
    CODEGEN_FN                                                                                            \
    if (has_error()) {                                                                                    \
      return ValueInfo::null();                                                                           \
    }                                                                                                     \
    if (lhs.has_error() or rhs.has_error()) {                                                             \
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Operation can't be applied to invalid values");                   \
        return ValueInfo::null();                                                                         \
    }                                                                                                     \
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {                                                      \
        llvm::Value* l_res = ptr-> OP_NAME(lhs.native_value(), rhs.native_value(), op_name);              \
        return ValueInfo{*this, TagInfo{}, l_res};                                                        \
    } else {                                                                                              \
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "Type already deleted");                                           \
        return ValueInfo::null();                                                                         \
    }                                                                                                     \
}                                                                                                         \
/**/

FOR_EACH_BINARY_OP(BINARY_VALUE_OP)

#undef BINARY_VALUE_OP
 
//
// member_field_entry
//
member_field_entry::member_field_entry(const std::string& name, TypeInfo type, bool is_readonly)
: m_name{name}, m_type{type}, m_is_readonly{is_readonly} {
}
member_field_entry::member_field_entry(const std::string& name, TypeInfo type)
: m_name{name}, m_type{type} {
}

bool member_field_entry::is_valid() const {
    return  m_name.size() > 0 and not type().has_error() and type().is_valid();
}

//
// LinkSymbolName
//
LinkSymbolName::LinkSymbolName() : BaseT{State::ERROR} {
}

LinkSymbolName::LinkSymbolName(const std::string& ns, const std::string& name)
    : BaseT{State::VALID}
    , m_namespace{ns}
    , m_name{name}
    , m_is_global{false}
    , m_full_name{M_full_name()} {
    CODEGEN_FN
    if (m_name.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "name can't be empty in LinkSymbolName");
        M_mark_error();
        return;
    }
    if (m_namespace.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "Namespace can't be empty");
        M_mark_error();
    }
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "LinkSymbolName not valid");
        M_mark_error();
    }
}

LinkSymbolName::LinkSymbolName(const std::string& name)
    : BaseT{State::VALID}
    , m_name{name}
    , m_is_global{true}
    , m_full_name{M_full_name()} {
    CODEGEN_FN
    if (m_name.empty()) {
        CODEGEN_PUSH_ERROR(VALUE_ERROR, "name can't be empty in LinkSymbolName");
        M_mark_error();
        return;
    }
}

std::string LinkSymbolName::M_full_name() const {
    if (has_error()) {
        return "<INVALID_NAME>";
    } else if (is_global()) {
        return m_name;
    } else {
        return LLVM_BUILDER_CONCAT << namespace_name() << s_ns_separator << short_name();
    }
}

const std::string& LinkSymbolName::namespace_name() const {
    if (has_error()) {
        static const std::string s_invalid_ns{"<INVALID_NS>"};
        return s_invalid_ns;
    } else if (is_global()) {
        static const std::string s_global_ns_name{};
        return s_global_ns_name;
    } else {
        return m_namespace;
    }
}

bool LinkSymbolName::is_valid() const {
    if (has_error()) {
        return false;
    }
    bool r = not m_name.empty();
    if (not is_global()) {
        r &= not m_namespace.empty();
    }
    return r;
}

LinkSymbolName LinkSymbolName::null() {
    static LinkSymbolName s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// LinkSymbol
//
bool LinkSymbol::Arg::operator == (const Arg& o) const {
    return m_type == o.m_type and m_name == o.m_name;
}

LinkSymbol::LinkSymbol() : BaseT{State::ERROR} {
}

LinkSymbol::LinkSymbol(const LinkSymbolName& name,
                       const TypeInfo& ctype,
                       symbol_type type)
    : BaseT{State::VALID}
    , m_symbol_name{name}
    , m_ctype{ctype}
    , m_type{type} {
    M_ensure_valid();
    object::Counter::singleton().on_new(object::Callback::object_t::LINK_SYMBOL, (uint64_t)this, m_symbol_name.short_name());
}

LinkSymbol::~LinkSymbol() {
    object::Counter::singleton().on_delete(object::Callback::object_t::LINK_SYMBOL, (uint64_t)this, m_symbol_name.short_name());
}

void LinkSymbol::M_ensure_valid() const {
    if (m_symbol_name.has_error() or m_ctype.has_error()) {
        M_mark_error();
        return;
    }
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "LinkSymbol not valid");
        M_mark_error();
    } else if (not m_symbol_name.is_valid()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "LinkSymbolName not valid");
        M_mark_error();
    } else if (not m_ctype.is_valid()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "LinkSymbol Type not valid");
        M_mark_error();
    } else if (is_custom_struct()) {
        if (not m_ctype.is_struct()) {
            CODEGEN_PUSH_ERROR(LINK_SYMBOL, "LinkSymbol of struct type incorrect tag");
            M_mark_error();
        }
    } else if(not is_function()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "LinkSymbol is not of any identified type");
        M_mark_error();
    }
}

void LinkSymbol::add_arg(const TypeInfo& type, const std::string& name) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (type.has_error()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "arg type can't be invalid");
        M_mark_error();
        return;
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "arg name can't be empty");
        M_mark_error();
        return;
    }
    if (is_function()) {
        m_arg_list.emplace_back(type, name);
    } else {
        CODEGEN_PUSH_ERROR(LINK_SYMBOL, "not a function to add arg");
        M_mark_error();
    }
}

bool LinkSymbol::operator == (const LinkSymbol& o) const {
    if (has_error() and o.has_error()) {
        return true;
    }
    const bool r = symbol_name() == o.symbol_name();
    LLVM_BUILDER_ASSERT(meta::bool_if(r, m_ctype == o.m_ctype and m_type == o.m_type));
    if (r and is_function()) {
        if (m_arg_list.size() != o.m_arg_list.size()) {
            return false;
        }
        for (uint32_t i = 0; i != m_arg_list.size(); ++i) {
            if (m_arg_list[i] != o.m_arg_list[i]) {
                return false;
            }
        }
    }
    return r;
}

LinkSymbol LinkSymbol::null() {
    static LinkSymbol s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// TypeInfoImpl
//
TypeInfoImpl::TypeInfoImpl(void_construct_t, CursorPtr cursor_impl, llvm::Type* type) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(void_construct_t{}, cursor_impl, type);
}

TypeInfoImpl::TypeInfoImpl(int_construct_t, CursorPtr cursor_impl, llvm::IntegerType* type, bool is_signed) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(int_construct_t{}, cursor_impl, type, is_signed);
}

TypeInfoImpl::TypeInfoImpl(int_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(int_construct_t{}, cursor_impl, type, num_bytes);
}

TypeInfoImpl::TypeInfoImpl(float_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(float_construct_t{}, cursor_impl, type, num_bytes);
}

TypeInfoImpl::TypeInfoImpl(pointer_construct_t, CursorPtr cursor_impl, llvm::Type* type, uint32_t num_bytes, const TypeInfo& base_type) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(pointer_construct_t{}, cursor_impl, type, num_bytes, base_type);
}

TypeInfoImpl::TypeInfoImpl(array_construct_t, CursorPtr cursor_impl, TypeInfo element_type, uint32_t array_size) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(array_construct_t{}, cursor_impl, element_type, array_size);
}

TypeInfoImpl::TypeInfoImpl(vector_construct_t, CursorPtr cursor_impl, TypeInfo element_type, uint32_t vector_size) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(vector_construct_t{}, cursor_impl, element_type, vector_size);
}

TypeInfoImpl::TypeInfoImpl(struct_construct_t, CursorPtr cursor_impl, const std::string& name, const std::vector<member_field_entry>& field_types, const bool packed) {
    LLVM_BUILDER_ASSERT(cursor_impl.is_valid());
    m_impl = std::make_shared<Impl>(struct_construct_t{}, cursor_impl, name, field_types, packed);
}


LLVM_BUILDER_NS_END
