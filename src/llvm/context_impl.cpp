//
// Created by vibhanshu on 2025-12-11
// 

#include "context_impl.h"
#include "ds/fixed_string.h"

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
llvm::LLVMContext& CursorContextImpl::ctx() {
    LLVM_BUILDER_ASSERT(has_value())
    return CursorPtr{Cursor::Context::value()}.ctx();
}

llvm::IRBuilder<> &CursorContextImpl::builder() {
    LLVM_BUILDER_ASSERT(has_value());
    return CursorPtr{Cursor::Context::value()}.builder();
}

TypeInfo CursorContextImpl::context_type() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.context_type();
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

Function CursorContextImpl::mk_function(const std::string& name, bool is_external) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_function(name, is_external);
    } else {
        return Function::null("Cursor not found in context");
    }
}

TypeInfo CursorContextImpl::mk_type_pointer(const TypeInfo& base_type) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_pointer(base_type);
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

#define DECL_MK_TYPE(TYPE_NAME)                                           \
TypeInfo CursorContextImpl::mk_type_##TYPE_NAME() {                       \
    if (has_value()) {                                                    \
        return CursorPtr{Cursor::Context::value()}.mk_type_##TYPE_NAME(); \
    } else {                                                              \
        return TypeInfo::null("Cursor not found in context");             \
    }                                                                     \
}                                                                         \
/**/ 

DECL_MK_TYPE(void)
FOR_EACH_LLVM_TYPE(DECL_MK_TYPE)
#undef DECL_MK_TYPE

Event CursorContextImpl::find_event(const std::string& name) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.find_event(name);
    } else {
        return Event::null("Cursor not found in context");
    }
}

TypeInfo CursorContextImpl::mk_type_fn_type() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_fn_type();
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

TypeInfo CursorContextImpl::mk_type_array(TypeInfo element_type, uint32_t num_elements) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_array(element_type, num_elements);
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

TypeInfo CursorContextImpl::mk_type_vector(TypeInfo element_type, uint32_t num_elements) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_vector(element_type, num_elements);
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

TypeInfo CursorContextImpl::mk_type_struct(const std::string& name, const std::vector<field_entry_t>& element_list, bool is_packed) {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.mk_type_struct(name, element_list, is_packed);
    } else {
        return TypeInfo::null("Cursor not found in context");
    }
}

bool CursorContextImpl::is_bind_called() {
    if (has_value()) {
        return CursorPtr{Cursor::Context::value()}.is_bind_called();
    } else {
        return false;
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

