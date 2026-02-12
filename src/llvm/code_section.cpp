//
// Created by vibhanshu on 2025-11-21
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
    void mk_ptr(const std::string& name, const TypeInfo& type, const ValueInfo& default_value) {
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
    void print(std::ostream& os) const {
        os << "{VariableContext: ptr:[";
        for (const auto& kv : m_ptr) {
            os << ", " << kv.first;
        }
        os << "], values:[";
        for (const auto& kv : m_values) {
            os << ", " << kv.first;
        }
        os << "]";
    }
    OSTREAM_FRIEND(VariableContext)
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
    void mk_ptr(const std::string& name, const TypeInfo& type, const ValueInfo& default_value) {
        LLVM_BUILDER_ASSERT(not name.empty())
        LLVM_BUILDER_ASSERT(not type.has_error())
        LLVM_BUILDER_ASSERT(not default_value.has_error())
        if (m_ctx_head) {
            m_ctx_head->mk_ptr(name, type, default_value);
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
    void print(std::ostream& os) const {
        os << "Variable stack:" << std::endl;
        VariableContext* l_ctx = m_ctx_head.get();
        int32_t stack_pos = 0;
        while (l_ctx != nullptr) {
            os << "     " << ++stack_pos << " " << *l_ctx << std::endl;
            l_ctx = l_ctx->parent().get();
        }
    }
    OSTREAM_FRIEND(VariableContextMgr)
public:
    static VariableContextMgr& singleton() {
        static VariableContextMgr s_mgr{};
        return s_mgr;
    }
};

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
    bool m_is_commit  = false;
    bool m_is_sealed = false;
    std::unordered_set<ValueInfo, ValueHash> m_interned_values;
    std::unordered_map<ValueInfo, llvm::Value*, ValueHash> m_eval_values;
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
    void M_force_seal() {
        m_is_sealed = true;
    }
public:
    const std::string& name() const {
        return m_name;
    }
    bool is_open() const {
        return m_is_open;
    }
    bool is_commit() const {
        return m_is_commit;
    }
    void enter() {
        // TODO{vibhanshu}: do you want this block to be re-entrant
        LLVM_BUILDER_ASSERT(not is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(not is_commit());
        m_cursor_impl.builder().SetInsertPoint(m_basic_block);
        m_is_open = true;
    }
    void exit() {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(is_sealed());
        LLVM_BUILDER_ASSERT(not is_commit());
        m_cursor_impl.builder().restoreIP(m_insert_point);
        m_is_commit = true;
    }
    bool is_sealed() const {
        return m_is_sealed;
    }
    Function& function() {
        return m_fn;
    }
    const Function& function() const {
        return m_fn;
    }
    void set_return_value(ValueInfo value) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(not is_commit());
        LLVM_BUILDER_ASSERT(value.type() == m_fn.return_type());
        LLVM_BUILDER_ASSERT(not value.has_error());
        m_cursor_impl.builder().CreateRet(value.M_eval());
        M_force_seal();
    }
    void jump_to_section(Impl& s) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(not is_commit());
        LLVM_BUILDER_ASSERT(this != &s);
        LLVM_BUILDER_ASSERT(s.m_basic_block != nullptr);
        m_cursor_impl.builder().CreateBr(s.m_basic_block);
        M_force_seal();
    }
    void conditional_jump(ValueInfo value, Impl& then_dst,
                          Impl& else_dst) {
        LLVM_BUILDER_ASSERT(is_open());
        LLVM_BUILDER_ASSERT(not is_sealed());
        LLVM_BUILDER_ASSERT(not is_commit());
        LLVM_BUILDER_ASSERT(not value.has_error());
        m_cursor_impl.builder().CreateCondBr(value.M_eval(),
                                  then_dst.native_handle(),
                                  else_dst.native_handle());
        M_force_seal();
    }
    ValueInfo intern(const ValueInfo& v) {
        auto it = m_interned_values.emplace(v);
        return *it.first;
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

bool CodeSection::is_commit() const {
    CODEGEN_FN
    if (has_error()) {
        return false;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return ptr->is_commit();
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
        if (ptr->is_sealed() or ptr->is_commit()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already closed:" << name());
            M_mark_error();
            return;
        }
        ptr->enter();
        // TODO{vibhnashu}: ensure that in function, only the first section will have argument to allow
        //         later sections to over-ride arguemnt values
        CodeSectionContext::M_push_section(*this);
        VariableContextMgr& var_mgr = VariableContextMgr::singleton();
        Function fn = function();
        if (not has_error()) {
            ValueInfo l_value = CodeSectionContext::current_context();
            TypeInfo l_type_info = l_value.type();
            LLVM_BUILDER_ASSERT(l_type_info.is_pointer() or l_type_info.is_scalar());
            var_mgr.set("context", l_value);
        }
    } else {
        M_mark_error();
        return;
    }
}

void CodeSection::M_exit() {
    CODEGEN_FN
    if (has_error()) {
        return;
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        if (not ptr->is_open()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection is not open");
            M_mark_error();
            return;
        }
        if (not ptr->is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection is not sealed");
            M_mark_error();
            return;
        }
        if (ptr->is_commit()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection is already commited/exited");
            M_mark_error();
            return;
        }
        ptr->exit();
        CodeSectionContext::M_pop_section(*this);
    } else {
        M_mark_error();
        return;
    }
}

auto CodeSection::detach() -> CodeSection {
    CODEGEN_FN
    if (has_error()) {
        return CodeSection::null();
    }
    if (std::shared_ptr<Impl> ptr = m_impl.lock()) {
        return CodeSectionContext::M_detach(*this);
    } else {
        M_mark_error();
        return CodeSection::null();
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
    if (has_error()) {
        return;
    }
    if (value.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection return value can't be set to invalid value");
        M_mark_error();
        return;
    }
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
        TypeInfo fn_type = ptr->function().return_type();
        if (value_type != fn_type) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "return type of value does not match return type of function: found:" << value_type.short_name() << ", expected:" << fn_type.short_name());
            M_mark_error();
            return;
        }
        ptr->set_return_value(value);
        M_exit();
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
        M_exit();
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
        M_exit();
    } else {
        M_mark_error();
        return;
    }
}

ValueInfo CodeSection::intern(const ValueInfo& v) {
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

void CodeSection::add_eval(const ValueInfo& key, llvm::Value* value) {
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

llvm::Value* CodeSection::get_eval(const ValueInfo& key) const {
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

CodeSection CodeSection::null() {
    static CodeSection s_null;
    LLVM_BUILDER_ASSERT(s_null.has_error());
    return s_null;
}

//
// CodeSectionContext
//
auto CodeSectionContext::M_sections() -> vec_t& {
    thread_local static vec_t s_vec;
    return s_vec;
}

auto CodeSectionContext::M_detached() -> std::list<CodeSection>& {
    thread_local static std::list<CodeSection> s_vec;
    return s_vec;
}

void CodeSectionContext::M_push_section(CodeSection& code) {
    CODEGEN_FN
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't push an invalid code section");
        return;
    }
    vec_t& l_vec = M_sections();
    for (CodeSection& l_sec : l_vec) {
        if (l_sec == code) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection already entered, can't re-enter");
            code.M_mark_error();
            return;
        }
        if ((l_sec.function()) != (code.function())) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection function mis-match, something is wrong");
            code.M_mark_error();
            return;
        }
    }
    l_vec.emplace_back(code);
}

void CodeSectionContext::M_pop_section(CodeSection& code) {
    CODEGEN_FN
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't pop an invalid code section");
        return;
    }
    vec_t& l_vec = M_sections();
    if (l_vec.size() == 0) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't pop code-section, none exist: " << code.name());
        return;
    }
    CodeSection& l_code = l_vec.back();
    if (code != l_code) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Section mis-match: " << code.name() << " <-> " << l_code.name());
        return;
    }
    l_vec.pop_back();
}

auto CodeSectionContext::M_detach(CodeSection &code) -> CodeSection {
    CODEGEN_FN
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't detach an invalid code section");
        return CodeSection::null();
    }
    vec_t& l_vec = M_sections();
    CodeSection& l_code = l_vec.back();
    if (code != l_code) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Section mis-match: " << code.name() << " <-> " << l_code.name());
        return CodeSection{};
    }
    l_vec.pop_back();
    CodeSection& l_new_entry = M_detached().emplace_back(std::move(code));
    l_vec.push_back(l_new_entry);
    return l_new_entry;
}

void CodeSectionContext::push_var_context() {
    VariableContextMgr::singleton().push_context();
}

void CodeSectionContext::pop_var_context() {
    VariableContextMgr::singleton().pop_context();
}

ValueInfo CodeSectionContext::pop(std::string_view name) {
    CODEGEN_FN
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't pop/read an empty variable name");
        return ValueInfo::null();
    } else {
        return VariableContextMgr::singleton().try_get_value(cname.str());
    }
}

void CodeSectionContext::push(std::string_view name, const ValueInfo& v) {
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

void CodeSectionContext::mk_ptr(std::string_view name, const TypeInfo& type, const ValueInfo& default_value) {
    CODEGEN_FN
    CString cname{name};
    if (cname.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk an empty variable name");
    } else if (type.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk a pointer with invalid type variable");
    } else if (default_value.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't mk a pointer with invalid default value");
    } else {
        VariableContextMgr::singleton().mk_ptr(cname.str(), type, default_value);
    }
}

Function CodeSectionContext::function() {
    return current_section().function();
}

Function CodeSectionContext::current_function() {
    return current_section().function();
}

void CodeSectionContext::set_return_value(ValueInfo value) {
    CODEGEN_FN
    if (value.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Invalid value can't be pushed to CodeSection");
        return ;
    }
    CodeSection l_section = current_section();
    if (l_section.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection not found");
        return;
    }
    l_section.M_set_return_value(value);
}

void CodeSectionContext::jump_to_section(CodeSection& dst) {
    CODEGEN_FN
    CodeSection l_section = current_section();
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

CodeSection CodeSectionContext::mk_section(const std::string& name) {
    CODEGEN_FN
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection with empty name");
        return CodeSection::null();
    }
    Function fn = current_function();
    if (fn.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "CodeSection/Function not found");
        return CodeSection::null();
    }
    return fn.mk_section(name);
}

void CodeSectionContext::define_section(const std::string& name, Function& fn, std::function<void()>&& defn) {
    CODEGEN_FN
    if (name.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection with empty name");
        return;
    }
    if (fn.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection without valid function");
        return;
    }
    if (not defn) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection without valid definition");
        return;
    }
    CodeSection l_section = fn.mk_section(name);
    l_section.enter();
    l_section.detach();
    defn();
}

void CodeSectionContext::section_break(const std::string &new_section_name) {
    CODEGEN_FN
    if (new_section_name.empty()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't make CodeSection with empty name");
        return;
    }
    CodeSection l_section = current_function().mk_section(new_section_name);
    current_section().jump_to_section(l_section);
    l_section.enter();
    l_section.detach();
}

auto CodeSectionContext::current_section() -> CodeSection {
    CODEGEN_FN
    vec_t& l_vec = M_sections();
    if (l_vec.size() == 0) {
        // TODO{vibhanshu}: not really sure if we should throw an eror here or not
        // CODEGEN_PUSH_ERROR(CODE_SECTION, "No section found");
        return CodeSection::null();
    }
    return l_vec.back();
}

bool CodeSectionContext::is_current_section(CodeSection &code) {
    CODEGEN_FN
    if (code.has_error()) {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "Can't use invalid CodeSection");
        return false;
    }
    vec_t& l_vec = M_sections();
    if (l_vec.size() > 0) {
        return code == l_vec.back();
    } else {
        return false;
    }
}

ValueInfo CodeSectionContext::current_context() {
    return current_function().context().value();
}

size_t CodeSectionContext::clean_sealed_context() {
    CODEGEN_FN
    vec_t& l_vec = M_sections();
    size_t l_count = l_vec.size();
    for (CodeSection& l_section : l_vec) {
        if (not l_section.is_sealed()) {
            CODEGEN_PUSH_ERROR(CODE_SECTION, "Section not sealed:" << l_section.name());
            return l_count;
        }
    }
    l_vec.clear();
    M_detached().clear();
    return l_count;
}

void CodeSectionContext::assert_no_context() {
    vec_t& l_vec = M_sections();
    if (l_vec.size() > 0) {
        LLVM_BUILDER_ABORT("sections open found");
    }
}

void CodeSectionContext::print_section_stack(std::ostream &os) {
    vec_t& l_vec = M_sections();
    os << " number of sections:" << l_vec.size() << std::endl;
    for (const CodeSection &l_sec : l_vec) {
        os << "  > " << l_sec.name() << ":" << l_sec.is_sealed() << ":" << &l_sec << std::endl;
    }
}

void CodeSectionContext::print_variable_stack(std::ostream& os) {
    os << VariableContextMgr::singleton();
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

LLVM_BUILDER_NS_END


