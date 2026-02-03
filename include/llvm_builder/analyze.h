//
// Created by vibhanshu on 2025-04-20
//

#ifndef LLVM_BUILDER_LLVM_ANALYZE_H_
#define LLVM_BUILDER_LLVM_ANALYZE_H_

#include "llvm_builder/defines.h"
#include "module.h"

#include <ostream>
#include <string_view>
#include <vector>

namespace llvm {
    class Instruction;
    class BasicBlock;
};

LLVM_BUILDER_NS_BEGIN

namespace object {

class Callback {
public:
    enum class object_t : uint32_t {
        CURSOR,
        MODULE,
        PACKAGED_MODULE,
        LINK_SYMBOL,
        FUNCTION,
        CODE_SECTION,
        TYPE,
        VALUE
    };
public:
    Callback() = default;
    virtual ~Callback() = default;
public:
    void on_new(object_t type, uint64_t id, const std::string& name) {
        M_on_new(type, id, name);
    }
    void on_delete(object_t type, uint64_t id, const std::string& name) {
        M_on_delete(type, id, name);
    }
protected:
    virtual void M_on_new(object_t type, uint64_t id, const std::string& name) = 0;
    virtual void M_on_delete(object_t type, uint64_t id, const std::string& name) = 0;
public:
    bool operator == (const Callback& o) const {
        return this == &o;
    }
};

class Counter {
    using object_t = typename Callback::object_t;
private:
    std::vector<std::unique_ptr<Callback>> m_cb;
private:
    explicit Counter() = default;
    ~Counter() = default;
public:
    void add_callback(std::unique_ptr<Callback>&& cb);
public:
    void on_new(object_t type, uint64_t id, const std::string& name);
    void on_delete(object_t type, uint64_t id, const std::string& name);
public:
    static Counter& singleton();
};
  
}

namespace analysis {

class ModuleAnalysis;

// NOTE{vibhanshu}: only operations in this list
//                  are supported
enum class OpType : uint16_t {
    UNDEFINED,
    FN_CALL,
    NEG,
    ADD,
    SUB,
    MULT,
    DIV,
    REMAINDER,
    RETURN,
    BRANCH,
    LSHIFT,
    RSHIFT,
    AND,
    OR,
    XOR,
    CMP,
    ALLOC,
    LOAD,
    STORE,
    EXTRACT_VEC,
    INSERT_VEC,
    GEP,
    CAST
};

class Inst {
    friend class CodeBlock;
private:
    const llvm::Instruction& m_inst;
    OpType m_type = OpType::UNDEFINED;
private:
    explicit Inst(const llvm::Instruction &inst);
public:
    ~Inst() = default;
public:
    std::string_view name() const;
    std::string op_code_name() const;
    uint32_t raw_op_code() const;
    OpType op_code() const {
        return m_type;
    }
    uint32_t num_operands() const;
    uint32_t num_successor() const;
    bool is_terminator() const;
    bool is_unary() const;
    bool is_binary() const;
    bool is_shift() const;
    bool is_cast() const;
    bool is_only_user() const;
    bool is_atomic() const;
    bool is_volatile() const;
    bool is_throw() const;
    bool have_side_effects() const;
    bool safe_to_remove() const;
    bool will_return() const;
    bool is_debug_or_psudo() const;
    bool may_write_memory() const;
    bool may_read_memory() const;
    bool has_prev() const;
    bool has_next() const;
    std::unique_ptr<Inst> prev() const;
    std::unique_ptr<Inst> next() const;
    void analyze(std::ostream& os, const std::string& offset) const;
};

class CodeBlock {
    friend class FunctionAnalysis;
private:
    const llvm::BasicBlock& m_block;
    std::vector<Inst> m_insts;
private:
  explicit CodeBlock(const llvm::BasicBlock &block);
public:
    ~CodeBlock() = default;
public:
    void analyze(std::ostream& os, const std::string& offset) const;
    void for_each_inst(std::function<void(const Inst&)>&& fn) const {
        for (const Inst &l_inst : m_insts) {
            fn(l_inst);
        }
    }
};

class FunctionAnalysis {
    friend class ModuleAnalysis;
private:
    const llvm::Function& m_func;
    std::vector<CodeBlock> m_code_blocks;
private:
    explicit FunctionAnalysis(const llvm::Function& func);
public:
    ~FunctionAnalysis() = default;
public:
    int32_t num_inst() const;
    void analyze(std::ostream& os, const std::string& offset) const;
    const std::vector<CodeBlock> &code_blocks() const {
        return m_code_blocks;
    }
    void for_each_inst(std::ostream& os, std::function<void(const Inst&)>&& fn) const;
};

class ModuleAnalysis {
    const Module& m_module;
    std::vector<FunctionAnalysis> m_funcs;

    ModuleAnalysis(const ModuleAnalysis&) = delete;
    ModuleAnalysis(ModuleAnalysis&&) = delete;
    ModuleAnalysis& operator=(const ModuleAnalysis&) = delete;
    ModuleAnalysis& operator=(ModuleAnalysis&&) = delete;
public:
    explicit ModuleAnalysis(const Module& module);
    ~ModuleAnalysis();
public:
    void analyze(std::ostream& os) const;
    const std::vector<FunctionAnalysis>& functions() const {
        return m_funcs;
    }
    void for_each_inst(std::ostream& os, std::function<void(const Inst&)>&& fn) const;
};

}

LLVM_BUILDER_NS_END

#endif // LLVM_BUILDER_LLVM_ANALYZE_H_
