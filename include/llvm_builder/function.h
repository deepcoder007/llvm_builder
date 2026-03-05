//
// Created by vibhanshu on 2024-06-07
//

#ifndef LLVM_BUILDER_LLVM_BUILDER_FUNCTION_H_
#define LLVM_BUILDER_LLVM_BUILDER_FUNCTION_H_

#include "defines.h"
#include "type.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>

namespace llvm {
    class Function;
    class Argument;
    class BasicBlock;
};

LLVM_BUILDER_NS_BEGIN

class ValueInfo;
class Module;
class Function;
class IfElseCond;
class FunctionContext;
class CodeSectionImpl;
class FunctionImpl;

class CodeSection : public _BaseObject {
    using BaseT = _BaseObject;
public:
    class Impl;
    friend class FunctionContext;
    friend class Function;
    friend class ValueInfo;
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
    bool is_sealed() const;
    void enter();
    Function function();
    void jump_to_section(CodeSection dst);
    void conditional_jump(const ValueInfo& value, CodeSection then_dst, CodeSection else_dst);
    llvm::BasicBlock* native_handle() const;
    bool operator == (const CodeSection& o) const;
    static CodeSection null(const std::string& log = "");
private:
    void M_set_return_value(ValueInfo value);
    void M_re_enter();
    ValueInfo M_intern(const ValueInfo& v);
    void M_add_eval(const ValueInfo& key, llvm::Value* value);
    llvm::Value* M_get_eval(const ValueInfo& key) const;
};

class Function : public _BaseObject {
    using BaseT = _BaseObject;
    struct c_construct {
    };
    friend class Cursor;
    friend class Module;
    friend class FunctionImpl;
    friend class FunctionContext;
    friend class ValueInfo;
    friend class IfElseCond;
    friend class CodeSection;
public:
    class Impl;
private:
    std::weak_ptr<Impl> m_impl;
public:
    explicit Function();
    explicit Function(FunctionImpl& impl);
    explicit Function(const std::string& name, bool is_external = false);
    ~Function();
public:
    bool is_valid() const;
    Module parent_module() const;
    const std::string& name() const;
    bool is_external() const;
    ValueInfo call_fn() const;
    void declare_fn(Module& dst_mod);
    CodeSection current_section();
    bool is_current_section(CodeSection& code);
    void assert_no_context();
    llvm::Function *native_handle() const;
    void verify();
    void remove_from_module();
    void write_to_ostream() const;
    bool operator == (const Function& o) const;
    static Function null(const std::string& log = "");
private:
    CodeSection mk_section(const std::string& name);
    llvm::Value* M_eval_arg() const;
    void M_push_section(CodeSection& code);
    void M_pop_section(CodeSection& code);
};

class FunctionContext {
    Function m_prev_fn;
    static Function s_current_fn;
public:
    explicit FunctionContext(const Function& fn);
    ~FunctionContext();
public:
    static bool set_fn(const Function& fn);
    static void clear_fn();
    static void push_var_context();
    static void pop_var_context();
    static ValueInfo pop(std::string_view name);
    static void push(std::string_view name, const ValueInfo& v);
    static ValueInfo mk_ptr(std::string_view name, const TypeInfo& type, const ValueInfo& default_value);
    static void set_return_value(ValueInfo value);
    static void jump_to_section(CodeSection &dst);
    static void section_break(const std::string& new_section_name);
    static Function function() {
        return s_current_fn;
    }
};

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_FUNCTION_H_
