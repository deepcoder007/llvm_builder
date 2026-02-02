//
// Created by vibhanshu on 2024-06-07
//

#include "core/llvm/analyze.h"
#include "util/debug.h"
#include "util/string_util.h"
#include "core/util/defines.h"
#include "util/logger.h"
#include "ext_include.h"

#include <cstdio>
#include <iomanip>

LLVM_NS_BEGIN

namespace object {

void Counter::add_callback(std::unique_ptr<Callback>&& cb) {
    m_cb.emplace_back(std::move(cb));
}

void Counter::on_new(object_t type, uint64_t id, const std::string& name) {
    for (std::unique_ptr<Callback>& cb : m_cb) {
        cb->on_new(type, id, name);
    }
}

void Counter::on_delete(object_t type, uint64_t id, const std::string& name) {
    for (std::unique_ptr<Callback>& cb : m_cb) {
        cb->on_delete(type, id, name);
    }
}

Counter& Counter::singleton() {
    static Counter s_object{};
    return s_object;
}

}

namespace analysis {

//
// Inst
//
Inst::Inst(const llvm::Instruction &inst) : m_inst{inst} {
    const uint32_t l_code = m_inst.getOpcode();
    switch(l_code) {
    case llvm::Instruction::FNeg:    m_type = OpType::NEG; break;
    case llvm::Instruction::Add:     m_type = OpType::ADD; break;
    case llvm::Instruction::FAdd:    m_type = OpType::ADD; break;
    case llvm::Instruction::Sub:     m_type = OpType::SUB; break;
    case llvm::Instruction::FSub:    m_type = OpType::SUB; break;
    case llvm::Instruction::Mul:     m_type = OpType::MULT; break;
    case llvm::Instruction::FMul:    m_type = OpType::MULT; break;
    case llvm::Instruction::UDiv:    m_type = OpType::DIV; break;
    case llvm::Instruction::SDiv:    m_type = OpType::DIV; break;
    case llvm::Instruction::FDiv:    m_type = OpType::DIV; break;
    case llvm::Instruction::URem:    m_type = OpType::REMAINDER; break;
    case llvm::Instruction::SRem:    m_type = OpType::REMAINDER; break;
    case llvm::Instruction::FRem:    m_type = OpType::REMAINDER; break;
    case llvm::Instruction::Shl:     m_type = OpType::LSHIFT; break;
    case llvm::Instruction::LShr:    m_type = OpType::RSHIFT; break;
    case llvm::Instruction::AShr:    m_type = OpType::RSHIFT; break;
    case llvm::Instruction::And:     m_type = OpType::AND; break;
    case llvm::Instruction::Or:      m_type = OpType::OR; break;
    case llvm::Instruction::Xor:     m_type = OpType::XOR; break;
    case llvm::Instruction::Ret:     m_type = OpType::RETURN; break;
    case llvm::Instruction::Br:      m_type = OpType::BRANCH; break;
    case llvm::Instruction::ICmp:    m_type = OpType::CMP; break;
    case llvm::Instruction::FCmp:    m_type = OpType::CMP; break;
    case llvm::Instruction::Alloca:  m_type = OpType::ALLOC; break;
    case llvm::Instruction::Load:    m_type = OpType::LOAD; break;
    case llvm::Instruction::Store:   m_type = OpType::STORE; break;
    case llvm::Instruction::GetElementPtr:   m_type = OpType::GEP; break;
    case llvm::Instruction::ExtractElement:  m_type = OpType::EXTRACT_VEC; break;
    case llvm::Instruction::InsertElement:   m_type = OpType::INSERT_VEC; break;
    case llvm::Instruction::Trunc:   m_type = OpType::CAST; break;
    case llvm::Instruction::ZExt:    m_type = OpType::CAST; break;
    case llvm::Instruction::SExt:    m_type = OpType::CAST; break;
    case llvm::Instruction::FPToUI:  m_type = OpType::CAST; break;
    case llvm::Instruction::FPToSI:  m_type = OpType::CAST; break;
    case llvm::Instruction::UIToFP:  m_type = OpType::CAST; break;
    case llvm::Instruction::SIToFP:  m_type = OpType::CAST; break;
    case llvm::Instruction::FPTrunc: m_type = OpType::CAST; break;
    case llvm::Instruction::FPExt:   m_type = OpType::CAST; break;
    case llvm::Instruction::PtrToInt:   m_type = OpType::CAST; break;
    case llvm::Instruction::IntToPtr:   m_type = OpType::CAST; break;
    case llvm::Instruction::Call:    m_type = OpType::FN_CALL; break;

    default:
        CORE_INFO << " OpCode not supported yet:" << l_code;
        // CORE_ABORT(CORE_CONCAT << "OpCode not supported yet:" << l_code);
    }
}

auto Inst::name() const -> std::string_view {
    return m_inst.getName();
}

auto Inst::op_code_name() const -> std::string {
    return m_inst.getOpcodeName();
}

uint32_t Inst::raw_op_code() const {
    return m_inst.getOpcode();
}

uint32_t Inst::num_operands() const {
    return m_inst.getNumOperands();
}

uint32_t Inst::num_successor() const {
    return m_inst.getNumSuccessors();
}

bool Inst::is_terminator() const {
    return m_inst.isTerminator();
}

bool Inst::is_unary() const {
    return m_inst.isUnaryOp();
}

bool Inst::is_binary() const {
    return m_inst.isBinaryOp();
}

bool Inst::is_shift() const {
    return m_inst.isShift();
}

bool Inst::is_cast() const {
    return m_inst.isCast();
}

bool Inst::is_only_user() const {
    return meta::remove_const(m_inst).isOnlyUserOfAnyOperand();
}

bool Inst::is_atomic() const {
    return m_inst.isAtomic();
}

bool Inst::is_volatile() const {
    return m_inst.isVolatile();
}

bool Inst::is_throw() const {
    return m_inst.mayThrow();
}

bool Inst::have_side_effects() const {
    return m_inst.mayHaveSideEffects();
}

bool Inst::safe_to_remove() const {
    return m_inst.isSafeToRemove();
}

bool Inst::will_return() const {
    return m_inst.willReturn();
}

bool Inst::is_debug_or_psudo() const {
    return m_inst.isDebugOrPseudoInst();
}

bool Inst::may_write_memory() const {
    return m_inst.mayWriteToMemory();
}

bool Inst::may_read_memory() const {
    return m_inst.mayReadFromMemory();
}

bool Inst::has_prev() const {
    return m_inst.getPrevNonDebugInstruction() != nullptr;
}

bool Inst::has_next() const {
    return m_inst.getNextNonDebugInstruction() != nullptr;
}

auto Inst::prev() const -> std::unique_ptr<Inst> {
    CORE_ASSERT(has_prev());
    const llvm::Instruction *v = m_inst.getPrevNonDebugInstruction();
    if (v != nullptr) {
        return std::unique_ptr<Inst>(new Inst{*v});
    } else {
        return std::unique_ptr<Inst>();
    }
}

auto Inst::next() const -> std::unique_ptr<Inst> {
    CORE_ASSERT(has_next());
    const llvm::Instruction *v = m_inst.getNextNonDebugInstruction();
    if (v != nullptr) {
        return std::unique_ptr<Inst>(new Inst{*v});
    } else {
        return std::unique_ptr<Inst>();
    }
}

void Inst::analyze(std::ostream &os, const std::string &offset) const {
    os << offset << " > " << std::setw(15) << name() << std::setw(10) << " op:" << op_code_name()
        << " opcode:" << raw_op_code()
        << std::endl;
}

//
// CodeBlock
//
CodeBlock::CodeBlock(const llvm::BasicBlock& block)
    : m_block{block} {
    for (const llvm::Instruction& ii : m_block) {
        Inst l_inst{ii};
        m_insts.emplace_back(l_inst);
    }
}


void CodeBlock::analyze(std::ostream& os, const std::string& offset) const {
    os << offset << "CodeSection: BEGIN" << std::endl
       << offset << "    is_entry:" << m_block.isEntryBlock() << std::endl;
    for (const Inst &l_inst : m_insts) {
        l_inst.analyze(os, CORE_CONCAT << offset << "    ");
    }
    os << offset << "CodeSection: END" << std::endl;
}

//
// FunctionAnalysis
//
FunctionAnalysis::FunctionAnalysis(const llvm::Function& func)
    : m_func{func} {
    for (const llvm::BasicBlock& bi : m_func) {
        CodeBlock l_block{bi};
        m_code_blocks.emplace_back(l_block);
    }
}

int32_t FunctionAnalysis::num_inst() const {
    return m_func.getInstructionCount();
}

void FunctionAnalysis::analyze(std::ostream& os, const std::string& offset) const {
    os << offset << "Function: BEGIN" << std::endl
        << offset << "   num_instruction:" << m_func.getInstructionCount() << std::endl
        << offset << "   is_var_arg:" << m_func.isVarArg() << std::endl
        << offset << "   is_declaration:" << m_func.isDeclaration() << std::endl
        << offset << "   num_args:" << m_func.arg_size() << std::endl
        << offset << "   num_basic_blocks:" << m_func.size() << std::endl;
    for (const CodeBlock& l_block : m_code_blocks) {
        l_block.analyze(os, CORE_CONCAT << offset << "    ");
    }
    os << offset << "Function: END" << std::endl;
}

void FunctionAnalysis::for_each_inst(std::ostream& os, std::function<void(const Inst &)> &&fn) const {
    os << "Function: BEGIN" << std::endl
        << "   num_instruction:" << m_func.getInstructionCount() << std::endl
        << "   is_var_arg:" << m_func.isVarArg() << std::endl
        << "   is_declaration:" << m_func.isDeclaration() << std::endl
        << "   num_args:" << m_func.arg_size() << std::endl
        << "   num_basic_blocks:" << m_func.size() << std::endl;
    for (const CodeBlock& l_block : m_code_blocks) {
        l_block.for_each_inst(std::move(fn));
    }
    os << "Function: END" << std::endl;
}

//
// ModuleAnalysis
//
ModuleAnalysis::ModuleAnalysis(const Module& module)
    : m_module{module} {
    llvm::Module* m = m_module.native_handle();
    for(const llvm::Function& mi : *m) {
        FunctionAnalysis func_analysis{mi};
        m_funcs.emplace_back(func_analysis);
    }
}

ModuleAnalysis::~ModuleAnalysis() = default;

void ModuleAnalysis::analyze(std::ostream& os) const {
    os << " DEBUG MODULE BEGIN " << std::endl;
    for (const FunctionAnalysis &fn : m_funcs) {
        fn.analyze(os, "    ");
    }
    os << " DEBUG MODULE END" << std::endl;
}

void ModuleAnalysis::for_each_inst(std::ostream &os, std::function<void(const Inst &)> &&fn) const {
    os << " DEBUG MODULE BEGIN " << std::endl;
    for (const FunctionAnalysis &l_fn : m_funcs) {
        l_fn.for_each_inst(os, std::move(fn));
    }
    os << " DEBUG MODULE END" << std::endl;
}

}

LLVM_NS_END
