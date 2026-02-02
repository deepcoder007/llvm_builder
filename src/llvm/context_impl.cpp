//
// Created by vibhanshu on 2025-12-11
// 

#include "context_impl.h"
#include "llvm_builder/ds/fixed_string.h"

LLVM_BUILDER_NS_BEGIN

//
// CursorPtr
// 
CursorPtr::CursorPtr(Cursor &parent) : m_impl{parent.m_impl} {
    LLVM_BUILDER_ASSERT(not m_impl.expired());
}

CursorPtr::CursorPtr(const std::weak_ptr<Impl>& impl) : m_impl{impl} {
    LLVM_BUILDER_ASSERT(not m_impl.expired());
}

CursorPtr::~CursorPtr() = default;

//
// CursorContextImpl
// 
const std::string& CursorContextImpl::name() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.name();
    } else  {
        return StringManager::null();
    }
}

llvm::LLVMContext& CursorContextImpl::ctx() {
    LLVM_BUILDER_ASSERT(has_value())
    return CursorPtr{Cursor::Context::value()}.ctx();
}

llvm::IRBuilder<> &CursorContextImpl::builder() {
    LLVM_BUILDER_ASSERT(has_value());
    return CursorPtr{Cursor::Context::value()}.builder();
}

llvm::orc::ThreadSafeContext& CursorContextImpl::thread_safe_context() {
    LLVM_BUILDER_ASSERT(has_value());
    return CursorPtr{Cursor::Context::value()}.thread_safe_context();
}

Module CursorContextImpl::main_module() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.main_module();
    } else {
        return Module::null();
    }
}

Module CursorContextImpl::gen_module() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.gen_module();
    } else {
        return Module::null();
    }
}

Function CursorContextImpl::mk_function(FunctionImpl&& fn_impl) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_function(std::move(fn_impl));
    } else {
        return Function::null();
    }
}

TypeInfo CursorContextImpl::mk_type_pointer(const TypeInfo& base_type) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_pointer(base_type);
    } else {
        return TypeInfo::null();
    }
}

#define DECL_MK_TYPE(TYPE_NAME)                                           \
TypeInfo CursorContextImpl::mk_type_##TYPE_NAME() {                       \
    if (has_value()) {                                                    \
        return CursorPtr{Cursor::Context::value()}.mk_type_##TYPE_NAME(); \
    } else {                                                              \
        CODEGEN_PUSH_ERROR(CONTEXT, "No context found");                  \
        return TypeInfo::null();                                          \
    }                                                                     \
}                                                                         \
/**/ 

DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE

TypeInfo CursorContextImpl::mk_int_context() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_int_context();
    } else {
        return TypeInfo::null();
    }
}

TypeInfo CursorContextImpl::mk_type_array(TypeInfo element_type, uint32_t num_elements) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_array(element_type, num_elements);
    } else {
        return TypeInfo::null();
    }
}

TypeInfo CursorContextImpl::mk_type_vector(TypeInfo element_type, uint32_t num_elements) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_vector(element_type, num_elements);
    } else {
        return TypeInfo::null();
    }
}

TypeInfo CursorContextImpl::mk_type_struct(const std::string& name, const std::vector<member_field_entry>& element_list, bool is_packed) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_struct(name, element_list, is_packed);
    } else {
        return TypeInfo::null();
    }
}

void CursorContextImpl::main_module_hook_fn(on_main_module_fn_t &&fn) {
    if (has_value()) {
        CursorPtr{Cursor::Context::value()}.main_module_hook_fn(std::move(fn));
    }
}

bool CursorContextImpl::is_bind_called() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.is_bind_called();
    } else {
        return false;
    }
}

void CursorContextImpl::bind() {
    if (has_value()) {
        CursorPtr{Cursor::Context::value()}.bind();
    }
}

llvm::DataLayout CursorContextImpl::get_host_data_layout() {
    static bool s_llvm_init = false;
    if (!s_llvm_init) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        s_llvm_init = true;
    }
    auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (not JTMB) {
        llvm::consumeError(JTMB.takeError());
        return llvm::DataLayout("");
    }
    auto TM = JTMB->createTargetMachine();
    if (not TM) {
        llvm::consumeError(TM.takeError());
        return llvm::DataLayout("");
    }
    return (*TM)->createDataLayout();
}

LLVM_BUILDER_NS_END

