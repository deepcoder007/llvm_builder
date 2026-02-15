//
// Created by vibhanshu on 2025-10-31
//

#ifndef LLVM_BUILDER_LLVM_CONTEXT_IMPL_H_
#define LLVM_BUILDER_LLVM_CONTEXT_IMPL_H_

#include "llvm_builder/module.h"
#include "llvm_builder/function.h"
#include "ext_include.h"
#include "util/debug.h"

LLVM_BUILDER_NS_BEGIN

//
// CursorPtr
//
class CursorPtr {
    using on_main_module_fn_t = typename Cursor::on_main_module_fn_t;
    using on_module_fn_t = typename Cursor::on_module_fn_t;
    using Impl = typename Cursor::Impl;
private:
    std::weak_ptr<Impl> m_impl;
public:
    CursorPtr(Cursor &parent);
    CursorPtr(const std::weak_ptr<Impl>& impl);
    CursorPtr(CursorPtr&&) = default;
    CursorPtr(const CursorPtr&) = default;
    ~CursorPtr();
public:
    bool is_valid() const;
    const std::string& name() const;
    Module main_module();
    Module gen_module();
    Function mk_function(FunctionImpl&& fn_impl);
    TypeInfo mk_type_pointer(const TypeInfo& base_type);
#define DECL_MK_TYPE(TYPE_NAME)            \
    TypeInfo mk_type_ ##TYPE_NAME();       \
/**/ 
DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE
    TypeInfo mk_int_context();
    TypeInfo mk_type_fn_type();
    TypeInfo mk_type_array(TypeInfo element_type, uint32_t num_elements);
    TypeInfo mk_type_vector(TypeInfo element_type, uint32_t num_elements);
    TypeInfo mk_type_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed);
    void main_module_hook_fn(on_main_module_fn_t &&fn);
    bool is_bind_called() const;
    void bind();
    void cleanup();
    void for_each_module(on_module_fn_t&& fn);
public:
    // TODO{vibhanshu}: make these private
    llvm::LLVMContext& ctx();
    llvm::IRBuilder<>& builder();
    llvm::orc::ThreadSafeContext& thread_safe_context();
};

//
// CursorContextImpl
//
class CursorContextImpl {
    using on_main_module_fn_t = typename Cursor::on_main_module_fn_t;
public:
    static bool has_value() {
        return Cursor::Context::has_value();
    }
    static const std::string& name();
    static llvm::LLVMContext& ctx();
    static llvm::IRBuilder<> &builder();
    static llvm::orc::ThreadSafeContext& thread_safe_context(); 
    static Module main_module();
    static Module gen_module();
    static Function mk_function(FunctionImpl&& fn_impl);
    static TypeInfo mk_type_pointer(const TypeInfo& base_type);
#define DECL_MK_TYPE(TYPE_NAME)                   \
    static TypeInfo mk_type_ ##TYPE_NAME();       \
/**/ 
DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE
    static TypeInfo mk_int_context();
    static TypeInfo mk_type_fn_type();
    static TypeInfo mk_type_array(TypeInfo element_type, uint32_t num_elements);
    static TypeInfo mk_type_vector(TypeInfo element_type, uint32_t num_elements);
    static TypeInfo mk_type_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed);
    static void main_module_hook_fn(on_main_module_fn_t &&fn);
    static bool is_bind_called();
    static void bind();
    static llvm::DataLayout get_host_data_layout();
};

//
// FunctionImpl
// 
class FunctionImpl {
    using Impl = typename Function::Impl;
    using c_construct = typename Function::c_construct;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit FunctionImpl(Module parent, const std::string& fn_name,
                      TypeInfo return_type, const FnContext& context, c_construct);
    explicit FunctionImpl(const std::string& external_fn_name,
                      TypeInfo return_type, const FnContext& context, c_construct);
    explicit FunctionImpl(Module parent, llvm::Function& raw_fn, c_construct);
    ~FunctionImpl();
public:
    FunctionImpl(FunctionImpl&&);
    FunctionImpl& operator = (FunctionImpl&&);
public:
    std::weak_ptr<Impl> ptr() {
        return m_impl;
    }
    bool is_valid() const {
        return static_cast<bool>(m_impl);
    }
};

class CodeSectionImpl {
    using Impl = typename CodeSection::Impl;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit CodeSectionImpl(const std::string& name, Function fn);
    ~CodeSectionImpl();
public:
    std::weak_ptr<Impl> ptr() {
        return m_impl;
    }
    bool is_valid() const {
        return static_cast<bool>(m_impl);
    }
    const std::string& name() const;
};

class TypeInfoImpl {
    using Impl = typename TypeInfo::Impl;
public:
    using void_construct_t = typename TypeInfo::void_construct_t;
    using int_construct_t = typename TypeInfo::int_construct_t;
    using float_construct_t = typename TypeInfo::float_construct_t;
    using pointer_construct_t = typename TypeInfo::pointer_construct_t;
    using array_construct_t = typename TypeInfo::array_construct_t;
    using vector_construct_t = typename TypeInfo::vector_construct_t;
    using struct_construct_t = typename TypeInfo::struct_construct_t;
    using function_construct_t = typename TypeInfo::function_construct_t;
private:
    std::shared_ptr<Impl> m_impl;
public:
    // explicit TypeInfoImpl(llvm::Type* type, uint32_t num_bytes, const TypeInfo* base_type = nullptr);
    explicit TypeInfoImpl(void_construct_t, CursorPtr, llvm::Type* type);
    explicit TypeInfoImpl(int_construct_t, CursorPtr, llvm::IntegerType* type, bool is_signed);
    explicit TypeInfoImpl(int_construct_t, CursorPtr, llvm::Type* type, uint32_t num_bytes);
    explicit TypeInfoImpl(float_construct_t, CursorPtr, llvm::Type* type, uint32_t num_bytes);
    explicit TypeInfoImpl(pointer_construct_t, CursorPtr, llvm::Type* type, uint32_t num_bytes, const TypeInfo& base_type);
    explicit TypeInfoImpl(array_construct_t, CursorPtr, TypeInfo element_type, uint32_t array_size);
    explicit TypeInfoImpl(vector_construct_t, CursorPtr, TypeInfo element_type, uint32_t vector_size);
    explicit TypeInfoImpl(struct_construct_t, CursorPtr, const std::string& name, const std::vector<member_field_entry>& field_types, const bool packed);
    explicit TypeInfoImpl(function_construct_t, CursorPtr, llvm::FunctionType* type);
public:
    std::weak_ptr<Impl> ptr() {
        return m_impl;
    }
    bool is_valid() const {
        return static_cast<bool>(m_impl);
    }
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_CONTEXT_IMPL_H_
