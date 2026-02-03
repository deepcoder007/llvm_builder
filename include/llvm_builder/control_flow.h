//
// Created by vibhanshu on 2024-04-13
//


#ifndef LLVM_BUILDER_LLVM_CONTROL_FLOW_
#define LLVM_BUILDER_LLVM_CONTROL_FLOW_

#include "llvm_builder/defines.h"
#include "llvm_builder/defines.h"
#include "type.h"
#include "value.h"
#include "llvm_builder/function.h"

LLVM_BUILDER_NS_BEGIN

// TODO{vibhanshu}: ability to specify preferred branch to avoid 2 jumps in instruction
// TODO{vibhanshu}: test if nested if statements work. i.e. if-else inside if-else inside if-else
//                   for nested if-else, keep track of closest if-else statement
class IfElseCond {
    struct c_construct {
    };
    enum class branch_type : uint8_t {
        invalid,
        br_then,
        br_else
    };
public:
    using br_fn_t = std::function<void()>;
public:
    class BranchSection {
        class Impl;
    private:
        std::unique_ptr<Impl> m_impl;

        BranchSection(const BranchSection&) = delete;
        BranchSection(BranchSection&&) = delete;
        BranchSection& operator=(const BranchSection&) = delete;
        BranchSection& operator=(BranchSection&&) = delete;
    public:
        explicit BranchSection(const std::string& name, branch_type type, Function fn, IfElseCond& parent);
        ~BranchSection();
    public:
        bool is_open() const;
        bool is_sealed() const;
        void enter();
        void exit();
        void define_branch(br_fn_t&& fn);
        CodeSection code_section();
    };
private:
    const std::string m_name;
    ValueInfo m_value;
    CodeSection m_parent;
    BranchSection m_then_branch;
    BranchSection m_else_branch;
    CodeSection m_post_if_branch;
    // TODO{vibhanshu}: replace with fixed_string to avoid malloc calls
    branch_type m_is_inside_branch_type = branch_type::invalid;
    bool m_is_sealed = false;
public:
    // TODO{vibhanshu}: attach ValueInfo with a Section so you don't need to explicity pass section
    explicit IfElseCond(const std::string& name, const ValueInfo& value);
    ~IfElseCond();
public:
    bool is_sealed() const {
        return m_is_sealed;
    }
    void then_branch(br_fn_t&&);
    void else_branch(br_fn_t&&);
    BranchSection& then_branch();
    BranchSection& else_branch();
    void bind();
    void enter_branch(c_construct, branch_type type);
    void exit_branch(c_construct, branch_type type);
};

// TODO{vibhanshu}: implement a switch-case instruction

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_CONTROL_FLOW_
