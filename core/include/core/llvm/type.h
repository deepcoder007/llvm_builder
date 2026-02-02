//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_LLVM_TYPE_H_
#define CORE_LLVM_TYPE_H_

#include "core/llvm/defines.h"
#include "core/util/defines.h"
#include "core/util/callbacks.h"
#include "core/util/object.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace llvm {
    class Type;
}

LLVM_NS_BEGIN

// TODO{vibhanshu}: add support for un-ordered floating point also (i.e. NaN)
// TODO{vibhanshu}: add support for unsigned int types

class ValueInfo;
class TypeInfoImpl;

class member_field_entry;

#define DECL_EQUIV_FN(fn)                                                     \
    bool is_##fn##_equiv () const {                                           \
        return is_##fn () or (is_vector() and base_type().is_##fn ());        \
    }                                                                         \
    /**/                                                                      \

class TypeInfo : public _BaseObject<TypeInfo> {
    using BaseT = _BaseObject<TypeInfo>;
    friend class ValueInfo;
    friend class TypeInfoImpl;
    struct void_construct_t{};
    struct int_construct_t{};
    struct float_construct_t{};
    struct pointer_construct_t {};
    struct array_construct_t {};
    struct vector_construct_t {};
    struct struct_construct_t {};
public:
    class field_entry_t : public _BaseObject<field_entry_t> {
        using BaseT = _BaseObject<field_entry_t>;
    private:
        const uint32_t m_idx;
        const uint32_t m_offset;
        // TODO{vibhanshu}: maybe need to add field alignment also
        const std::string& m_name;
        const TypeInfo& m_type;
        const bool m_is_readonly = false;
    public:
        explicit field_entry_t(const uint32_t idx,
                               const uint32_t offset,
                               const std::string& name,
                               const TypeInfo& type,
                               bool is_readonly);
        // explicit field_entry_t(const uint32_t idx, const uint32_t offset, const std::pair<std::string, TypeInfo>& entry);
        ~field_entry_t();
    public:
        uint32_t idx() const {
            return m_idx;
        }
        uint32_t offset() const {
            return m_offset;
        }
        const std::string& name() const {
            return m_name;
        }
        const TypeInfo& type() const {
            return m_type;
        }
        bool is_readonly() const {
            return m_is_readonly;
        }
        bool operator == (const field_entry_t& o) const;
        static field_entry_t& null();
        void print(std::ostream& os) const;
        OSTREAM_FRIEND(field_entry_t)
    };
    static_assert(sizeof(field_entry_t) == 5*sizeof(std::nullptr_t));
    class Impl;
private:
    struct derived_info_t;
private:
    std::weak_ptr<Impl> m_impl;
public:
    explicit TypeInfo();
    explicit TypeInfo(TypeInfoImpl& impl);
    ~TypeInfo();
public:
    uint32_t size_in_bytes() const;
    bool is_valid() const;
    bool is_void() const;
    bool is_boolean() const;
    bool is_integer() const;
    bool is_pointer() const;
    bool is_unsigned_integer() const;
    bool is_signed_integer() const;
    bool is_float() const;
    bool is_scalar() const;
    bool is_array() const;
    bool is_vector() const;
    bool is_struct() const;
    bool operator == (const TypeInfo& o) const;
    llvm::Type *native_value() const;
    const TypeInfo& base_type() const;
    field_entry_t operator[] (uint32_t i) const;
    field_entry_t operator[] (const std::string& s) const;
    uint32_t num_elements() const;
    uint32_t struct_size_bytes() const;
    std::string struct_name() const;
    std::string short_name() const;
    bool is_valid_pointer_field() const;
    bool is_valid_struct_field() const;
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(TypeInfo);
    void dump_llvm_type_info(std::ostream& os) const;
    bool check_sync(const ValueInfo& value) const;
public:
    TypeInfo pointer_type() const;
    DECL_EQUIV_FN(boolean)
    DECL_EQUIV_FN(integer)
    DECL_EQUIV_FN(float)
    DECL_EQUIV_FN(signed_integer)
    DECL_EQUIV_FN(unsigned_integer)
    ValueInfo type_cast(const ValueInfo& src_value, const std::string& op_name) const;
#define MK_BINARY_FN(FN_NAME)                                                                           \
    ValueInfo FN_NAME(const ValueInfo& lhs, const ValueInfo& rhs, const std::string& op_name) const;    \
/**/
    FOR_EACH_BINARY_OP(MK_BINARY_FN) 
#undef MK_BINARY_FN
public:
    static TypeInfo& null();
#define DECL_MK_TYPE(TYPE_NAME)              \
    static TypeInfo mk_ ##TYPE_NAME();       \
/**/ 

DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE
    template <typename T>
    static TypeInfo mk_type();
    static TypeInfo mk_int_context();
    static TypeInfo mk_type_from_name(const std::string& name);
    static TypeInfo mk_array(TypeInfo element_type, uint32_t num_elements);
    static TypeInfo mk_vector(TypeInfo element_type, uint32_t num_elements);
    static TypeInfo mk_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed = true);
    static TypeInfo from_raw(llvm::Type* l_raw_type);
};
#undef DECL_EQUIV_FN

class member_field_entry {
    std::string m_name;
    TypeInfo m_type = TypeInfo::null();
    bool m_is_readonly = false;
public:
    explicit member_field_entry() = default;
    explicit member_field_entry(const std::string& name, TypeInfo type, bool is_readonly);
    explicit member_field_entry(const std::string& name, TypeInfo type);
public:
    const std::string& name() const {
        return m_name;
    }
    const TypeInfo& type() const {
        return m_type;
    }
    bool is_readonly() const {
        return m_is_readonly;
    }
    bool is_valid() const;
    bool operator == (const member_field_entry& o) const {
        return m_name == o.m_name and m_type == o.m_type;
    }
};

class LinkSymbolName : public _BaseObject<LinkSymbolName> {
    using BaseT = _BaseObject<LinkSymbolName>;
private:
    // TODO{vibhanshu}: do we need to support hierarchical namespace?
    // TODO{vibhanshu}: maybe use fixed_string to store strings,
    //                  i.e. allow only interned symbols for faster lookup
    const std::string m_namespace;
    const std::string m_name;
    const bool m_is_global = false;
    const std::string m_full_name;
    static std::string s_ns_separator;
public:
    explicit LinkSymbolName();
    explicit LinkSymbolName(const std::string& ns, const std::string& name);
    explicit LinkSymbolName(const std::string& name);
public:
    const std::string& short_name() const {
        return m_name;
    }
    bool is_global() const {
        return m_is_global;
    }
    const std::string& namespace_name() const;
    const std::string& full_name() const {
        return m_full_name;
    }
    bool equals_name(const std::string& name) const {
        return m_full_name == name;
    }
    bool is_valid() const;
    bool operator == (const LinkSymbolName& o) const {
        return m_full_name == o.m_full_name;
    }
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(LinkSymbolName)
    static LinkSymbolName null();
private:
    std::string M_full_name() const;
};

class LinkSymbol : public _BaseObject<LinkSymbol> {
    using BaseT = _BaseObject<LinkSymbol>;
    using field_entry_t = typename TypeInfo::field_entry_t;
public:
    enum class symbol_type : uint8_t {
        unknown,
        custom_struct,
        function
    };
private:
    struct Arg {
        const TypeInfo m_type;
        const std::string m_name;
    public:
        const std::string& name() const {
            return m_name;
        }
        const TypeInfo& type() const {
            return m_type;
        }
        bool operator == (const Arg& o) const;
    };
private:
    const LinkSymbolName m_symbol_name;
    const TypeInfo m_ctype;
    const symbol_type m_type = symbol_type::unknown;
    std::vector<Arg> m_arg_list;
public:
    explicit LinkSymbol();
    explicit LinkSymbol(const LinkSymbolName& name,
                        const TypeInfo& ctype,
                        symbol_type type);
    LinkSymbol(LinkSymbol&&) = default;
    LinkSymbol(const LinkSymbol&) = default;
    ~LinkSymbol();
public:
    const LinkSymbolName& symbol_name() const {
        return m_symbol_name;
    }
    const std::string& full_name() const {
        return symbol_name().full_name();
    }
    bool equals_name(const std::string& name) const {
        return m_symbol_name.equals_name(name);
    }
    bool is_valid() const {
        return m_type != symbol_type::unknown;
    }
    bool is_custom_struct() const {
        return m_type == symbol_type::custom_struct;
    }
    bool is_function() const {
        return m_type == symbol_type::function;
    }
    void add_arg(const TypeInfo& type, const std::string& name);
    bool operator == (const LinkSymbol& o) const;
    void print(std::ostream& os) const;
    OSTREAM_FRIEND(LinkSymbol)
    static LinkSymbol null();
private:
    void M_ensure_valid() const;
};

LLVM_NS_END

#endif // CORE_LLVM_TYPE_H_
