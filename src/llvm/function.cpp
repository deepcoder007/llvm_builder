//
// Created by vibhanshu on 2024-06-07
//

#include "llvm_builder/function.h"
#include "llvm_builder/analyze.h"
#include "meta/noncopyable.h"
#include "ds/fixed_string.h"
#include "util/debug.h"
#include "util/cstring.h"
#include "llvm_builder/module.h"
#include "llvm/context_impl.h"
#include "llvm/ext_include.h"

#include <iostream>

LLVM_BUILDER_NS_BEGIN

//
//  VariableContext
//
class VariableContext : public meta::noncopyable {
    std::unordered_map<std::string, ValueInfo> m_ptr;
    std::unordered_map<std::string, ValueInfo> m_values;
    std::unique_ptr<VariableContext> m_parent;
    bool m_is_active = false;
public:
    explicit VariableContext(std::unique_ptr<VariableContext>& parent)
        : m_parent{std::move(parent)} {
        if (m_parent) {
            LLVM_BUILDER_ASSERT(m_parent->is_active());
            m_parent->disable();
        }
        enable();
    }
    ~VariableContext() {
        LLVM_BUILDER_ASSERT(not m_parent);
    }
public:
    bool is_active() const {
        return m_is_active;
    }
    void enable() {
        m_is_active = true;
    }
    void disable() {
        m_is_active = false;
    }
    std::unique_ptr<VariableContext>& parent() {
        return m_parent;
    }
    ValueInfo mk_ptr(const std::string& name, const TypeInfo& type, const ValueInfo& default_value) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not type.has_error())
        LLVM_BUILDER_ASSERT(not default_value.has_error())
        LLVM_BUILDER_ASSERT(is_active());
        ValueInfo l_ptr = ValueInfo::mk_pointer(type);
        m_ptr[name] = l_ptr;
        if (not default_value.is_null()) {
            l_ptr.store(default_value);
        }
        return l_ptr;
    }
    void set(const std::string& name, const ValueInfo& v) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not v.has_error())
        LLVM_BUILDER_ASSERT(is_active());
        ValueInfo ptr_value = try_get_ptr(name);
        if (ptr_value.is_null()) {
            m_values[name] = v;
        } else {
            ptr_value.store(v);
        }
    }
    ValueInfo try_get_ptr(const std::string& name) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        if (m_ptr.find(name) != m_ptr.end()) {
            return m_ptr.at(name);
        } else if (m_parent) {
            return m_parent->try_get_ptr(name);
        } else {
            return ValueInfo::null();
        }
    }
    ValueInfo try_get_value(const std::string& name) const {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not name.empty())
        ValueInfo r = try_get_ptr(name);
        if (not r.is_null()) {
            return r.load();
        } else {
            auto it = m_values.find(name);
            if (it != m_values.end()) {
                return it->second;
            } else if (m_parent) {
                return m_parent->try_get_value(name);
            } else {
                CODEGEN_PUSH_ERROR(VALUE_ERROR, "value not found:" << name);
                return ValueInfo::null();
            }
        }
    }
};

class VariableContextMgr : meta::noncopyable {
    std::unique_ptr<VariableContext> m_ctx_head;
    uint32_t m_num_ctx = 0;
private:
    explicit VariableContextMgr() {
        push_context();
    }
public:
    ~VariableContextMgr() = default;
public:
    bool has_context() const {
        return static_cast<bool>(m_ctx_head);
    }
    void push_context() {
        std::unique_ptr<VariableContext> l_new = std::make_unique<VariableContext>(m_ctx_head);
        m_ctx_head = std::move(l_new);
        m_ctx_head->enable();
        ++m_num_ctx;
    }
    void pop_context() {
        if (m_num_ctx > 0) {
            LLVM_BUILDER_ASSERT(m_ctx_head);
            std::unique_ptr<VariableContext> l_parent = std::move(m_ctx_head->parent());
            m_ctx_head = std::move(l_parent);
            --m_num_ctx;
            if (m_ctx_head) {
                m_ctx_head->enable();
            }
        } else {
            LLVM_BUILDER_ASSERT(not m_ctx_head);
        }
    }
    ValueInfo mk_ptr(const std::string& name, const TypeInfo& type, const ValueInfo& default_value) {
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not type.has_error())
        LLVM_BUILDER_ASSERT(not default_value.has_error())
        if (m_ctx_head) {
            return m_ctx_head->mk_ptr(name, type, default_value);
        } else {
            return ValueInfo::null();
        }
    }
    void set(const std::string& name, const ValueInfo& v) {
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not v.has_error())
        if (m_ctx_head) {
            m_ctx_head->set(name, v);
        }
    }
    ValueInfo try_get_ptr(const std::string& name) const {
        LLVM_BUILDER_ASSERT(not name.empty())
        if (m_ctx_head) {
            return m_ctx_head->try_get_ptr(name);
        } else {
            return ValueInfo::null();
        }
    }
    ValueInfo try_get_value(const std::string& name) const {
        LLVM_BUILDER_ASSERT(not name.empty())
        if (m_ctx_head) {
            return m_ctx_head->try_get_value(name);
        } else {
            return ValueInfo::null();
        }
    }
public:
    static VariableContextMgr& singleton() {
        static VariableContextMgr s_mgr{};
        return s_mgr;
    }
};

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
    std::vector<CodeSection> m_section_stack;
public:
    explicit Impl(const LinkSymbolName& symbol_name
                  , const std::string& fn_name
                  , bool is_external)
          : m_parent{Module::Context::value()}
          , m_fn_name{fn_name}
          , m_is_external{is_external}
          , m_type{M_mk_fn_type()}
          , m_fn{M_mk_fn(m_parent)}
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
                declare_fn(l_current_module);
                l_existing = l_raw_module->getFunction(m_fn->getName());
            }
            l_fn_to_call = l_existing;
        }
        return ValueInfo{l_fn_to_call, typename ValueInfo::construct_fn_t{}};
    }
    void declare_fn(const Module& dst_mod) const {
        LLVM_BUILDER_ASSERT(is_valid());
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
    CodeSection mk_section(const std::string& name, const Function& fn) {
        LLVM_BUILDER_ASSERT(not name.empty());
        LLVM_BUILDER_ASSERT(not fn.has_error());
        LLVM_BUILDER_ASSERT(fn.name() == m_fn_name);
        for (CodeSectionImpl& section : m_section_list) {
            if (section.name() == name) {
                CODEGEN_PUSH_ERROR(CODE_SECTION, "Duplicate code-section name:" << name);
                return CodeSection::null();
            }
        }
        CodeSectionImpl& l_entry = m_section_list.emplace_back(name, fn);
        return CodeSection{l_entry};
    }
    void M_push_section(CodeSection& code) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not code.has_error())
        for (CodeSection& l_sec : m_section_stack) {
            if (l_sec == code) {
                CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already entered, can't re-enter");
                code.M_mark_error();
                return;
            }
            if ((l_sec.function()) != (code.function())) {
                CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection function mis-match, something is wrong, probably previous section of previous function was not closed");
                code.M_mark_error();
                return;
            }
        }
        m_section_stack.emplace_back(code);
    }
    void M_pop_section(CodeSection& code) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not code.has_error());
        LLVM_BUILDER_ASSERT(m_section_stack.size() != 0);
        LLVM_BUILDER_ASSERT(code == m_section_stack.back());
        m_section_stack.pop_back();
        if (m_section_stack.size() != 0) {
            CodeSection& l_code = m_section_stack.back();
            l_code.M_re_enter();
        }
    }
    CodeSection current_section() {
        CODEGEN_FN
        if (m_section_stack.size() == 0) {
            // TODO{vibhanshu}: not really sure if we should throw an eror here or not
            // CODEGEN_PUSH_ERROR(CODE_SECTION, "No section found");
            return CodeSection::null();
        }
        return m_section_stack.back();
    }
    bool is_current_section(CodeSection &code) {
        CODEGEN_FN
        if (code.has_error()) {
            return false;
        }
        if (m_section_stack.size() > 0) {
            return code == m_section_stack.back();
        } else {
            return false;
        }
    }
    void assert_no_context() {
        if (m_section_stack.size() > 0) {
            LLVM_BUILDER_ABORT("sections open found");
        }
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
    llvm::Function* M_mk_fn(Module dest_mod) const {
        CODEGEN_FN
        if (is_external()) {
            dest_mod = Module::Context::value();
        }
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

Function::Function(const std::string& name, bool is_external)
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
    if (not Module::Context::has_value()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "no active module found");
        M_mark_error();
        return;
    }
    FunctionImpl impl(name, is_external, c_construct{});
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

Module Function::parent_module() const {
    CODEGEN_FN
    if (has_error()) {
        return Module::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->parent_module();
    } else {
        M_mark_error();
        return Module::null();
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
    CODEGEN_FN
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

void Function::declare_fn(Module& dst_mod) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (dst_mod.has_error()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "trying to declare function with invalid dst module");
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->declare_fn(dst_mod);
    } else {
        M_mark_error();
    }
}

CodeSection Function::mk_section(const std::string& name) {
    CODEGEN_FN
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

CodeSection Function::current_section() {
    if (has_error()) {
        return CodeSection::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->current_section();
    } else {
        M_mark_error();
        return CodeSection::null();
    }
}

bool Function::is_current_section(CodeSection& code) {
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_current_section(code);
    } else {
        M_mark_error();
        return false;
    }
}

void Function::assert_no_context() {
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->assert_no_context();
    } else {
        M_mark_error();
        return;
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

auto Function::null(const std::string& log) -> Function {
    static Function s_func{};
    LLVM_BUILDER_ASSERT(s_func.has_error());
    Function result = s_func;
    result.M_mark_error(log);
    return result;
}

llvm::Value* Function::M_eval_arg() const {
    if (has_error()) {
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->M_eval_arg();
    } else {
        M_mark_error();
        return nullptr;
    }
}

void Function::M_push_section(CodeSection& code) {
    if (has_error()) {
        return;
    }
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't push an invalid code section");
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->M_push_section(code);
    } else {
        M_mark_error();
    }
}

void Function::M_pop_section(CodeSection& code) {
    if (has_error()) {
        return;
    }  
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't push an invalid code section");
        return;
    }
    if (not is_current_section(code)) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Section not the top section" << code.name());
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->M_pop_section(code);
    } else {
        M_mark_error();
    }
}

struct ValueHash {
    size_t operator() (const ValueInfo& /*o*/) const {
        // TODO{vibhanshu}: fix the hash function to avoid collision
        //     in hasmap buckets, currently it leads to linear search
        return 1;
    }
};

//
// CodeSection::Impl
//
class CodeSection::Impl : public meta::noncopyable {
    using insert_point_t = typename llvm::IRBuilderBase::InsertPoint;
    using basic_block_t = typename llvm::BasicBlock;
private:
    const std::string m_name;
    CursorPtr m_cursor_impl;
    Function m_fn;
    insert_point_t m_insert_point;
    basic_block_t* const m_basic_block;
    bool m_is_open = false;
    bool m_is_sealed = false;
    std::unordered_set<ValueInfo, ValueHash> m_interned_values;
    std::vector<ValueInfo> m_sink_values;
    std::unordered_map<ValueInfo, llvm::Value*, ValueHash> m_eval_values;
    std::unordered_map<ValueInfo, std::vector<ValueInfo>, ValueHash> m_store_loads;
public:
    explicit Impl(const std::string& name, const Function& fn)
        : m_name{name}, m_cursor_impl{Cursor::Context::value()}, m_fn{fn}
        , m_insert_point{m_cursor_impl.builder().saveIP()}
        , m_basic_block{llvm::BasicBlock::Create(m_cursor_impl.ctx(), m_name, m_fn.native_handle())} {
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not fn.has_error())
        object::Counter::singleton().on_new(object::Callback::object_t::CODE_SECTION, (uint64_t)this, m_name);
    }
    ~Impl() {
        object::Counter::singleton().on_delete(object::Callback::object_t::CODE_SECTION, (uint64_t)this, m_name);
    }
private:
    static void M_collect_loads(const ValueInfo& root,
                                std::vector<ValueInfo>& out,
                                std::unordered_set<ValueInfo, ValueHash>& visited) {
        if (not visited.insert(root).second) {
            return;
        }
        if (root.value_type() == ValueInfo::value_type_t::load) {
            out.emplace_back(root);
        }
        for (const ValueInfo& p : root.M_parents()) {
            M_collect_loads(p, out, visited);
        }
    }
    void M_build_sink_loads() {
        for (const ValueInfo& sink : m_sink_values) {
            std::vector<ValueInfo> loads;
            std::unordered_set<ValueInfo, ValueHash> visited;
            M_collect_loads(sink, loads, visited);
            m_store_loads.emplace(sink, std::move(loads));
        }
    }
    void M_force_seal() {
        m_is_sealed = true;
        M_build_sink_loads();
        for (auto &kv : m_store_loads) {
            for (ValueInfo& l_load_value : kv.second) {
                l_load_value.M_eval();
            }
        }
        for (ValueInfo& v : m_sink_values) {
            v.M_eval();
        }
    }
public:
    const std::string& name() const {
        return m_name;
    }
    bool is_open() const {
        return m_is_open;
    }
    void enter() {
        // TODO{vibhanshu}: do you want this block to be re-entrant
        LLVM_BUILDER_ASSERT(not is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        m_cursor_impl.builder().SetInsertPoint(m_basic_block);
        m_is_open = true;
    }
    void re_enter() {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        m_cursor_impl.builder().SetInsertPoint(m_basic_block);
    }
    bool is_sealed() const {
        return m_is_sealed;
    }
    Function function() const {
        return m_fn;
    }
    void set_return_value(ValueInfo value) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(value.type() == TypeInfo::mk_int32());
        LLVM_BUILDER_ASSERT(not value.has_error());
        m_sink_values.emplace_back(value);
        M_force_seal();
        m_cursor_impl.builder().CreateRet(value.M_eval());
        m_cursor_impl.builder().restoreIP(m_insert_point);
    }
    void jump_to_section(Impl& s) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(this != &s);
        LLVM_BUILDER_ASSERT(s.m_basic_block != nullptr);
        M_force_seal();
        m_cursor_impl.builder().CreateBr(s.m_basic_block);
        m_cursor_impl.builder().restoreIP(m_insert_point);
    }
    void conditional_jump(ValueInfo value, Impl& then_dst,
                          Impl& else_dst) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(not value.has_error());
        M_force_seal();
        m_cursor_impl.builder().CreateCondBr(value.M_eval(),
                                  then_dst.native_handle(),
                                  else_dst.native_handle());
        m_cursor_impl.builder().restoreIP(m_insert_point);
    }
    ValueInfo intern(const ValueInfo& v) {
        auto it = m_interned_values.emplace(v);
        ValueInfo r = *it.first;
        if (r.M_is_value_sink()) {
            m_sink_values.emplace_back(r);
        }
        return r;
    }
    void add_eval(const ValueInfo& key, llvm::Value* value) {
        if (value != nullptr) {
            m_eval_values.try_emplace(key, value);
        }
    }
    llvm::Value* get_eval(const ValueInfo& key) const {
        auto it = m_eval_values.find(key);
        if (it != m_eval_values.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }
    basic_block_t* native_handle() const {
        return m_basic_block;
    }
};

//
// CodeSection
//
CodeSection::CodeSection() : BaseT{State::ERROR} {
}

CodeSection::CodeSection(CodeSectionImpl& impl)
  : BaseT{State::VALID} {
    CODEGEN_FN
    if (impl.is_valid()) {
        m_impl = impl.ptr();
    } else {
        M_mark_error();
    }
}

CodeSection::~CodeSection() = default;

auto CodeSection::name() const -> const std::string& {
    static std::string s_name{};
    if (has_error()) {
        return s_name;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->name();
    } else {
        CODEGEN_PUSH_ERROR(FUNCTION, "function already deleted");
        M_mark_error();
        return s_name;
    }
}

bool CodeSection::is_valid() const {
    if (has_error()) {
        return false;
    }
    return not m_impl.expired();
}

bool CodeSection::is_open() const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_open();
    } else {
        M_mark_error();
        return false;
    }
}

void CodeSection::enter() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_open()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection can't be open again:" << name());
            M_mark_error();
            return;
        }
        if (ptr->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already closed:" << name());
            M_mark_error();
            return;
        }
        ptr->enter();
        ptr->function().M_push_section(*this);
    } else {
        M_mark_error();
        return;
    }
}

void CodeSection::M_re_enter() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (ptr->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already closed:" << name());
            M_mark_error();
            return;
        }
        if (not ptr->function().is_current_section(*this)) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not the current section of function:" << name());
            M_mark_error();
            return;
        }
        ptr->re_enter();
    } else {
        M_mark_error();
        return;
    }
}

bool CodeSection::is_sealed() const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_sealed();
    } else {
        M_mark_error();
        return false;
    }
}

Function CodeSection::function() {
    CODEGEN_FN
    if (has_error()) {
        return Function::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->function();
    } else {
        M_mark_error();
        return Function::null();
    }
}

void CodeSection::M_set_return_value(ValueInfo value) {
    CODEGEN_FN
    LLVM_BUILDER_ASSERT(not has_error());
    LLVM_BUILDER_ASSERT(not value.has_error());
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (not ptr->is_open()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not open:" << ptr->name());
            M_mark_error();
            return;
        }
        if (ptr->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already sealed:" << ptr->name());
            M_mark_error();
            return;
        }
        TypeInfo value_type = value.type();
        if (value_type != TypeInfo::mk_int32()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "return type of value does not match return type of function: found:" << value_type.short_name() << ", expected:int32");
            M_mark_error();
            return;
        }
        ptr->set_return_value(value);
        ptr->function().M_pop_section(*this);
    } else {
        M_mark_error();
        return;
    }
}

void CodeSection::jump_to_section(CodeSection dst) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (dst.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't jump to invalid code section");
        M_mark_error();
        return;
    }
    std::shared_ptr<Impl> ptr1 = m_impl.lock();
    std::shared_ptr<Impl> ptr2 = dst.m_impl.lock();
    if (ptr1 and ptr2) {
        if (not ptr1->is_open()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not open:" << ptr1->name());
            M_mark_error();
            return;
        }
        if (ptr1->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already sealed:" << ptr1->name());
            M_mark_error();
            return;
        }
        ptr1->jump_to_section(*ptr2);
        ptr1->function().M_pop_section(*this);
    } else {
        M_mark_error();
        return;
    }
}

void CodeSection::conditional_jump(const ValueInfo &value,
                                   CodeSection then_dst,
                                   CodeSection else_dst) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (value.has_error() or then_dst.has_error() or else_dst.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't jump based on invalid condition or branches value");
        M_mark_error();
        return;
    }
    std::shared_ptr<Impl> ptr1 = m_impl.lock();
    std::shared_ptr<Impl> ptr2 = then_dst.m_impl.lock();
    std::shared_ptr<Impl> ptr3 = else_dst.m_impl.lock();
    if (ptr1 and ptr2 and ptr3) {
        if (not ptr1->is_open()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not open:" << ptr1->name());
            M_mark_error();
            return;
        }
        if (ptr1->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already sealed:" << ptr1->name());
            M_mark_error();
            return;
        }
        ptr1->conditional_jump(value, *ptr2, *ptr3);
        ptr1->function().M_pop_section(*this);
    } else {
        M_mark_error();
        return;
    }
}

ValueInfo CodeSection::M_intern(const ValueInfo& v) {
    CODEGEN_FN
    if (has_error()) {
        return ValueInfo::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->intern(v);
    } else {
        M_mark_error();
        return ValueInfo::null();
    }
}

void CodeSection::M_add_eval(const ValueInfo& key, llvm::Value* value) {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        ptr->add_eval(key, value);
    } else {
        M_mark_error();
        return;
    }
}

llvm::Value* CodeSection::M_get_eval(const ValueInfo& key) const {
    CODEGEN_FN
    if (has_error()) {
        return nullptr;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->get_eval(key);
    } else {
        M_mark_error();
        return nullptr;
    }
}

llvm::BasicBlock* CodeSection::native_handle() const {
    CODEGEN_FN
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

bool CodeSection::operator == (const CodeSection& o) const {
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

CodeSection CodeSection::null(const std::string& log) {
    static CodeSection s_null;
    LLVM_BUILDER_ASSERT(s_null.has_error());
    CodeSection result = s_null;
    result.M_mark_error(log);
    return result;
}

//
// FunctionContext
//
Function FunctionContext::s_current_fn{};

FunctionContext::FunctionContext(const Function& fn)
  : m_prev_fn{s_current_fn} {
    set_fn(fn);
}

FunctionContext::~FunctionContext() {
    set_fn(m_prev_fn);
}

bool FunctionContext::set_fn(const Function& fn) {
    s_current_fn = fn;
    bool is_valid = not s_current_fn.has_error();
    if (is_valid) {
        CodeSection l_section = s_current_fn.current_section();
        l_section.M_re_enter();
    }
    return is_valid;
}

void FunctionContext::clear_fn() {
    s_current_fn = Function{};
}

void FunctionContext::push_var_context() {
    VariableContextMgr::singleton().push_context();
}

void FunctionContext::pop_var_context() {
    VariableContextMgr::singleton().pop_context();
}

ValueInfo FunctionContext::pop(std::string_view name) {
    CODEGEN_FN
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't pop/read an empty variable name");
        return ValueInfo::null();
    } else {
        return VariableContextMgr::singleton().try_get_value(cname.str());
    }
}

void FunctionContext::push(std::string_view name, const ValueInfo& v) {
    CODEGEN_FN
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't push/store an empty variable name");
    } else if (v.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't push/store an invalid value");
    } else {
        VariableContextMgr::singleton().set(cname.str(), v);
    }
}

ValueInfo FunctionContext::mk_ptr(std::string_view name, const TypeInfo& type, const ValueInfo& default_value) {
    CODEGEN_FN
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk an empty variable name");
    } else if (type.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk a pointer with invalid type variable");
    } else if (default_value.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk a pointer with invalid default value");
    } else {
        return VariableContextMgr::singleton().mk_ptr(cname.str(), type, default_value);
    }
    return ValueInfo::null();
}

void FunctionContext::set_return_value(ValueInfo value) {
    CODEGEN_FN
    if (value.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Invalid value can't be pushed to CodeSection");
        return ;
    }
    CodeSection l_section = function().current_section();
    if (l_section.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not found");
        return;
    }
    l_section.M_set_return_value(value);
}

void FunctionContext::jump_to_section(CodeSection& dst) {
    CODEGEN_FN
    CodeSection l_section = function().current_section();
    if (l_section.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not found");
        return;
    }
    if (dst.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't jump to invalid section");
        return;
    }
    l_section.jump_to_section(dst);
}



void FunctionContext::section_break(const std::string &new_section_name) {
    CODEGEN_FN
    if (new_section_name.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection with empty name");
        return;
    }
    CodeSection l_section = function().mk_section(new_section_name);
    function().current_section().jump_to_section(l_section);
    l_section.enter();
}

//
// CodeSectionImpl
//
CodeSectionImpl::CodeSectionImpl(const std::string& name, Function fn) {
    CODEGEN_FN
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Section name can't be empty");
        return;
    }
    if (fn.has_error()) {
        return;
    }
    if (fn.is_external()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "External functions can't have a section defined");
        return;
    }
    if (not CursorContextImpl::has_value()) {
        CODEGEN_PUSH_ERROR(CONTEXT, "No context found");
        return;
    }
    m_impl = std::make_shared<Impl>(name, fn);
}

CodeSectionImpl::~CodeSectionImpl() = default;

const std::string& CodeSectionImpl::name() const {
    if (m_impl) {
        return m_impl->name();
    } else {
        return StringManager::null();
    }
}

//
// FunctionImpl
//
FunctionImpl::FunctionImpl(const std::string& fn_name, bool is_external, c_construct) {
    CODEGEN_FN
    if (fn_name.empty()) {
        return;
    }
    LinkSymbolName sym_name{fn_name};
    m_impl = std::make_shared<Impl>(sym_name, fn_name, false);
    if (not is_valid()) {
        CODEGEN_PUSH_ERROR(FUNCTION, "Function can't be defined correctly:" << fn_name);
        return;
    }
    if (not is_external) {
        m_impl->mk_section("fn_begin_section", Function{*this}).enter();
        LLVM_BUILDER_ASSERT(Module::Context::has_value())
        Module::Context::value().register_symbol(m_impl->link_symbol());
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
