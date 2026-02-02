
#include "llvm_builder/defines.h"
#include "llvm_builder/type.h"
#include "llvm_builder/value.h"
#include "llvm_builder/analyze.h"
#include "llvm/context_impl.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"
#include "llvm_builder/module.h"
#include "util/debug.h"
#include "util/string_util.h"
#include "llvm_builder/meta/noncopyable.h"
#include "ext_include.h"

LLVM_BUILDER_NS_BEGIN

//
// IfElseCond::BranchSection::Impl
//
class IfElseCond::BranchSection::Impl : public meta::noncopyable {
    const std::string m_name;
    branch_type m_type = branch_type::invalid;
    Function m_fn;
    IfElseCond& m_parent;
    CodeSection m_section;
public:
    explicit Impl(const std::string& name, branch_type type, Function fn, IfElseCond& parent)
        : m_name{name}
        , m_type{type}
        , m_fn{fn}
        , m_parent{parent} {
        LLVM_BUILDER_ASSERT(not name.empty());
        LLVM_BUILDER_ASSERT(not fn.has_error());
        LLVM_BUILDER_ASSERT(m_type == branch_type::br_then or m_type == branch_type::br_else);
        m_section = fn.mk_section(m_name);
    }
    ~Impl() {
        if (m_section.is_open()) {
            LLVM_BUILDER_ASSERT(m_section.is_sealed());
        }
    }
public:
    bool is_open() const {
        return m_section.is_open();
    }
    bool is_sealed() const {
        return m_section.is_sealed();
    }
    void enter() {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(not m_section.is_sealed());
        LLVM_BUILDER_ASSERT(not m_section.is_commit());
        m_section.enter();
        m_parent.enter_branch(c_construct{}, m_type);
    }
    void exit() {
        CODEGEN_FN
        CodeSectionContext::current_section().jump_to_section(m_parent.m_post_if_branch);
        LLVM_BUILDER_ASSERT(m_section.is_sealed());
        LLVM_BUILDER_ASSERT(m_section.is_commit());
        m_parent.exit_branch(c_construct{}, m_type);
    }
    void define_branch(br_fn_t&& fn) {
        CODEGEN_FN
        LLVM_BUILDER_ASSERT(fn);
        enter();
        fn();
        exit();
    }
    CodeSection code_section() {
        return m_section;
    }
};

//
// IfElseCond::BranchSection
//
IfElseCond::BranchSection::BranchSection(const std::string& name, branch_type type, Function fn, IfElseCond& parent) {
    if (ErrorContext::has_error()) {
        return;
    }
    m_impl = std::make_unique<Impl>(name, type, fn, parent);
}

IfElseCond::BranchSection::~BranchSection() = default;

bool IfElseCond::BranchSection::is_open() const {
    if (ErrorContext::has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_open();
}

bool IfElseCond::BranchSection::is_sealed() const {
    if (ErrorContext::has_error()) {
        return false;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->is_sealed();
}

void IfElseCond::BranchSection::enter() {
    if (ErrorContext::has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->enter();
}

void IfElseCond::BranchSection::exit() {
    if (ErrorContext::has_error()) {
        return;
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->exit();
}

void IfElseCond::BranchSection::define_branch(br_fn_t&& fn) {
    if (ErrorContext::has_error()) {
        return;
    }
    if (not fn) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR,  "can't define an empty branch")
    }
    LLVM_BUILDER_ASSERT(m_impl);
    m_impl->define_branch(std::move(fn));
}

auto IfElseCond::BranchSection::code_section() -> CodeSection {
    if (ErrorContext::has_error()) {
        return CodeSection::null();
    }
    LLVM_BUILDER_ASSERT(m_impl);
    return m_impl->code_section();
}

//
// IfElseCond
//
IfElseCond::IfElseCond(const std::string &name, const ValueInfo &value)
    : m_name{name}, m_value{value}, m_parent{CodeSectionContext::current_section()},
      m_then_branch{LLVM_BUILDER_CONCAT << m_name << ".then", branch_type::br_then,
                    CodeSectionContext::current_function(), *this},
      m_else_branch{LLVM_BUILDER_CONCAT << m_name << ".else", branch_type::br_else, CodeSectionContext::current_function(), *this},
      m_post_if_branch{CodeSectionContext::current_function().mk_section(LLVM_BUILDER_CONCAT << name << ".post")} {
    if (m_name.empty()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR,  "branching without branch name")
    }
    if (m_value.has_error()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR,  "branching with invalid value")
    } else if (not m_value.type().is_boolean()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR,  "branching can only be done over boolean type value")
    }
}

IfElseCond::~IfElseCond() {
    // TODO{vibhanshu}: instead jump directly to post for un-specified branch
    if (not m_then_branch.is_sealed() and not m_else_branch.is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "Then/Else branch not found for condition:" << m_name);
    }
    if (not is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElseCondition was never completed:" << m_name);
    }
}

void IfElseCond::then_branch(br_fn_t&& fn) {
    if (ErrorContext::has_error()) {
        return;
    }
    if (is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse already sealed");
        return;
    }
    if (not fn) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse branch not correctly specified");
        return;
    }
    m_then_branch.define_branch(std::move(fn));
}

void IfElseCond::else_branch(br_fn_t&& fn) {
    if (ErrorContext::has_error()) {
        return;
    }
    if (is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse already sealed");
        return;
    }
    if (not fn) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse branch not correctly specified");
        return;
    }
    m_else_branch.define_branch(std::move(fn));
}

auto IfElseCond::then_branch() -> BranchSection& {
    if (is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse already sealed");
    }
    return m_then_branch;
}

auto IfElseCond::else_branch() -> BranchSection& {
    if (is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse already sealed");
    }
    return m_else_branch;
}

void IfElseCond::bind() {
    if (ErrorContext::has_error()) {
        return;
    }
    if (is_sealed()) {
        CODEGEN_PUSH_ERROR(BRANCH_ERROR, "IfElse already sealed");
        return;
    }
    if (m_is_inside_branch_type == branch_type::invalid) {
        LLVM_BUILDER_ASSERT(CodeSectionContext::is_current_section(m_parent));
        CodeSection l_then_section = m_post_if_branch;
        CodeSection l_else_section = m_post_if_branch;
        if (m_then_branch.is_open()) {
            l_then_section = m_then_branch.code_section();
        }
        if (m_else_branch.is_open()) {
            l_else_section = m_else_branch.code_section();
        }
        m_parent.conditional_jump(m_value, l_then_section, l_else_section);
        m_post_if_branch.enter();
        m_post_if_branch.detach();
        m_is_sealed = true;
    } else {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't bind when already inside a section");
    }
}

void IfElseCond::enter_branch(c_construct, branch_type type) {
    if (m_is_inside_branch_type == branch_type::invalid) {
        m_is_inside_branch_type = type;
        if (type == branch_type::br_then) {
            LLVM_BUILDER_ASSERT(m_then_branch.is_open());
            LLVM_BUILDER_ASSERT(not m_then_branch.is_sealed());
        } else {
            LLVM_BUILDER_ASSERT(type == branch_type::br_else);
            LLVM_BUILDER_ASSERT(m_else_branch.is_open());
            LLVM_BUILDER_ASSERT(not m_else_branch.is_sealed());
        }
    } else {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't enter branch while inside another branch");
    }
}

void IfElseCond::exit_branch(c_construct, [[maybe_unused]] branch_type type) {
    if (m_is_inside_branch_type != branch_type::invalid) {
        LLVM_BUILDER_ASSERT(m_is_inside_branch_type == type);
        m_is_inside_branch_type = branch_type::invalid;
    } else {
        CODEGEN_PUSH_ERROR(CODE_SECTION, "can't exit branch without entering branch");
    }
}

LLVM_BUILDER_NS_END
