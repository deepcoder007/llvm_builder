//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/module.h"
#include "llvm_builder/function.h"
#include "context_impl.h"
#include "llvm_builder/type.h"
#include "llvm_builder/analyze.h"
#include "ds/fixed_string.h"
#include "util/debug.h"
#include "meta/noncopyable.h"
#include "util/cstring.h"
#include "ext_include.h"
#include "llvm/TargetParser/Host.h"

LLVM_BUILDER_NS_BEGIN

class ModuleImpl {
    using Impl = typename Module::Impl;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit ModuleImpl(const std::string& name, const std::weak_ptr<Cursor::Impl>& cursor_impl);
    ~ModuleImpl() = default;
public:
    std::weak_ptr<Impl> ptr() const {
        return m_impl;
    }
};

//
// Cursor::Impl
//
class Cursor::Impl : meta::noncopyable {
    friend class CursorPtr;
private:
    const std::string m_name;
    llvm::orc::ThreadSafeContext m_ts_context;
    llvm::LLVMContext& m_context;
    llvm::IRBuilder<> m_ir_builder;
    std::unordered_map<std::string, ModuleImpl> m_modules;
    std::vector<FunctionImpl> m_func_list;
    std::vector<TypeInfoImpl> m_type_list;
#define DECL_MK_TYPE(TYPE_NAME)         \
    TypeInfo m_type_ ##TYPE_NAME;       \
/**/ 
DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE
    std::unordered_map<uint32_t, std::vector<TypeInfo>> m_array_types;
    std::unordered_map<uint32_t, std::vector<TypeInfo>> m_vector_types;
    TypeInfo m_int_context;
    std::vector<std::pair<TypeInfo, TypeInfo>> m_pointer_cache;
    Module m_main_module;
    std::vector<on_main_module_fn_t> m_main_mod_hook;
    bool m_is_bind = false;
    bool m_is_deleted = false;
public:
    explicit Impl(const std::string &name)
      : m_name{name}, m_ts_context{std::make_unique<llvm::LLVMContext>()},
        m_context{*m_ts_context.withContextDo([](llvm::LLVMContext* ctx) { return ctx; })},
        m_ir_builder{m_context} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty());
        object::Counter::singleton().on_new(object::Callback::object_t::CURSOR, (uint64_t)this, m_name);
    }
    ~Impl() {
        CODEGEN_FN
        m_ir_builder.ClearInsertionPoint();
        object::Counter::singleton().on_delete(object::Callback::object_t::CURSOR, (uint64_t)this, m_name);
    }
    bool is_bind_called() const {
        return m_is_bind;
    }
    bool is_valid() const {
        return not m_is_deleted;
    }
    void cleanup() {
        m_modules.clear();
        m_func_list.clear();
        m_type_list.clear();
#define DECL_DEL_TYPE(TYPE_NAME)                                               \
        m_type_ ##TYPE_NAME = TypeInfo::null();                                \
/**/
DECL_DEL_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_DEL_TYPE)
#undef DECL_DEL_TYPE
        m_is_deleted = true;
    }
    void init(Cursor& parent) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not parent.has_error());
        CursorPtr ptr{parent};
        LLVM_BUILDER_ASSERT(ptr.is_valid());
        {
            llvm::Type* l_type = llvm::Type::getVoidTy(m_context);
            LLVM_BUILDER_ASSERT(l_type->isVoidTy());
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::void_construct_t{}, ptr, l_type);
            m_type_void = TypeInfo{l_type_impl};
        }
        {
            llvm::Type* l_type = llvm::Type::getInt1Ty(m_context);
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::int_construct_t{}, ptr, l_type, 1);
            m_type_bool = TypeInfo{l_type_impl};
        }
        {
            llvm::Type* l_type = llvm::Type::getFloatTy(m_context);
            LLVM_BUILDER_ASSERT(l_type->isFloatTy());
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::float_construct_t{}, ptr, l_type, sizeof(float));
            m_type_float32 = TypeInfo{l_type_impl};
        }
        {
            llvm::Type* l_type = llvm::Type::getDoubleTy(m_context);
            LLVM_BUILDER_ASSERT(l_type->isDoubleTy());
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::float_construct_t{}, ptr, l_type, sizeof(double));
            m_type_float64 = TypeInfo{l_type_impl};
        }
#define ADD_SIGNED_TYPE(TYPE_SIZE)                                                                                                \
        {                                                                                                                         \
            llvm::IntegerType* l_type = llvm::Type::getInt##TYPE_SIZE##Ty(m_context);                                             \
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::int_construct_t{}, ptr, l_type, true); \
            m_type_int##TYPE_SIZE = TypeInfo{l_type_impl};                                                                        \
        }                                                                                                                         \
/**/
#define ADD_UNSIGNED_TYPE(TYPE_SIZE)                                                                                              \
        {                                                                                                                         \
            llvm::IntegerType* l_type = llvm::Type::getInt##TYPE_SIZE##Ty(m_context);                                             \
            TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::int_construct_t{}, ptr, l_type, false);\
            m_type_uint##TYPE_SIZE = TypeInfo{l_type_impl};                                                                       \
        }                                                                                                                         \
/**/
        ADD_SIGNED_TYPE(8)
        ADD_SIGNED_TYPE(16)
        ADD_SIGNED_TYPE(32)
        ADD_SIGNED_TYPE(64)
        ADD_UNSIGNED_TYPE(8)
        ADD_UNSIGNED_TYPE(16)
        ADD_UNSIGNED_TYPE(32)
        ADD_UNSIGNED_TYPE(64)
#undef ADD_SIGNED_TYPE
#undef ADD_UNSIGNED_TYPE
        LLVM_BUILDER_ASSERT(not ErrorContext::has_error());
    }
private:
    const std::string &name() const {
        return m_name;
    }
    Module main_module() const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_bind_called());
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(m_main_module.is_init());
        return m_main_module;
    }
    Module gen_module(const std::weak_ptr<Impl>& ptr) {
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(is_bind_called());
        const std::string l_mod_name = LLVM_BUILDER_CONCAT << m_name << "_" << m_modules.size();
        return M_gen_module(l_mod_name, ptr);
    }
    Function mk_function(FunctionImpl&& fn_impl) {
        CODEGEN_FN
        // TODO{vibhanshu}: add checks to avoid name clashes
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(fn_impl.is_valid());
        FunctionImpl& impl =  m_func_list.emplace_back(std::move(fn_impl));
        LLVM_BUILDER_ASSERT(not fn_impl.is_valid());
        return Function{impl};
    }
    TypeInfo mk_type_pointer(const TypeInfo& base_type, CursorPtr parent) {
        CODEGEN_FN
        // TODO{vibhanshu}: allow pointer type of struct, array, void, vector only -> disallow pointer of scalar types
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not base_type.has_error());
        for (const std::pair<TypeInfo, TypeInfo>& ptr : m_pointer_cache) {
            if (ptr.first == base_type) {
                return ptr.second;
            }
        }
        llvm::Type* l_type = llvm::PointerType::get(m_context, 0);
        TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::pointer_construct_t{}, parent, l_type, sizeof(int*), base_type);
        TypeInfo ptr_type{l_type_impl};
        m_pointer_cache.emplace_back(std::make_pair(base_type, ptr_type));
        return ptr_type;
    }
#define DECL_MK_TYPE(TYPE_NAME)                   \
    TypeInfo mk_type_ ##TYPE_NAME() {             \
        LLVM_BUILDER_ASSERT(is_valid());                  \
        return m_type_ ##TYPE_NAME;               \
    }                                             \
/**/
DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE
    TypeInfo mk_type_array(TypeInfo element_type, uint32_t num_elements, CursorPtr parent) {
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not element_type.has_error());
        LLVM_BUILDER_ASSERT(num_elements != 0);
        LLVM_BUILDER_ASSERT(element_type.is_valid_struct_field());
        std::vector<TypeInfo>& l_vec = m_array_types[num_elements];
        for (const TypeInfo& l_info : l_vec) {
            if (l_info.base_type() == element_type) {
                return l_info;
            }
        }
        TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::array_construct_t{}, parent, element_type, num_elements);
        return l_vec.emplace_back(l_type_impl);
    }
    TypeInfo mk_type_vector(TypeInfo element_type, uint32_t num_elements, CursorPtr parent) {
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not element_type.has_error());
        LLVM_BUILDER_ASSERT(num_elements != 0);
        LLVM_BUILDER_ASSERT(element_type.is_scalar())
        std::vector<TypeInfo>& l_vec = m_vector_types[num_elements];
        for (const TypeInfo& l_info : l_vec) {
            if (l_info.base_type() == element_type) {
                return l_info;
            }
        }
        TypeInfoImpl& l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::vector_construct_t{}, parent, element_type, num_elements);
        return l_vec.emplace_back(l_type_impl);
    }
    TypeInfo mk_int_context() {
        LLVM_BUILDER_ASSERT(is_valid());
        if (m_int_context.is_null()) {
            std::vector<member_field_entry> args;
            args.emplace_back("arg", TypeInfo::mk_int32());
            m_int_context = TypeInfo::mk_struct("int_arg", args, false);
        }
        return m_int_context;
    }
    TypeInfo mk_type_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed, CursorPtr parent) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not name.empty());
        LLVM_BUILDER_ASSERT(element_list.size() > 0);
        LLVM_BUILDER_ASSERT(not is_bind_called());
        for ([[maybe_unused]] const member_field_entry& l_entry : element_list) {
            LLVM_BUILDER_ASSERT(l_entry.is_valid());
            LLVM_BUILDER_ASSERT(l_entry.type().is_valid_struct_field());
        }
        TypeInfoImpl l_type_impl = m_type_list.emplace_back(typename TypeInfoImpl::struct_construct_t{}, parent, name, element_list, is_packed);
        TypeInfo l_type{l_type_impl};
        if (not ErrorContext::has_error()) {
            main_module_hook_fn([l_type, name] (Module& module) {
                LinkSymbol l_link_symbol{LinkSymbolName{name}, l_type, LinkSymbol::symbol_type::custom_struct};
                module.register_symbol(l_link_symbol);
                module.add_struct_definition(l_type);
            });
            return l_type;
        } else {
            return TypeInfo::null();
        }
    }
    void main_module_hook_fn(on_main_module_fn_t &&fn) {
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not is_bind_called());
        LLVM_BUILDER_ASSERT(fn);
        m_main_mod_hook.emplace_back(std::move(fn));
    }
    void bind(const std::weak_ptr<Impl>& ptr) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not is_bind_called());
        m_is_bind = true;
        m_main_module = M_gen_module(m_name, ptr);
        Module::Context l_module_ctx{m_main_module};
        for (on_main_module_fn_t& fn : m_main_mod_hook) {
            fn(m_main_module);
        }
    }
    void for_each_module(on_module_fn_t &&fn) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(is_bind_called());
        LLVM_BUILDER_ASSERT(m_main_module.is_init());
        for (const auto &kv : m_modules) {
            Module l_module{kv.second};
            LLVM_BUILDER_ASSERT(l_module.is_init());
            fn(l_module);
        }
    }
private:
    Module M_gen_module(const std::string& mod_name, const std::weak_ptr<Impl>& ptr) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(is_bind_called());
        LLVM_BUILDER_ASSERT(not mod_name.empty());
        LLVM_BUILDER_ASSERT(not ptr.expired());
        LLVM_BUILDER_ASSERT(not Module::Context::has_value());
        auto it = m_modules.try_emplace(mod_name, mod_name, ptr);
        LLVM_BUILDER_ASSERT(it.second);
        Module l_module{it.first->second};
        l_module.init_standard();
        return l_module;
    }
    // TODO{vibhanshu}: these 3 functions below are supposed to be retired
    llvm::LLVMContext& ctx() {
        return m_context;
    }
    llvm::IRBuilder<>& builder() {
        return m_ir_builder;
    }
    llvm::orc::ThreadSafeContext& thread_safe_ctx() {
        return m_ts_context;
    }
};

//
// CursorPtr
// 
bool CursorPtr::is_valid() const {
    return not m_impl.expired();
}

const std::string& CursorPtr::name() const {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->name();
    } else {
        static std::string s_null{"<NULL>"};
        return s_null;
    }
}

Module CursorPtr::main_module() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            if (ptr->is_bind_called()) {
                return ptr->main_module();
            } else {
                CODEGEN_PUSH_ERROR(MODULE, " bind not called for Cursor:" << name());
                return Module::null();
            }
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
            return Module::null();
        }
    } else {
        return Module::null();
    }
}

Module CursorPtr::gen_module() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            if (ptr->is_bind_called()) {
                return ptr->gen_module(m_impl);
            } else {
                CODEGEN_PUSH_ERROR(MODULE, " bind not called for Cursor:" << name());
                return Module::null();
            }
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
            return Module::null();
        }
    } else {
        return Module::null();
    }
} 

Function CursorPtr::mk_function(FunctionImpl&& fn_impl) {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            return ptr->mk_function(std::move(fn_impl));
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
            return Function::null();
        }
    } else {
        return Function::null();
    }
}

TypeInfo CursorPtr::mk_type_pointer(const TypeInfo& base_type) {
    if (base_type.has_error()) {
        CODEGEN_PUSH_ERROR(MODULE, " can't define pointer to an invalid type");
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            return ptr->mk_type_pointer(base_type, *this);
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
        }
    }
    return TypeInfo::null();
}

#define DECL_MK_TYPE(TYPE_NAME)                                           \
TypeInfo CursorPtr::mk_type_##TYPE_NAME() {                         \
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {                      \
        if (ptr->is_valid()) {                                            \
            return ptr->mk_type_##TYPE_NAME();                            \
        } else {                                                          \
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());    \
        }                                                                 \
    }                                                                     \
    return TypeInfo::null();                                              \
}                                                                         \
/**/ 

DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE

TypeInfo CursorPtr::mk_int_context() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            return ptr->mk_int_context();
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
        }
    }
    return TypeInfo::null();
}

TypeInfo CursorPtr::mk_type_array(TypeInfo element_type, uint32_t num_elements) {
    if (element_type.has_error()) {
        CODEGEN_PUSH_ERROR(MODULE, " can't define array to an invalid type");
        return TypeInfo::null();
    }
    if (num_elements == 0) {
        CODEGEN_PUSH_ERROR(MODULE, " can't define array of 0 elements");
        return TypeInfo::null();
    }
    if (not element_type.is_valid_struct_field()) {
        CODEGEN_PUSH_ERROR(MODULE, "array can't be defined for this field type:" << element_type.short_name());
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            return ptr->mk_type_array(element_type, num_elements, *this);
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
        }
    }
    return TypeInfo::null();
}

TypeInfo CursorPtr::mk_type_vector(TypeInfo element_type, uint32_t num_elements) {
    if (element_type.has_error()) {
        CODEGEN_PUSH_ERROR(MODULE, " can't define vector to an invalid type");
        return TypeInfo::null();
    }
    if (num_elements == 0) {
        CODEGEN_PUSH_ERROR(MODULE, " can't define vector of 0 elements");
        return TypeInfo::null();
    }
    if (not element_type.is_scalar()) {
        CODEGEN_PUSH_ERROR(MODULE, "vector can be defined of only scalar type");
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            return ptr->mk_type_vector(element_type, num_elements, *this);
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
        }
    }
    return TypeInfo::null();
}

TypeInfo CursorPtr::mk_type_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed) {
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(CONTEXT, "can't define struct without name");
        return TypeInfo::null();
    }
    if (element_list.size() == 0) {
        CODEGEN_PUSH_ERROR(CONTEXT, "can't define struct without fields");
        return TypeInfo::null();
    }
    bool has_error = false;
    for (const member_field_entry &l_element : element_list) {
        if (not l_element.is_valid()) {
            CODEGEN_PUSH_ERROR(CONTEXT, "can't define struct with invalid entry");
            return TypeInfo::null();
        }
        const TypeInfo& l_element_type = l_element.type();
        if (l_element_type.has_error() or not l_element_type.is_valid_struct_field()) {
            CODEGEN_PUSH_ERROR(CONTEXT, "fields of struct is not of valid type:" << l_element.name());
            has_error = true;
        }
        // TODO{vibhanshu}: test the whole hierarchy of struct deeply
        // rules are:
        // 1. struct and array entries should be scalar or pointer
        // 2. pointer is allowed only of struct and array (not scalar or void)
        // this rule needs to be tested recursively over struct definition
        // any failure rejects this structure
    }
    if (has_error) {
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            if (not ptr->is_bind_called()) {
                return ptr->mk_type_struct(name, element_list, is_packed, *this);
            } else {
                CODEGEN_PUSH_ERROR(MODULE, " can't create new type after binding cursor:" << this->name());
            }
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << this->name());
        }
    }
    return TypeInfo::null();
}

void CursorPtr::main_module_hook_fn(on_main_module_fn_t &&fn) {
    if (not fn) {
        CODEGEN_PUSH_ERROR(CONTEXT, "can't add an empty function callback");
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            if (not ptr->is_bind_called()) {
                ptr->main_module_hook_fn(std::move(fn));
            } else {
                CODEGEN_PUSH_ERROR(MODULE, " can't create new type after binding cursor:" << name());
            }
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " already deleted Cursor:" << name());
            return;
        }
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "Cursor no longer valid");
    }
}

bool CursorPtr::is_bind_called() const {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_bind_called();
    } else {
        return false;
    }
}

void CursorPtr::bind() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_bind_called()) {
            CODEGEN_PUSH_ERROR(MODULE, " bind already called for Cursor:" << name());
        } else {
            ptr->bind(m_impl);
        }
    }
}

void CursorPtr::cleanup() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_valid()) {
            ptr->cleanup();
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " Cursor already deleted:" << name());
        }
    }
}

void CursorPtr::for_each_module(on_module_fn_t&& fn) {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_bind_called()) {
            return ptr->for_each_module(std::move(fn));
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " modules not yet initialied for cursor:" << name());
        }
    }
}

// TODO{vibhanshu}: make these private
llvm::LLVMContext& CursorPtr::ctx() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->ctx();
    } else {
        LLVM_BUILDER_ABORT(LLVM_BUILDER_CONCAT << "No cursor found");
    }
}

llvm::IRBuilder<>& CursorPtr::builder() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->builder();
    } else {
        LLVM_BUILDER_ABORT(LLVM_BUILDER_CONCAT << "No cursor found");
    }
}

llvm::orc::ThreadSafeContext& CursorPtr::thread_safe_context() {
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->thread_safe_ctx();
    } else {
        LLVM_BUILDER_ABORT(LLVM_BUILDER_CONCAT << "No cursor found");
    }
}


//
// Cursor
//
Cursor::Cursor(const std::string& name) : BaseT{State::VALID} {
    m_impl = std::make_shared<Impl>(name);
    m_impl->init(*this);
}

Cursor::Cursor() : BaseT{State::ERROR} {
}

Cursor::~Cursor() = default;

auto Cursor::name() -> const std::string& {
    CODEGEN_FN
    if (has_error()) {
        static std::string s_name{"<ERROR_CURSOR>"};
        return s_name;
    }
    return CursorPtr{*this}.name();
}

Module Cursor::main_module() {
    CODEGEN_FN
    if (has_error()) {
        return Module::null();
    }
    return CursorPtr{*this}.main_module();
}

Module Cursor::gen_module() {
    CODEGEN_FN
    if (has_error()) {
        return Module::null();
    }
    return CursorPtr{*this}.gen_module();
}

void Cursor::main_module_hook_fn(on_main_module_fn_t &&fn) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    return CursorPtr{*this}.main_module_hook_fn(std::move(fn));
}

bool Cursor::is_bind_called() {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    return CursorPtr{*this}.is_bind_called();
}

void Cursor::bind() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    CursorPtr{*this}.bind();
}

void Cursor::cleanup() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    CursorPtr{*this}.cleanup();
}

void Cursor::for_each_module(on_module_fn_t&& fn) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (not fn) {
        CODEGEN_PUSH_ERROR(MODULE, "action defined to be null");
        return;
    }
    CursorPtr{*this}.for_each_module(std::move(fn));
}

bool Cursor::operator == (const Cursor& o) const {
    if (has_error() and o.has_error()) {
        return true;
    }
    return m_impl.get() == o.m_impl.get();
}

Cursor Cursor::null() {
    static Cursor s_null_cursor{};
    LLVM_BUILDER_ASSERT(s_null_cursor.has_error());
    return s_null_cursor;
}

CONTEXT_DEF(Cursor)

//
//  Module::Impl
//
class Module::Impl : meta::noncopyable {
    friend class Module;
private:
    const std::string m_name;
    CursorPtr m_cursor_impl;
    std::unique_ptr<llvm::Module> m_raw_module;
    // TODO{vibhanshu}: add check that one {module_name}_{symbol_name} combination
    //                  should have a unique ctype, symbol_type
    std::vector<LinkSymbol> m_public_symbols;
    std::unordered_map<std::string, TypeInfo> m_structs;
    bool m_is_init = false;
    // static std::vector<std::string> s_module_names;
public:
    explicit Impl(const std::string& name, const std::weak_ptr<Cursor::Impl>& cursor_impl)
        : m_name{name}, m_cursor_impl{cursor_impl} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty());
        LLVM_BUILDER_ASSERT(m_cursor_impl.is_valid());
        CodeSectionContext::assert_no_context();
        m_raw_module = std::make_unique<llvm::Module>(llvm::StringRef{name}, m_cursor_impl.ctx());
        object::Counter::singleton().on_new(object::Callback::object_t::MODULE, (uint64_t)this, m_name);
    }
    ~Impl() {
        object::Counter::singleton().on_delete(object::Callback::object_t::MODULE, (uint64_t)this, m_name);
    }
public:
    bool is_init() const {
        return m_is_init and static_cast<bool>(m_raw_module);
    }
    const std::string& name() const {
        return m_name;
    }
    CursorPtr cursor() {
        return m_cursor_impl;
    }
    const std::vector<LinkSymbol>& public_symbols() const {
        return m_public_symbols;
    }
    TypeInfo struct_type(const std::string &name) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty());
        if (m_structs.contains(name)) {
            return m_structs.at(name);
        } else {
            CODEGEN_PUSH_ERROR(MODULE, " struct not found :" << name);
            return TypeInfo::null();
        }
    }
    llvm::Module *native_handle() const {
        return m_raw_module.get();
    }
    std::vector<std::string> exported_symbol_names() const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_init());
        return transformed_public_symbols([this] (const LinkSymbol& symbol) -> std::string {
            return symbol.full_name();
        });
    }
    std::vector<std::string> transformed_public_symbols(std::function<std::string(const LinkSymbol &)> &&fn) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_init());
        LLVM_BUILDER_ASSERT(fn);
        std::vector<std::string> l_ret;
        for (const LinkSymbol& l_sym: m_public_symbols) {
            l_ret.emplace_back(fn(l_sym));
        }
        return l_ret;
    }
    std::string public_symbol_name(const std::string &symbol) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not symbol.empty());
        if (contains(symbol)) {
            return LinkSymbolName{symbol}.full_name();
        } else {
            CODEGEN_PUSH_ERROR(MODULE, "Symbol not found:" << symbol);
            return std::string{};
        }
    }
    bool contains(const std::string &symbol) const {
        LLVM_BUILDER_ASSERT(not symbol.empty());
        for (const LinkSymbol& l_sym : m_public_symbols) {
            if (l_sym.equals_name(symbol)) {
                return true;
            }
        }
        return false;
    }
    void register_symbol(const LinkSymbol &link_symbol) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not link_symbol.has_error());
        LLVM_BUILDER_ASSERT(not link_symbol.full_name().empty());
        LLVM_BUILDER_ASSERT(not contains(link_symbol.full_name()));
        LLVM_BUILDER_ASSERT(link_symbol.is_valid());
        m_public_symbols.emplace_back(link_symbol);
    }
    void init_standard() {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not is_init());
        llvm::DataLayout l_data_layout = CursorContextImpl::get_host_data_layout();
        LLVM_BUILDER_ASSERT(m_raw_module);
        m_raw_module->setDataLayout(l_data_layout);
        std::string target_triple = l_data_layout.getStringRepresentation().empty()
            ? std::string("x86_64-unknown-linux-gnu")
            : llvm::sys::getDefaultTargetTriple();
        m_raw_module->setTargetTriple(llvm::Triple{target_triple});
        m_raw_module->setPICLevel(llvm::PICLevel::BigPIC);
        m_raw_module->setPIELevel(llvm::PIELevel::Large);
        m_is_init = true;
    }
    Function get_function(Module& parent, const std::string &name) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not parent.has_error());
        LLVM_BUILDER_ASSERT(not name.empty());
        const std::string l_full_name = LinkSymbolName{name}.full_name();
        LLVM_BUILDER_ASSERT(m_raw_module);
        llvm::Function* l_fn = m_raw_module->getFunction(l_full_name);
        if (l_fn == nullptr) {
            l_fn = m_raw_module->getFunction(name);
        }
        LLVM_BUILDER_ASSERT(l_fn != nullptr);
        FunctionImpl impl{parent, *l_fn, Function::c_construct{}};
        if (impl.is_valid()) {
            return m_cursor_impl.mk_function(std::move(impl));
        } else {
            CODEGEN_PUSH_ERROR(MODULE, "unable to create valid function");
            return Function::null();
        }
    }
    void add_struct_definition(Module& parent, const TypeInfo &struct_type) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not parent.has_error());
        LLVM_BUILDER_ASSERT(not struct_type.has_error());
        auto it = m_structs.try_emplace(struct_type.struct_name(), struct_type);
        if (not it.second) {
            CODEGEN_PUSH_ERROR(TYPE_ERROR, "struct type already exist with name:" << struct_type.struct_name());
        }
#if 0
        std::vector<Function> r_fn_defs;
        const std::string l_struct_full_name = struct_type.struct_name();
        {
            FunctionBuilder fn_builder;
            fn_builder.set_context(FnContext{arg_type.pointer_type()})
                .set_module(parent)
                .set_name(LinkSymbolName{l_struct_full_name, "size"}.full_name());
            Function l_size_fn = fn_builder.compile();
            {
                CodeSection l_fn_body = l_size_fn.mk_section("fn_body");
                l_fn_body.enter();
                ValueInfo struct_size = ValueInfo::calc_struct_size(struct_type);
                ValueInfo ctx = CodeSectionContext::current_context();
                ctx.field("result").store(struct_size);
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
            l_size_fn.verify();
            r_fn_defs.emplace_back(l_size_fn);
        }
        {
            FunctionBuilder fn_builder;
            fn_builder.set_context(FnContext{arg_type.pointer_type()})
                .set_module(parent)
                .set_name(LinkSymbolName{l_struct_full_name, "field_count"}.full_name());
            Function l_field_count_fn = fn_builder.compile();
            {
                CodeSection l_fn_body = l_field_count_fn.mk_section("fn_body");
                l_fn_body.enter();
                ValueInfo field_count = ValueInfo::calc_struct_field_count(struct_type);
                ValueInfo ctx = CodeSectionContext::current_context();
                ctx.field("result").store(field_count);
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
            l_field_count_fn.verify();
            r_fn_defs.emplace_back(l_field_count_fn);
        }
        {
            FunctionBuilder fn_builder;
            fn_builder.set_context(FnContext{TypeInfo::mk_int_context().pointer_type()})
                .set_module(parent)
                .set_name(LinkSymbolName{l_struct_full_name, "get_field_offset"}.full_name());
            Function l_field_offset_fn = fn_builder.compile();
            {
                CodeSection l_fn_body = l_field_offset_fn.mk_section("fn_body");
                l_fn_body.enter();
                ValueInfo ctx = CodeSectionContext::pop("context"_cs);
                ValueInfo idx = ctx.field("offset").load();
                ValueInfo offset = ValueInfo::calc_struct_field_offset(struct_type, idx);
                ctx.field("result").store(offset);
                CodeSectionContext::set_return_value(ValueInfo::from_constant(0));
            }
            // TODO{vibhanshu}: make a cleaner interface to do this ?
            l_field_offset_fn.verify();
            r_fn_defs.emplace_back(l_field_offset_fn);
        }
        return r_fn_defs;
#endif
    }
    // TODO{vibhanshu}: remove usage of C files as much possible, if not remove, reduce the size
    void write_to_file() const {
        CODEGEN_FN
        write_llvm_output("");
    }
    void write_llvm_output(const std::string& suffix) const {
        CODEGEN_FN
        const std::string stage_file_path = [suffix, this]() -> std::string {
            const std::string l_base_dir{"."};
            if (suffix.empty()) {
                return LLVM_BUILDER_CONCAT << l_base_dir << "/" << name() << ".ll";
            } else {
                return LLVM_BUILDER_CONCAT << l_base_dir << "/" << name() << "_" << suffix << ".ll";
            }
        } ();
        llvm::Error l_err = llvm::writeToOutput(llvm::StringRef{stage_file_path}, [this] (llvm::raw_ostream& os) -> llvm::Error {
            LLVM_BUILDER_ASSERT(m_raw_module);
            m_raw_module->print(os, nullptr);
            return llvm::Error::success();
        });
        LLVM_BUILDER_ASSERT(not l_err);
    }
    void write_to_ostream() const {
        LLVM_BUILDER_ASSERT(m_raw_module);
        // TODO{vibhanshu}: add assembly annotation for
        m_raw_module->print(llvm::errs(), nullptr);
    }
};

//
// ModuleImpl
// 
ModuleImpl::ModuleImpl(const std::string& name, const std::weak_ptr<Cursor::Impl>& cursor_impl) {
    m_impl = std::make_shared<Impl>(name, cursor_impl);
}

//
// Module
//
Module::Module(const ModuleImpl& impl)
    : BaseT{State::VALID} {
    CODEGEN_FN
    m_impl = impl.ptr();
}

Module::Module() : BaseT{State::ERROR} {
}

Module::~Module() = default;

bool Module::is_init() const {
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_init();
    } else {
        M_mark_error();
        return false;
    }
}

const std::string& Module::name() const {
    CODEGEN_FN
    if (has_error()) {
        return StringManager::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->name();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return StringManager::null();
    }
}

auto Module::public_symbols() const -> const std::vector<LinkSymbol>& {
    CODEGEN_FN
    static const std::vector<LinkSymbol> s_null{};
    if (has_error()) {
        return s_null;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->public_symbols();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return s_null;
    }
}

auto Module::struct_type(const std::string& name) const -> TypeInfo {
    CODEGEN_FN
    if (has_error()) {
        return TypeInfo::null();
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(MODULE, "can't search an empty string in a module");
        M_mark_error();
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->struct_type(name);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return TypeInfo::null();
    }
}

llvm::Module* Module::native_handle() const {
    CODEGEN_FN
    if (has_error()) {
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->native_handle();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return nullptr;
    }
}

std::vector<std::string> Module::exported_symbol_names() const {
    CODEGEN_FN
    if (has_error()) {
        return std::vector<std::string>{};
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_init()) {
            return ptr->exported_symbol_names();
        } else {
            CODEGEN_PUSH_ERROR(MODULE, "module not init");
            return std::vector<std::string>{};
        }
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return std::vector<std::string>{};
    }
}

std::vector<std::string> Module::transformed_public_symbols(std::function<std::string(const LinkSymbol&)>&& fn) const {
    CODEGEN_FN
    if (has_error()) {
        return std::vector<std::string>{};
    }
    if (not fn) {
        CODEGEN_PUSH_ERROR(MODULE, "trying to have null transform operation on symbols");
        M_mark_error();
        return std::vector<std::string>{};
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_init()) {
            return ptr->transformed_public_symbols(std::move(fn));
        } else {
            CODEGEN_PUSH_ERROR(MODULE, "module not init");
            return std::vector<std::string>{};
        }
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return std::vector<std::string>{};
    }
}

auto Module::public_symbol_name(const std::string& symbol) const -> std::string {
    CODEGEN_FN
    if (has_error()) {
        return StringManager::null();
    }
    if (symbol.empty()) {
        CODEGEN_PUSH_ERROR(MODULE, "can't search for an empty symbol");
        M_mark_error();
        return StringManager::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->public_symbol_name(symbol);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return StringManager::null();
    }
}

bool Module::contains(const std::string& symbol) const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (symbol.empty()) {
        CODEGEN_PUSH_ERROR(MODULE, "can't search for an empty symbol");
        M_mark_error();
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->contains(symbol);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return false;
    }
}

void Module::register_symbol(const LinkSymbol& link_symbol) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (link_symbol.has_error()) {
        CODEGEN_PUSH_ERROR(MODULE, "can't register an invalid symbol");
        M_mark_error();
        return;
    }
    if (link_symbol.full_name().empty()) {
        CODEGEN_PUSH_ERROR(MODULE, "can't register empty symbol");
        M_mark_error();
        return;
    }
    if (not link_symbol.is_valid()) {
        CODEGEN_PUSH_ERROR(MODULE, "link symbol not valid:" << link_symbol.full_name());
        M_mark_error();
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->contains(link_symbol.full_name())) {
            CODEGEN_PUSH_ERROR(MODULE, "symbol already exists:" << link_symbol.full_name());
            M_mark_error();
            return;
        }
        return ptr->register_symbol(link_symbol);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return;
    }
}

void Module::init_standard() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_init()) {
            CODEGEN_PUSH_ERROR(MODULE, "module already init:" << name());
            M_mark_error();
            return;
        }
        ptr->init_standard();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
    }
}

std::unique_ptr<llvm::orc::ThreadSafeModule> Module::take_thread_safe_module() {
    CODEGEN_FN
    if (has_error()) {
        return nullptr;
    }
    if (not is_init()) {
        CODEGEN_PUSH_ERROR(MODULE, "thread safe module can't be created:" << name());
        M_mark_error();
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        auto tsm = std::make_unique<llvm::orc::ThreadSafeModule>(
            std::move(ptr->m_raw_module), ptr->m_cursor_impl.thread_safe_context());
        ptr->m_is_init = false;
        return tsm;
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return nullptr;
    }
}

Function Module::get_function(const std::string& name) {
    CODEGEN_FN
    if (has_error()) {
        return Function::null();
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(MODULE, "Trying to search function with empty name");
        M_mark_error();
        return Function::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->get_function(*this, name);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
        return Function::null();
    }
}

void Module::add_struct_definition(const TypeInfo& struct_type) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (struct_type.has_error()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "struct type invalid");
        M_mark_error();
        return;
    }
    if (not struct_type.is_struct()) {
        CODEGEN_PUSH_ERROR(TYPE_ERROR, "type is not struct");
        M_mark_error();
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->add_struct_definition(*this, struct_type);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
    }
}

void Module::write_llvm_output(const std::string& suffix) const {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (not is_init()) {
        CODEGEN_PUSH_ERROR(MODULE, "module not yet init" << name());
        M_mark_error();
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->write_llvm_output(suffix);
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
    }
}

void Module::write_to_file() const {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (not is_init()) {
        CODEGEN_PUSH_ERROR(MODULE, "module not yet init" << name());
        M_mark_error();
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->write_to_file();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
    }
}

void Module::write_to_ostream() const {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (not is_init()) {
        CODEGEN_PUSH_ERROR(MODULE, "module not yet init" << name());
        M_mark_error();
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->write_to_ostream();
    } else {
        CODEGEN_PUSH_ERROR(MODULE, "module already deleted");
        M_mark_error();
    }
}

bool Module::operator==(const Module &o) const {
    if (has_error() and o.has_error()) {
        return true; 
    }
    std::shared_ptr<Impl> ptr1 = m_impl.lock();
    std::shared_ptr<Impl> ptr2 = o.m_impl.lock();
    if (ptr1 and ptr2) {
        return ptr1.get() == ptr2.get();
    } else {
        return false;
    }
}

Module Module::null() {
    static Module s_module{};
    LLVM_BUILDER_ASSERT(s_module.has_error());
    return s_module;
}

CONTEXT_DEF(Module)

LLVM_BUILDER_NS_END
