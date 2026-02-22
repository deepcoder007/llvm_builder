//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/function.h"
#include "llvm_builder/analyze.h"
#include "meta/noncopyable.h"
#include "ds/fixed_string.h"
#include "util/debug.h"
#include "llvm_builder/module.h"
#include "llvm/context_impl.h"

#include <iostream>

LLVM_BUILDER_NS_BEGIN

//
// Function::Impl
//
class Function::Impl : meta::noncopyable {
    Module m_parent;
    const std::string m_fn_name;
    const bool m_is_external = false;
    llvm::Argument* m_raw_arg = nullptr;
    llvm::FunctionType* m_type = nullptr;
    llvm::Function* m_fn = nullptr;
    LinkSymbol m_link_symbol;
    std::vector<CodeSectionImpl> m_section_list;
public:
    explicit Impl(const LinkSymbolName& symbol_name
                  , Module parent
                  , const std::string& fn_name
                  , bool is_external)
          : m_parent{parent}
          , m_fn_name{fn_name}
          , m_is_external{is_external}
          , m_type{M_mk_fn_type()}
          , m_fn{M_mk_fn(parent)}
          , m_link_symbol{symbol_name, TypeInfo::mk_int32(), LinkSymbol::symbol_type::function} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not m_parent.has_error());
        LLVM_BUILDER_ASSERT(not m_fn_name.empty());
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not m_link_symbol.has_error());
        LLVM_BUILDER_ASSERT(CursorContextImpl::has_value());
        [[maybe_unused]] bool has_arg = false;
        for (auto& l_arg_value : m_fn->args()) {
            m_link_symbol.add_arg(CursorContextImpl::context_type(), "context");
            m_raw_arg = &l_arg_value;
            LLVM_BUILDER_ASSERT(not has_arg);
            has_arg = true;
        }
        LLVM_BUILDER_ASSERT(has_arg);
        object::Counter::singleton().on_new(object::Callback::object_t::FUNCTION, (uint64_t)this, m_fn_name);
    }
    ~Impl() {
        object::Counter::singleton().on_delete(object::Callback::object_t::FUNCTION, (uint64_t)this, m_fn_name);
    }
public:
    bool is_valid() const {
        return m_type != nullptr and m_fn != nullptr;
    }
    const Module& parent_module() const {
        return m_parent;
    }
    const std::string& name() const {
        return m_fn_name;
    }
    bool is_external() const {
        return m_is_external;
    }
    ValueInfo call_fn() const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(Module::Context::has_value());
        Module& l_current_module = Module::Context::value();
        LLVM_BUILDER_ASSERT(not l_current_module.has_error());
        llvm::Module* l_raw_module = l_current_module.native_handle();
        LLVM_BUILDER_ASSERT(l_raw_module != nullptr);
        llvm::Function* l_fn_to_call = m_fn;
        if (l_current_module != m_parent) {
            llvm::Function* l_existing = l_raw_module->getFunction(m_fn->getName());
            if (l_existing == nullptr) {
                declare_fn(m_parent, l_current_module);
                l_existing = l_raw_module->getFunction(m_fn->getName());
            }
            l_fn_to_call = l_existing;
        }
        return ValueInfo{l_fn_to_call, typename ValueInfo::construct_fn_t{}};
    }
    void declare_fn(const Module& src_mod, const Module& dst_mod) const {
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not src_mod.has_error())
        LLVM_BUILDER_ASSERT(not dst_mod.has_error());
        if (&dst_mod != &m_parent) {
            M_mk_fn(dst_mod);
        }
    }
    llvm::Function* native_handle() const {
        LLVM_BUILDER_ASSERT(is_valid());
        return m_fn;
    }
    void verify() {
        LLVM_BUILDER_ASSERT(is_valid());
        llvm::verifyFunction(*m_fn, &llvm::errs());
    }
    void remove_from_module() {
        LLVM_BUILDER_ASSERT(is_valid());
        m_fn->eraseFromParent();
    }
    void write_to_ostream() const {
        LLVM_BUILDER_ASSERT(is_valid());
        // TODO{vibhanshu}: add assembly annotation for debugging
        m_fn->print(llvm::errs(), nullptr);
    }
    llvm::Value* M_eval_arg() const {
        LLVM_BUILDER_ASSERT(m_fn->arg_size() == 1);
        return m_fn->getArg(0);
    }
    const LinkSymbol& link_symbol() const {
        return m_link_symbol;
    }
    CodeSection mk_section(const std::string& name, Function& fn) {
        // TODO{vibhanshu}: flag which section in functions are unused and give compilation error accordingly
        LLVM_BUILDER_ASSERT(not name.empty());
        LLVM_BUILDER_ASSERT(not fn.has_error());
        for (CodeSectionImpl& section : m_section_list) {
            if (section.name() == name) {
                CODEGEN_PUSH_ERROR(CODE_SECTION, "Duplicate code-section name:" << name);
                return CodeSection::null();
            }
        }
        CodeSectionImpl& l_entry = m_section_list.emplace_back(name, fn);
        return CodeSection{l_entry};
    }
private:
    static llvm::FunctionType* M_mk_fn_type() {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(CursorContextImpl::has_value());
        TypeInfo l_ctx_type = CursorContextImpl::context_type();
        LLVM_BUILDER_ASSERT(not l_ctx_type.has_error());
        llvm::FunctionType* l_fn_type = llvm::FunctionType::get(TypeInfo::mk_int32().native_value(), {l_ctx_type.native_value()}, false);
        LLVM_BUILDER_ASSERT(l_fn_type != nullptr);
        return l_fn_type;
    }
    llvm::Function* M_mk_fn(const Module& dest_mod) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not dest_mod.has_error());
        llvm::Function* l_fn = llvm::Function::Create(m_type,
                                                      llvm::GlobalValue::ExternalLinkage,
                                                      m_fn_name,
                                                      *dest_mod.native_handle());
        LLVM_BUILDER_ASSERT(l_fn != nullptr);
        l_fn->setMustProgress();
        l_fn->setDSOLocal(true);
        l_fn->setDoesNotRecurse();
        l_fn->setDoesNotThrow();
        for (auto& arg: l_fn->args()) {
            arg.setName("context");
        }
        return l_fn;
    }
};


//
// Function
//
Function::Function()
  : BaseT{State::ERROR} {
}

Function::Function(FunctionImpl& impl)
  : BaseT{State::VALID} {
    CODEGEN_FN
    if (impl.is_valid()) {
        m_impl = impl.ptr();
    } else {
        M_mark_error();
    }
}

Function::Function(const std::string& name, Module& mod)
  : BaseT{State::VALID} {
    CODEGEN_FN
    if (not CursorContextImpl::has_value()) {
        CODEGEN_PUSH_ERROR(CONTEXT, "function can't be compiled as context not found");
        M_mark_error();
        return;
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function name not set");
        M_mark_error();
        return;
    }
    if (mod.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "module not valid");
        M_mark_error();
        return;
    }
    FunctionImpl impl(mod, name, c_construct{});
    Function fn = CursorContextImpl::mk_function(std::move(impl));
    LLVM_BUILDER_ASSERT(not impl.is_valid());
    if (fn.has_error()) {
        M_mark_error();
        return;
    }
    m_impl = fn.m_impl;
}

Function::Function(const std::string& name)
  : BaseT{State::VALID} {
    CODEGEN_FN
    if (not CursorContextImpl::has_value()) {
        CODEGEN_PUSH_ERROR(CONTEXT, "function can't be compiled as context not found");
        M_mark_error();
        return;
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function name not set");
        M_mark_error();
        return;
    }
    FunctionImpl impl(name, c_construct{});
    Function fn = CursorContextImpl::mk_function(std::move(impl));
    LLVM_BUILDER_ASSERT(not impl.is_valid());
    if (fn.has_error()) {
        M_mark_error();
        return;
    }
    m_impl = fn.m_impl;
}

Function::~Function() = default;

bool Function::is_valid() const {
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_valid();
    } else {
        M_mark_error();
        return false;
    }
}

const Module& Function::parent_module() const {
    static const Module s_null{};
    if (has_error()) {
        return s_null;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->parent_module();
    } else {
        M_mark_error();
        return s_null;
    }
}

const std::string& Function::name() const {
    if (has_error()) {
        return StringManager::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->name();
    } else {
        M_mark_error();
        return StringManager::null();
    }
}

bool Function::is_external() const {
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_external();
    } else {
        M_mark_error();
        return false;
    }
}

ValueInfo Function::call_fn() const {
    if (has_error()) {
        return ValueInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->call_fn();
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

void Function::declare_fn(Module& src_mod, Module& dst_mod) {
    if (has_error()) {
        return;
    }
    if (src_mod.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "trying to declare function with invalid src module");
        return;
    }
    if (dst_mod.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "trying to declare function with invalid dst module");
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->declare_fn(src_mod, dst_mod);
    } else {
        M_mark_error();
    }
}

CodeSection Function::mk_section(const std::string& name) {
    if (has_error()) {
        return CodeSection::null();
    }
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "code-section name can't be empty");
        M_mark_error();
        return CodeSection::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->mk_section(name, *this);
    } else {
        M_mark_error();
        return CodeSection::null();
    }
}

llvm::Function *Function::native_handle() const {
    if (has_error()) {
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->native_handle();
    } else {
        M_mark_error();
        return nullptr;
    }
}

void Function::verify() {
    if (ErrorContext::has_error()) {
        ErrorContext::print(std::cout, 5);
    }
    if (has_error()) {
        LLVM_BUILDER_ABORT("function has error");
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->verify();
    } else {
        M_mark_error();
        LLVM_BUILDER_ABORT("function is a null object");
    }
}

void Function::remove_from_module() {
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->remove_from_module();
    } else {
        M_mark_error();
    }
}

void Function::write_to_ostream() const {
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->write_to_ostream();
    } else {
        M_mark_error();
    }
}

bool Function::operator == (const Function& o) const {
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

auto Function::null() -> Function {
    static Function s_func{};
    LLVM_BUILDER_ASSERT(s_func.has_error());
    return s_func; 
}

llvm::Value* Function::M_eval_arg() const {
    if (has_error()) {
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->M_eval_arg();
    } else {
        return nullptr;
    }
}

//
// FunctionImpl
//
FunctionImpl::FunctionImpl(Module parent, const std::string& fn_name, c_construct) {
    CODEGEN_FN
    if (parent.has_error()) {
        return;
    }
    if (fn_name.empty()) {
        return;
    }
    LinkSymbolName sym_name{fn_name};
    m_impl = std::make_shared<Impl>(sym_name, parent, fn_name, false);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:" << fn_name);
        return;
    }
    parent.register_symbol(m_impl->link_symbol());
}

FunctionImpl::FunctionImpl(const std::string& external_fn_name, c_construct) {
    CODEGEN_FN
    if (external_fn_name.empty()) {
        return;
    }
    LinkSymbolName sym_name{external_fn_name};
    m_impl = std::make_shared<Impl>(sym_name, Module::null(), sym_name.full_name(), true);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:" << external_fn_name);
        return;
    }
}

FunctionImpl::FunctionImpl(FunctionImpl&& o) {
    m_impl = std::move(o.m_impl);
}

FunctionImpl& FunctionImpl::operator = (FunctionImpl&& o) {
    m_impl = std::move(o.m_impl);
    return *this;
}

FunctionImpl::~FunctionImpl() = default;

const std::string& FunctionImpl::name() const {
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->name();
}

LLVM_BUILDER_NS_END
