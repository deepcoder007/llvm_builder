//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_LLVM_BUILDER_FUNCTION_H_
#define LLVM_BUILDER_LLVM_BUILDER_FUNCTION_H_

#include "llvm_builder/defines.h"
#include "llvm_builder/type.h"
#include "llvm_builder/value.h"
#include "llvm_builder/defines.h"

#include <string_view>
#include <vector>
#include <list>
#include <optional>
#include <memory>

namespace llvm {
    class Function;
    class Argument;
    class BasicBlock;
};

LLVM_BUILDER_NS_BEGIN

class Module;
class Function;
class FunctionBuilder;
class IfElseCond;
class CodeSectionContext;
class CodeSectionImpl;
class FunctionImpl;

class FnContext : public _BaseObject<FnContext> {
    using BaseT = _BaseObject<FnContext>;
    friend class ValueInfo;
private:
    TypeInfo m_type;
    llvm::Argument* m_raw_arg = nullptr;
public:
    explicit FnContext();
    explicit FnContext(const TypeInfo& type);
    ~FnContext();
public:
    TypeInfo type() const {
        return m_type;
    }
    bool is_valid() const {
        return m_type.is_valid();
    }
    bool is_init() const {
        return m_raw_arg != nullptr;
    }
    void set_value(llvm::Argument* raw_arg);
    bool operator == (const FnContext& rhs) const;
    static const FnContext& null();
    ValueInfo value() const;
private:
    llvm::Value* M_eval();
};

class CodeSection : public _BaseObject<CodeSection>  {
    using BaseT = _BaseObject<CodeSection>;
public:
    class Impl;
    friend class CodeSectionContext;
private:
    std::weak_ptr<Impl> m_impl;
public:
    explicit CodeSection();
    explicit CodeSection(CodeSectionImpl& impl);
    ~CodeSection();
public:
    const std::string& name() const;
    bool is_valid() const;
    bool is_open() const;
    bool is_commit() const;
    bool is_sealed() const;
    void enter();
    CodeSection detach();
    Function function();
    void jump_to_section(CodeSection dst);
    void conditional_jump(const ValueInfo& value, CodeSection then_dst, CodeSection else_dst);
    ValueInfo intern(const ValueInfo& v);
    void add_eval(const ValueInfo& key, llvm::Value* value);
    llvm::Value* get_eval(const ValueInfo& key) const;
    llvm::BasicBlock* native_handle() const;
    bool operator == (const CodeSection& o) const;
    static CodeSection null();
private:
    void M_set_return_value(ValueInfo value);
    void M_exit();
    void M_re_enter();
};

class CodeSectionContext {
    friend class CodeSection;
public:
    using vec_t = std::vector<CodeSection>;
public:
    explicit CodeSectionContext() = delete;
    ~CodeSectionContext() = delete;
public:
    static void push_var_context();
    static void pop_var_context();
    static ValueInfo pop(std::string_view name);
    static void push(std::string_view name, const ValueInfo& v);
    static ValueInfo mk_ptr(std::string_view name, const TypeInfo& type, const ValueInfo& default_value);
    static Function function();
    static void set_return_value(ValueInfo value);
    static CodeSection mk_section(const std::string& name);
    static void define_section(const std::string& name, Function&, std::function<void()>&& defn);
    static void jump_to_section(CodeSection &dst);
    static void section_break(const std::string& new_section_name);
    static CodeSection current_section();
    static bool is_current_section(CodeSection& code);
    static Function current_function();
    static ValueInfo current_context();
    static size_t clean_sealed_context();
    static void assert_no_context();
    static void print_section_stack(std::ostream& os);
    static void print_variable_stack(std::ostream& os);
private:
    static vec_t &M_sections();
    static std::list<CodeSection>& M_detached();
    // TODO{vibhanshu}: given now sections are shared_ptr based, ensure that section
    //     which are neither in stack nor detached are invalid, maybe have a custom
    //     memory allocator with garbage collector for that
    static void M_push_section(CodeSection& code);
    static void M_pop_section(CodeSection& code);
    static void M_re_enter_top();
    static CodeSection M_detach(CodeSection& code);
};

class Function : public _BaseObject<Function> {
    using BaseT = _BaseObject<Function>;
    struct c_construct {
    };
    friend class FunctionBuilder;
    friend class Module;
    friend class FunctionImpl;
    friend class CodeSectionContext;
    friend class ValueInfo;
public:
    class Impl;
private:
    std::weak_ptr<Impl> m_impl;
public:
    explicit Function();
    explicit Function(FunctionImpl& impl);
    ~Function();
public:
    bool is_valid() const;
    const Module &parent_module() const;
    const std::string &name() const;
    bool is_external() const;
    const TypeInfo& return_type() const;
    ValueInfo call_fn(const ValueInfo& context) const;
    void declare_fn(Module& src_mod, Module& dst_mod);
    CodeSection mk_section(const std::string& name);
    llvm::Function *native_handle() const;
    void verify();
    void remove_from_module();
    void write_to_ostream() const;
    bool operator == (const Function& o) const;
    static Function null();
private:
    const FnContext& context() const;
};

// TODO{vibhanshu}: do we need function builder now given that everything simplified
class FunctionBuilder : public _BaseObject<FunctionBuilder> {
    using BaseT = _BaseObject<FunctionBuilder>;
    class Impl;
    friend class Impl;
private:
    std::shared_ptr<Impl> m_impl;
public:
    explicit FunctionBuilder();
    ~FunctionBuilder();
public:
    bool is_frozen() const;
    FunctionBuilder& set_context(const FnContext& context);
    FunctionBuilder& set_name(const std::string& name);
    FunctionBuilder& set_module(Module& mod);
    FunctionBuilder& mark_external();
    Function compile();
    bool operator == (const FunctionBuilder& rhs) const;
    static FunctionBuilder null();
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_FUNCTION_H_
