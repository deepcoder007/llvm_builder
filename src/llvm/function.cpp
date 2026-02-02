//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/function.h"
#include "llvm_builder/analyze.h"
#include "llvm_builder/meta/noncopyable.h"
#include "llvm_builder/ds/fixed_string.h"
#include "util/debug.h"
#include "llvm_builder/module.h"
#include "llvm/context_impl.h"

// TODO{vibhanshu}: remove this iostream after fixing verify
#include <iostream>

LLVM_BUILDER_NS_BEGIN

//
// FnContext
//
FnContext::FnContext() : BaseT{State::ERROR} {
}

FnContext::FnContext(const TypeInfo& type)
    : BaseT{State::VALID}, m_type{type} {
    CODEGEN_FN
    if (m_type.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Context Type invalid");
        M_mark_error();
        return;
    }
    if (not m_type.is_scalar() and not m_type.is_pointer()) {
        M_mark_error();
        CODEGEN_PUSH_ERROR(FUNCTION, "type of function arg should be scalar or pointer:");
        return;
    }
}

FnContext::~FnContext() = default;

bool FnContext::operator == (const FnContext& rhs) const {
    if (has_error() and rhs.has_error()) {
        return true;
    }
    return m_type == rhs.m_type
      and m_raw_arg == rhs.m_raw_arg
      and m_value == rhs.m_value;
}

void FnContext::set_value(llvm::Argument* raw_arg) {
    if (has_error()) {
        return;
    }
    m_raw_arg = raw_arg;
    m_value = ValueInfo{m_type, TagInfo{}, (llvm::Value*)m_raw_arg};
}

auto FnContext::null() -> const FnContext& {
    static FnContext s_null{};
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// Function::Impl
//
class Function::Impl : meta::noncopyable {
    Module m_parent;
    const std::string m_fn_name;
    const bool m_is_external = false;
    const TypeInfo m_return_type;
    FnContext m_context;
    llvm::FunctionType* m_type = nullptr;
    llvm::Function* m_fn = nullptr;
    LinkSymbol m_link_symbol;
    std::vector<CodeSectionImpl> m_section_list;
public:
    explicit Impl(const LinkSymbolName& symbol_name
                  , Module parent
                  , const std::string& fn_name
                  , bool is_external
                  , const TypeInfo& return_type
                  , const FnContext& context)
          : m_parent{parent}
          , m_fn_name{fn_name}
          , m_is_external{is_external}
          , m_return_type{return_type}
          , m_context{context}
          , m_type{M_mk_fn_type(m_return_type, m_context)}
          , m_fn{M_mk_fn(parent)}
          , m_link_symbol{symbol_name, return_type, LinkSymbol::symbol_type::function} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not m_parent.has_error());
        LLVM_BUILDER_ASSERT(not m_fn_name.empty());
        LLVM_BUILDER_ASSERT(not m_return_type.has_error());
        LLVM_BUILDER_ASSERT(not m_context.has_error());
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not m_link_symbol.has_error());
        [[maybe_unused]] bool has_arg = false;
        for (auto& l_arg_value : m_fn->args()) {
            m_link_symbol.add_arg(m_context.type(), "context");
            m_context.set_value(&l_arg_value);
            LLVM_BUILDER_ASSERT(not has_arg);
            has_arg = true;
        }
        LLVM_BUILDER_ASSERT(has_arg);
        object::Counter::singleton().on_new(object::Callback::object_t::FUNCTION, (uint64_t)this, m_fn_name);
    }
    explicit Impl(Module parent, llvm::Function& raw_fn)
      : m_parent{parent}, m_fn_name{raw_fn.getName()},
        m_return_type{TypeInfo::from_raw(raw_fn.getFunctionType()->getReturnType())}, m_type{raw_fn.getFunctionType()} , m_fn{&raw_fn} {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not m_parent.has_error());
        LLVM_BUILDER_ASSERT(not m_fn_name.empty());
        LLVM_BUILDER_ASSERT(not m_return_type.has_error());
        LLVM_BUILDER_ASSERT(is_valid());
        [[maybe_unused]] bool has_arg = false;
        for (auto& l_arg_value : m_fn->args()) {
            const llvm::StringRef l_arg_name_ref = l_arg_value.getName();
            const std::string l_arg_name{l_arg_name_ref.data(), l_arg_name_ref.size()};
            // llvm::Type* l_type = l_arg_value.getParamByValType();
            llvm::Type* l_type = l_arg_value.getType();
            if (l_type == nullptr) {
                m_type = nullptr;
                m_fn = nullptr;
                return;
            }
            const TypeInfo l_type_info = TypeInfo::from_raw(l_type);
            m_context = FnContext{l_type_info};
            m_context.set_value(&l_arg_value);
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
        return m_type != nullptr and m_fn != nullptr and m_return_type.is_valid();
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
    const TypeInfo& return_type() const {
        return m_return_type;
    }
    const FnContext& context() const {
        return m_context;
    }
    ValueInfo call_fn(const Function& parent, const ValueInfo& arg) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(is_valid());
        LLVM_BUILDER_ASSERT(not parent.has_error())
        LLVM_BUILDER_ASSERT(not arg.has_error())
        std::vector<llvm::Value*> l_raw_arg_list;
        if (not m_context.type().check_sync(arg)) {
            CODEGEN_PUSH_ERROR(FUNCTION, "Expected type:" << m_context.type().short_name() << ", found type:" << arg.type().short_name());
            parent.M_mark_error();
            return ValueInfo::null();
        }
        l_raw_arg_list.emplace_back(arg.native_value());
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
        llvm::CallInst* l_inst = CursorContextImpl::builder().CreateCall(l_fn_to_call, l_raw_arg_list);
        return ValueInfo{m_return_type, TagInfo{}, l_inst};
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
    static llvm::FunctionType* M_mk_fn_type(TypeInfo return_type, const FnContext& context) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not return_type.has_error());
        std::vector<llvm::Type*> l_arg_type_vec;
        LLVM_BUILDER_ASSERT(not context.has_error());
        l_arg_type_vec.emplace_back(context.type().native_value());
        llvm::FunctionType* l_fn_type = llvm::FunctionType::get(return_type.native_value(), l_arg_type_vec, false);
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

Function::~Function() = default;

bool Function::is_valid() const {
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_valid();
    } else {
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
        M_mark_error();
        return false;
    }
}

const TypeInfo& Function::return_type() const {
    if (has_error()) {
        return TypeInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->return_type();
    } else {
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
        M_mark_error();
        return TypeInfo::null(); 
    }
}

auto Function::context() const -> const FnContext& {
    if (has_error()) {
        return FnContext::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->context();
    } else {
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
        M_mark_error();
        return FnContext::null();
    }
}

ValueInfo Function::call_fn(const ValueInfo& context) const {
    if (has_error()) {
        return ValueInfo::null();
    }
    if (context.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "trying to call function with invalid context");
        M_mark_error();
        return ValueInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->call_fn(*this, context);
    } else {
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
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

//
// FunctionImpl
//
FunctionImpl::FunctionImpl(Module parent, const std::string& fn_name,
            TypeInfo return_type, const FnContext& context,
            c_construct) {
    CODEGEN_FN
    if (parent.has_error()) {
        return;
    }
    if (fn_name.empty()) {
        return;
    }
    if (return_type.has_error()) {
        return;
    }
    if (context.has_error()) {
        return;
    }
    LinkSymbolName sym_name{fn_name};
    m_impl = std::make_shared<Impl>(sym_name, parent, fn_name, false, return_type, context);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:" << fn_name);
        return;
    }
    parent.register_symbol(m_impl->link_symbol());
}

FunctionImpl::FunctionImpl(const std::string& external_fn_name,
                    TypeInfo return_type, const FnContext& context, c_construct) {
    CODEGEN_FN
    if (external_fn_name.empty()) {
        return;
    }
    if (return_type.has_error()) {
        return;
    }
    if (context.has_error()) {
        return;
    }
    LinkSymbolName sym_name{external_fn_name};
    m_impl = std::make_shared<Impl>(sym_name, Module::null(), sym_name.full_name(), true, return_type, context);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:" << external_fn_name);
        return;
    }
}


FunctionImpl::FunctionImpl(Module parent, llvm::Function& raw_fn, c_construct) {
    CODEGEN_FN
    if (parent.has_error()) {
        return;
    }
    m_impl = std::make_shared<Impl>(parent, raw_fn);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:");
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

LLVM_BUILDER_NS_END
